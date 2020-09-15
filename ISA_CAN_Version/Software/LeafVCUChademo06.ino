#include <SimpleISA.h>
#include <due_can.h>
#include <DueTimer.h>
#include <due_wire.h>
#include <Wire_EEPROM.h>
#include "globals.h"
#include "chademo.h"
/*
  CHAdeMO code for Damien Maguire's "Leaf VCU" hardware https://evbmw.com/index.php/evbmw-webshop/nissan-built-and-tested-boards/leaf-inverter-controller-fully-built-and-tested
  Converted from ATMega to SAM3X by Isaac Kelly
  Original rights to EVTV/Collin Kidder
*/

//#define DEBUG_TIMING	//if this is defined you'll get time related debugging messages
#define WIFI_ENABLED

template<class T> inline Print &operator <<(Print &obj, T arg) {
  obj.print(arg);  //Sets up serial streaming Serial<<someshit;
  return obj;
}

#define Serial SerialUSB

//These have been moved to eeprom. After initial compile the values will be read from EEPROM.
//These thus set the default value to write to eeprom upon first start up
#define MAX_CHARGE_V	158
#define MAX_CHARGE_A	130
#define TARGET_CHARGE_V	160
#define MIN_CHARGE_A	20
#define CAPACITY 180


const unsigned long Interval = 10;
unsigned long Time = 0;
unsigned long ChargeTimeRefSecs = 0; // reference time for charging.
unsigned long PreviousMillis = 0;
unsigned long CurrentMillis = 0;
float Voltage = 0;
float Current = 0;
float Power = 0;
float lastSavedAH = 0;
int Count = 0;
byte Command = 0; // "z" will reset the AmpHours and KiloWattHours counters
volatile uint8_t timerIntCounter = 0;
volatile uint8_t timerFastCounter  = 0;
volatile uint8_t timerChademoCounter = 0;
volatile uint8_t debugTick = 0;
volatile uint8_t wifiTick = 0;


uint16_t uint16Val;
uint16_t uint16Val2;
uint8_t uint8Val;
uint8_t print8Val;
uint8_t help8Val;

String cmdStr;

EESettings settings;
#define EEPROM_VALID	0xEE

ISA Sensor;


void timer2Int()
{
  timerFastCounter++;
  timerChademoCounter++;
  if (timerChademoCounter >= 3)
  {
    timerChademoCounter = 0;
    if (chademo.bChademoMode  && chademo.bChademoSendRequests) chademo.bChademoRequest = 1;
  }

  if (timerFastCounter == 8)
  {
    debugTick = 1;
    wifiTick = 1;
    timerFastCounter = 0;
    timerIntCounter++;
    if (timerIntCounter == 18)
    {
      timerIntCounter = 0;
    }
  }
}

void setup()
{
  //first thing configure the I/O pins and set them to a sane state
  pinMode(IN1, INPUT);
  pinMode(IN2, INPUT);
  pinMode(OUT1, OUTPUT);
  pinMode(OUT2, OUTPUT);
  pinMode(OUT3, OUTPUT);
  digitalWrite(OUT1, LOW);
  digitalWrite(OUT2, LOW);
  digitalWrite(OUT3, LOW);

  SerialUSB.begin(115200);//normal port
  Serial2.begin(19200);//wifi port

  Sensor.begin(0, 500); //Start ISA object on CAN 0 at 500 kbps
  Can1.begin(CAN_BPS_500K, 255);

  for (int filter = 0; filter < 7; filter++) {
    Can1.setRXFilter(filter, 0, 0, false);
  }
  for (int filter = 0; filter < 7; filter++) {
    Can0.setRXFilter(filter, 0, 0, false);
  }


  Wire.begin();
  EEPROM.read(0, settings);


  if (settings.valid != EEPROM_VALID) //not proper version so reset to defaults
  {
    settings.packSizeKWH = 33.0; //just a random guess. Maybe it should default to zero though?
    settings.valid = EEPROM_VALID;
    settings.ampHours = 0.0;
    settings.kiloWattHours = 0.0; // Is empty kiloWattHours count up
    settings.maxChargeAmperage = MAX_CHARGE_A;
    settings.maxChargeVoltage = MAX_CHARGE_V;
    settings.targetChargeVoltage = TARGET_CHARGE_V;
    settings.minChargeAmperage = MIN_CHARGE_A;
    settings.capacity = CAPACITY;
    settings.debuggingLevel = 1;
    Save();
  }
  help8Val = 1;
  print8Val = 1;
  if (settings.debuggingLevel > 0)
    settings.debuggingLevel = 0; //locked in to max debugging for now.


  Timer2.attachInterrupt(timer2Int);
  Timer2.start(25000); // Calls every 25ms

  updateTargetAV();
}


void loop()
{
  uint8_t pos;
  CurrentMillis = millis();
  uint8_t len;
  CAN_FRAME inFrame;
  float tempReading;

#ifdef DEBUG_TIMING
  if (debugTick == 1)
  {
    debugTick = 0;
    SerialUSB.println(millis());
  }
#endif

  if (wifiTick == 1)
  {
    handle_wifi();
    wifiTick = 0;
  }

  chademo.loop();

  if (CurrentMillis - PreviousMillis >= Interval)
  {
    Time = CurrentMillis - PreviousMillis;
    PreviousMillis = CurrentMillis;

    Count++;
    Voltage = Sensor.Voltage;
    Current = Sensor.Amperes;
    settings.ampHours = Sensor.AH;
    Power = Sensor.KW;

    //Count down kiloWattHours when drawing current.
    //Count up when charging (Current/Power is negative)
    settings.kiloWattHours = Sensor.KWH;

    chademo.doProcessing();

    if (Count >= 50)
    {
      Count = 0;

      USB();

      if (print8Val > 0)
        printSettings();
      else
        outputState();
      if (help8Val > 0)
        printHelp();
      if (chademo.bChademoMode)
      {
        if (settings.debuggingLevel > 0) {
          SerialUSB.print(F("Chademo Mode: "));
          SerialUSB.println(chademo.getState());
        }
      }
      else
      {
        checkChargingState(); // looking for reset of charging.
      }
    }
  }
  if (Can1.available() > 0) {
    Can1.read(inFrame);
    chademo.handleCANFrame(inFrame);
  }
}

void updateTargetAV()
{
  chademo.setTargetAmperage(settings.maxChargeAmperage);
  chademo.setTargetVoltage(settings.targetChargeVoltage);
}

void Save()
{
  SerialUSB.println("Saving EEPROM");
  noInterrupts();
  EEPROM.write(0, settings);
  lastSavedAH = settings.ampHours;
  SerialUSB.println (F("SAVED"));
  interrupts();
  delay(1000);
}

void RngErr() {
  SerialUSB.println(F("Range Error"));
}


void ParseCommand() {
  float fltVal = 0;
  if (cmdStr == "V") {
    uint16Val = SerialUSB.parseInt();
    if (uint16Val > 0 && uint16Val < 1000) {
      settings.targetChargeVoltage = uint16Val;
      Save();
      print8Val = 1;
    } else {
      RngErr();
    }
  }
  else if (cmdStr == "VMAX") {
    uint16Val = SerialUSB.parseInt();
    if (uint16Val > 0 && uint16Val < 1000) {
      settings.maxChargeVoltage = uint16Val;
      Save();
      print8Val = 1;
    } else {
      RngErr();
    }
  }
  else if (cmdStr == "AMIN") {
    uint8Val = SerialUSB.parseInt();
    if (uint8Val > 0 && uint8Val < 256) {
      settings.minChargeAmperage = uint8Val;
      Save();
      print8Val = 1;
    } else {
      RngErr();
    }
  }
  else if (cmdStr == "AMAX") {
    uint8Val = SerialUSB.parseInt();
    if (uint8Val > 0 && uint8Val < 256) {
      settings.maxChargeAmperage = uint8Val;
      Save();
      print8Val = 1;
    } else {
      RngErr();
    }
  }
  else if (cmdStr == "CAPAH") {
    uint8Val = SerialUSB.parseInt();
    if (uint8Val > 0 && uint8Val < 256) {
      settings.capacity = uint8Val;
      Save();
      print8Val = 1;
    } else {
      RngErr();
    }
  }
  else if (cmdStr == "CAPKWH") {
    fltVal = SerialUSB.parseFloat();
    if (fltVal > 0 && fltVal < 100) {
      settings.packSizeKWH = fltVal;
      Save();
      print8Val = 1;
    } else {
      RngErr();
    }
  }
  else if (cmdStr == "DBG") {
    uint8Val = SerialUSB.parseInt();
    if (uint8Val >= 0 && uint8Val < 4) {
      settings.debuggingLevel = uint8Val;
      Save();
      print8Val = 1;
    } else {
      RngErr();
    }
  }
  else if (cmdStr == "SAVE") {
    Save();
  }
  else if (cmdStr == "FULLRESET") {
    SerialUSB.println(F("Resetting EEPROM"));
    settings.valid = 0;
    Save();
  }
  else {
    help8Val = 1;
  }
}


void checkChargingState() {
  //If the current is negative we are either charging or regenerating.
  //If we are charging we expect a relatively constant current decreasing over
  //a period of time. Ie. a small rate of change of current while charging, throughout the charging period.
  unsigned long CurrentSecs = CurrentMillis / 1000;
  if (Current < -2.0 && Current > -20 && Voltage >= settings.targetChargeVoltage) { // Charging or regenerating at less than 20 amps
    //while we are charging batteries at less than 20 amps we assume if this occurs for longer than 5 minutes
    //we are charging and not regenerating
    if (Current > -15 && (CurrentSecs - ChargeTimeRefSecs) > 30) { // have been between 0 and -15 amps for more than 5 minutes
      //At this point we assume that we have been charging for the last 5 minutes or more prior to hitting -4 amps
      ResetChargeState();
      ChargeTimeRefSecs = CurrentSecs;
    }
  } else // Normal driving conditions or we don't care. So we keep resetting the reference.
    ChargeTimeRefSecs = CurrentSecs;
}

void ResetChargeState() {
  SerialUSB.println(F("Reset Ah & Wh"));
  settings.ampHours = 0.0;
  settings.kiloWattHours = settings.packSizeKWH;
  Sensor.RESTART();
  Save();
  print8Val = 1;
  SerialUSB.println(F("Done!!!"));
}

void outputState() {

  SerialUSB.print (F("JLD505: "));
  SerialUSB.print (Voltage, 3);
  SerialUSB.print (F("v "));
  SerialUSB.print (Current, 2);
  SerialUSB.print (F("A "));
  SerialUSB.print (settings.ampHours, 1);
  SerialUSB.print (F("Ah "));
  SerialUSB.print (Power, 1);
  SerialUSB.print (F("kW "));
  SerialUSB.print (settings.kiloWattHours, 1);
  SerialUSB.print (F("kWh "));
  SerialUSB.print (F("OUT1"));
  SerialUSB.print (digitalRead(OUT1) > 0 ? F(":1 ") : F(":0 "));
  SerialUSB.print (F("OUT2"));
  SerialUSB.print (digitalRead(OUT2) > 0 ? F(":1 ") : F(":0 "));
  SerialUSB.print (F("OUT3"));
  SerialUSB.print (digitalRead(OUT3) > 0 ? F(":1 ") : F(":0 "));
  SerialUSB.print (F("IN1"));
  SerialUSB.print (digitalRead(IN1) > 0 ? F(":1 ") : F(":0 "));
  SerialUSB.print (F("IN2"));
  SerialUSB.print (digitalRead(IN2) > 0 ? F(":1") : F(":0 "));
  SerialUSB.print (F("CHG T: "));
  SerialUSB.println (CurrentMillis / 1000 - ChargeTimeRefSecs);
}

void USB() {
  SerialCommand();
}


void SerialCommand()
{
  int available = SerialUSB.available();
  if (available > 0)
  {
    Command = SerialUSB.peek();
    if (Command == 'z')
    {
      ResetChargeState();
    }
    else if (Command == 'p')
    {
      print8Val = 1;
    }
    else if (Command == 'u')
    {
      updateTargetAV();
    }
    else if (Command == 'h') {
      help8Val = 1;
    }
    else {
      cmdStr = SerialUSB.readStringUntil('=');
      ParseCommand();
    }
    while (SerialUSB.available() > 0) SerialUSB.read();
  }
}

void printSettings() {
  print8Val = 0;
  SerialUSB.println (F("-"));
  SerialUSB.print (F("AH "));
  SerialUSB.println (settings.ampHours);
  SerialUSB.print (F("KWH "));
  SerialUSB.println (settings.kiloWattHours);
  SerialUSB.print (F("AMIN "));
  SerialUSB.println (settings.minChargeAmperage, 1);
  SerialUSB.print (F("AMAX "));
  SerialUSB.println (settings.maxChargeAmperage, 1);
  SerialUSB.print (F("V "));
  SerialUSB.println (settings.targetChargeVoltage, 1);
  SerialUSB.print (F("VMAX "));
  SerialUSB.println (settings.maxChargeVoltage, 1);
  SerialUSB.print (F("CAPAH "));
  SerialUSB.println (settings.capacity, 1);
  SerialUSB.print (F("CAPKWH "));
  SerialUSB.println (settings.packSizeKWH, 2);
  SerialUSB.print (F("DBG "));
  SerialUSB.println (settings.debuggingLevel, 1);
  SerialUSB.println (F("-"));
}

void printHelp() {
  help8Val = 0;
  SerialUSB.println(F("Commands"));
  SerialUSB.println(F("h - prints this message"));
  SerialUSB.println(F("z - resets AH/KWH readings"));
  SerialUSB.println(F("p - shows settings"));
  SerialUSB.println(F("u - updates CHAdeMO settings"));
  SerialUSB.println(F("AMIN - Sets min CHAdeMO current"));
  SerialUSB.println(F("AMAX - Sets max CHAdeMO current"));
  SerialUSB.println(F("V - Sets CHAdeMO voltage target"));
  SerialUSB.println(F("VMAX - Sets CHAdeMO max voltage (for protocol reasons)"));
  SerialUSB.println(F("CAPAH - Sets pack amp-hours"));
  SerialUSB.println(F("CAPKWH - Sets pack kilowatt-hours"));
  SerialUSB.println(F("DBG - Sets debugging level"));
  SerialUSB.println(F("Example: V=395 - sets CHAdeMO voltage target to 395"));
}

void timestamp()
{
  int milliseconds = (int) (millis() / 1) % 1000 ;
  int seconds = (int) (millis() / 1000) % 60 ;
  int minutes = (int) ((millis() / (1000 * 60)) % 60);
  int hours   = (int) ((millis() / (1000 * 60 * 60)) % 24);

  SerialUSB.print(F(" Time:"));
  SerialUSB.print(hours);
  SerialUSB.print(F(":"));
  SerialUSB.print(minutes);
  SerialUSB.print(F(":"));
  SerialUSB.print(seconds);
  SerialUSB.print(F("."));
  SerialUSB.println(milliseconds);
}
void handle_wifi() {
  /*

     Routine to send data to wifi on serial 2
    The information will be provided over serial to the esp8266 at 19200 baud 8n1 in the form :
    vxxx,ixxx,pxxx,mxxxx,nxxxx,oxxx,rxxx,qxxx* where :

    v=pack voltage (0-700Volts)
    i=current (0-1000Amps)
    p=power (0-300kw)
    m=chademo state
    n=KWH

     =end of string
    xxx=three digit integer for each parameter eg p100 = 100kw.
    updates will be every 100ms approx.

    v100,i200,p35,m3000,n4000*

    Chademo state enums
    STARTUP,
    SEND_INITIAL_PARAMS,
    WAIT_FOR_EVSE_PARAMS,
    SET_CHARGE_BEGIN,
    WAIT_FOR_BEGIN_CONFIRMATION,
    CLOSE_CONTACTORS,
    RUNNING,
    CEASE_CURRENT,
    WAIT_FOR_ZERO_CURRENT,
    OPEN_CONTACTOR,
    FAULTED,
    STOPPED,
    LIMBO
  */

  //Serial2.print("v100,i200,p35,m3000,n4000,o20,r30,q50*"); //test string

  digitalWrite(13, !digitalRead(13)); //blink led every time we fire this interrrupt.

  Serial2.print("v");//dc bus voltage
  Serial2.print(Sensor.Voltage);//voltage derived from ISA shunt
  Serial2.print(",i");//dc current
  Serial2.print(Sensor.Amperes);//current derived from ISA shunt
  Serial2.print(",p");//total motor power
  Serial2.print(Sensor.KW);//Power value derived from ISA Shunt
  Serial2.print(",m");//Chademo state
  Serial2.print(chademo.getState());
  Serial2.print(",n");//ISA Kwh measured
  Serial2.print(Sensor.KWH);// change to kwh on wifi.
  Serial2.print("*");// end of data indicator
}
