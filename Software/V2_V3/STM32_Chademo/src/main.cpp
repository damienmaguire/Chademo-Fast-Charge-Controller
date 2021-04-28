/*
 * This file is part of the stm32-template project.
 *
 * Copyright (C) 2020 Johannes Huebner <dev@johanneshuebner.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdint.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/can.h>
#include <libopencm3/stm32/iwdg.h>
#include "stm32_can.h"
#include "terminal.h"
#include "params.h"
#include "hwdefs.h"
#include "digio.h"
#include "hwinit.h"
#include "anain.h"
#include "param_save.h"
#include "my_math.h"
#include "errormessage.h"
#include "printf.h"
#include "stm32scheduler.h"
#include "chademo.h"
#include "isa_shunt.h"

static Stm32Scheduler* scheduler;
static Can* can;
static bool chargeMode = false;

static void CanCallback(uint32_t id, uint32_t data[2])
{
   switch (id)
   {
      case 0x108:
         ChaDeMo::Process108Message(data);
         break;
      case 0x109:
         ChaDeMo::Process109Message(data);
         break;
      case 0x511:
         break;
      case 0x521:
         ISA::handle521(data);
         break;
      case 0x522:
         ISA::handle522(data);
         break;
      case 0x523:
         ISA::handle523(data);
         break;
      case 0x524:
         ISA::handle524(data);
         break;
      case 0x525:
         ISA::handle525(data);
         break;
      case 0x526:
         ISA::handle526(data);
         break;
      case 0x527:
         ISA::handle527(data);
         break;
      case 0x528:
         ISA::handle528(data);
         break;
      default:
         break;
   }
}


static void RunChaDeMo()
{
   static uint32_t connectorLockTime = 0;

   if (!chargeMode && rtc_get_counter_val() > 100) //100*10ms = 1s
   {

      //Check for Charger start stop 1
      if (DigIo::charger_in_1.Get())
      {
         chargeMode = true;
         Param::SetInt(Param::opmode, MOD_CHARGESTART);
      }
   }

   /* 1s after entering charge mode, enable charge permission */
   if (Param::GetInt(Param::opmode) == MOD_CHARGESTART && rtc_get_counter_val() > 200)
   {
      //TODO compatibility check?
      ChaDeMo::SetEnabled(true);
      DigIo::charge_enable_out.Set();
   }

   if (connectorLockTime == 0 && ChaDeMo::ConnectorLocked())
   {
      connectorLockTime = rtc_get_counter_val();
      Param::SetInt(Param::opmode, MOD_CHARGELOCK);
   }

   //Once charger 2, close HV contactors
   if (Param::GetInt(Param::opmode) == MOD_CHARGELOCK && (rtc_get_counter_val() - connectorLockTime) > 1000 &&
         DigIo::charger_in_2.Get())
   {
      ChaDeMo::SetContactor(true);
      Param::SetInt(Param::opmode, MOD_CHARGE);
      DigIo::hv_enable_out.Set();

   }

   if (Param::GetInt(Param::opmode) == MOD_CHARGE)
   {
      int chargeCur = Param::GetInt(Param::chgcurlim);
      int chargeLim = Param::GetInt(Param::chargelimit);
      chargeCur = MIN(MIN(255, chargeLim), chargeCur);
      ChaDeMo::SetChargeCurrent(chargeCur);
      ChaDeMo::CheckSensorDeviation(Param::GetInt(Param::udcbms));
   }

   if (Param::GetInt(Param::opmode) == MOD_CHARGEND)
   {
      ChaDeMo::SetChargeCurrent(0);
   }

   ChaDeMo::SetTargetBatteryVoltage(Param::GetInt(Param::udcthresh));
   ChaDeMo::SetSoC(Param::Get(Param::soc));
   Param::SetInt(Param::cdmcureq, ChaDeMo::GetRampedCurrentRequest());

   if (chargeMode)
   {
      if (Param::GetInt(Param::batfull) ||
          Param::Get(Param::soc) >= Param::Get(Param::soclimit) ||
          Param::GetInt(Param::chargelimit) == 0)
         // !ISA::Alive(rtc_get_counter_val()))
      {
        // if (!ISA::Alive(rtc_get_counter_val()))
        // {
        //    ChaDeMo::SetGeneralFault();
         //}
         ChaDeMo::SetEnabled(false);
         //TODO:: SET CHARGE ENABLE OUTPUT OFF
         Param::SetInt(Param::opmode, MOD_CHARGEND);
      }

      Param::SetInt(Param::udccdm, ChaDeMo::GetChargerOutputVoltage());
      Param::SetInt(Param::idccdm, ChaDeMo::GetChargerOutputCurrent());
      ChaDeMo::SendMessages(can);
   }
   Param::SetInt(Param::cdmstatus, ChaDeMo::GetChargerStatus());
//   if (!ISA::Alive(rtc_get_counter_val()))
  // {
  //    ErrorMessage::Post(ERR_SHUNTCOMM);
  // }
}
//sample 100ms task
static void Ms100Task(void)
{
   //The following call toggles the LED output, so every 100ms
   //The LED changes from on to off and back.
   //Other calls:
   //DigIo::led_out.Set(); //turns LED on
   //DigIo::led_out.Clear(); //turns LED off
   //For every entry in digio_prj.h there is a member in DigIo
   DigIo::led_out.Toggle();
   //The boot loader enables the watchdog, we have to reset it
   //at least every 2s or otherwise the controller is hard reset.
   iwdg_reset();
   //Calculate CPU load. Don't be surprised if it is zero.
   s32fp cpuLoad = FP_FROMINT(scheduler->GetCpuLoad());
   //This sets a fixed point value WITHOUT calling the parm_Change() function
   Param::SetFlt(Param::cpuload, cpuLoad / 10);

      RunChaDeMo();

   //If we chose to send CAN messages every 100 ms, do this here.
 //  if (Param::GetInt(Param::canperiod) == CAN_PERIOD_100MS)
  //    can->SendAll();

    int16_t IsaTemp=ISA::Temperature;
    Param::SetInt(Param::tmpisa,IsaTemp);
}

//sample 10 ms task
static void Ms10Task(void)
{
   //Set timestamp of error message
   ErrorMessage::SetTime(rtc_get_counter_val());

   if (DigIo::test_in.Get())
   {
      //Post a test error message when our test input is high
      ErrorMessage::Post(ERR_TESTERROR);
   }

   int32_t IsaVolt = FP_FROMINT(ISA::Voltage)/1000;
    Param::SetFlt(Param::udcbms,IsaVolt);

    int32_t idc = FP_FROMINT(ISA::Amperes)/1000;
    Param::SetFlt(Param::IsaIdc, idc);

    int32_t kw = FP_FROMINT(ISA::KW)/1000;
    Param::SetFlt(Param::power, kw);

    int32_t kwh = FP_FROMINT(ISA::KWh)/1000;
    Param::SetFlt(Param::KWh, kwh);

    int32_t Amph = FP_FROMINT(ISA::Ah)/3600;
    Param::SetFlt(Param::AMPh, Amph);

}

/** This function is called when the user changes a parameter */
extern void parm_Change(Param::PARAM_NUM paramNum)
{
   switch (paramNum)
   {
   default:
      //Handle general parameter changes here. Add paramNum labels for handling specific parameters
      break;
   }
}

//Whichever timer(s) you use for the scheduler, you have to
//implement their ISRs here and call into the respective scheduler
extern "C" void tim2_isr(void)
{
   scheduler->Run();
}

extern "C" int main(void)
{
   extern const TERM_CMD termCmds[];

   clock_setup(); //Must always come first
   rtc_setup();
   ANA_IN_CONFIGURE(ANA_IN_LIST);
   DIG_IO_CONFIGURE(DIG_IO_LIST);
   AnaIn::Start(); //Starts background ADC conversion via DMA
   write_bootloader_pininit(); //Instructs boot loader to initialize certain pins
   gpio_primary_remap(AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON, AFIO_MAPR_CAN1_REMAP_PORTB);
   tim_setup(); //Sample init of a timer
   nvic_setup(); //Set up some interrupts
   parm_load(); //Load stored parameters

   Stm32Scheduler s(TIM2); //We never exit main so it's ok to put it on stack
   scheduler = &s;
   //Initialize CAN1, including interrupts. Clock must be enabled in clock_setup()
   Can c(CAN1, Can::Baud500);
   //store a pointer for easier access
   can = &c;
   c.SetNodeId(5);
   c.SetReceiveCallback(CanCallback);
      //Chademo
   c.RegisterUserMessage(0x108);
   c.RegisterUserMessage(0x109);

   //IVA Shunt
   c.RegisterUserMessage(0x511);
   c.RegisterUserMessage(0x521);
   c.RegisterUserMessage(0x522);
   c.RegisterUserMessage(0x523);
   c.RegisterUserMessage(0x524);
   c.RegisterUserMessage(0x525);
   c.RegisterUserMessage(0x526);
   c.RegisterUserMessage(0x526);
   c.RegisterUserMessage(0x527);
   c.RegisterUserMessage(0x528);

   //This is all we need to do to set up a terminal on USART3
   Terminal t(USART3, termCmds);

   //Up to four tasks can be added to each timer scheduler
   //AddTask takes a function pointer and a calling interval in milliseconds.
   //The longest interval is 655ms due to hardware restrictions
   //You have to enable the interrupt (int this case for TIM2) in nvic_setup()
   //There you can also configure the priority of the scheduler over other interrupts
   s.AddTask(Ms10Task, 10);
   s.AddTask(Ms100Task, 100);

   //backward compatibility, version 4 was the first to support the "stream" command
   Param::SetInt(Param::version, 4);
   parm_Change(Param::PARAM_LAST); //Call callback one for general parameter propagation

   //Now all our main() does is running the terminal
   //All other processing takes place in the scheduler or other interrupt service routines
   //The terminal has lowest priority, so even loading it down heavily will not disturb
   //our more important processing routines.
   while(1)
      t.Run();

   return 0;
}

