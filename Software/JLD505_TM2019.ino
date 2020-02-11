#include <SPI.h> // Arduino - 1.1.1
#include <EEPROM.h> // Arduino - 2.0.0
#include <mcp_can.h> // libraries folder
#include <INA226.h> // libraries folder
#include <EEPROMAnything.h> // libraries folder
#include <SoftwareSerial.h> // Arduino - 1.0.0
#include <FrequencyTimer2.h> // libraries folder
#include "src/globals.h"
#include "src/chademo.h"
/*
Notes on what needs to be done:
- Timing analysis showed that the USB, CANBUS, and BT routines take up entirely too much time. They can delay processing by
   almost 100ms! Minor change for test.

- Investigate what changes are necessary to support the Cortex M0 processor in the Arduino Zero

- Interrupt driven CAN has a tendency to lock up. It has been disabled for now - It locks up even if the JLD is not sending anything
  but it seems to be able to actually send as much as you want. Only interrupt driven reception seems to make things die.

Note about timing related code: The function millis() returns a 32 bit integer that specifies the # of milliseconds since last start up.
That's all well and good but 4 billion milliseconds is a little less than 50 days. One might ask "so?" well, this could potentially run
indefinitely in some vehicles and so would suffer rollover every 50 days. When this happens improper code will become stupid. So, try
to implement any timing code like this:
if ((millis() - someTimeStamp) >= SomeInterval) do stuff

Such a code block will do the proper thing so long as all variables used are unsigned long / uint32_t variables.

*/
//#define USE_ELCON
#define SCOTT_BMS
//#define ON_BOARD_CHARGER

//#define DEBUG_TIMING	//if this is defined you'll get time related debugging messages
//#define CHECK_FREE_RAM //if this is defined it does what it says - reports the lowest free RAM found at any point in the sketch

template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; } //Sets up serial streaming Serial<<someshit;

SoftwareSerial BTSerial(A2, A3); // RX | TX

//These have been moved to eeprom. After initial compile the values will be read from EEPROM.
//These thus set the default value to write to eeprom upon first start up
#define MAX_CHARGE_V	158
#define MAX_CHARGE_A	130
#define TARGET_CHARGE_V	160
#define MIN_CHARGE_A	20
#define INITIAL_SOC 100
#define CAPACITY 180
#define EEPROM_BASE 255
#define PRINT_USB 1
#define PRINT_BT 2
#define STATE_USB 1
#define STATE_BT 2

/**
 * Output modes for OUT0 and OUT1
 * 
 * In each case an Hysteresis applies after crossing the threshold to reset the output.
 */
#define OUTMODE_OFF 0       //OFF
#define OUTMODE_V_LT 1      //Voltage less than threshold
#define OUTMODE_V_GT 2      //Voltage greater than threshold
#define OUTMODE_A_LT 3      //Current less than threshold
#define OUTMODE_A_GT 4      //Current greater than threshold
#define OUTMODE_S_LT 5      //State of charge less than threshold
#define OUTMODE_S_GT 6      //State of charge greater than threshold
#define OUTMODE_V_LT_AH 7   //Same as OUTMODE_V_LT but also triggers if the AH > Capacity >> special case.

#define ELCON_PARAMS_ID 0x18FF50E5

INA226 ina;
const unsigned long Interval = 10;
unsigned long Time = 0;
unsigned long ChargeTimeRefSecs = 0; // reference time for charging.
unsigned long PreviousMillis = 0;
unsigned long CurrentMillis = 0;
float Voltage = 0;
float Current = 0;
float Power = 0;
float lastSavedAH = 0;
float Debug_Amps = 0;
int Count = 0;
byte Command = 0; // "z" will reset the AmpHours and KiloWattHours counters
volatile uint8_t bStartConversion = 0;
volatile uint8_t bGetTemperature = 0;
volatile uint8_t timerIntCounter = 0;
volatile uint8_t timerFastCounter  = 0;
volatile uint8_t timerChademoCounter = 0;
volatile uint8_t sensorReadPosition = 255;
uint8_t tempSensorCount = 0;
int32_t canMsgID = 0;
unsigned char canMsg[8];
unsigned char Flag_Recv = 0;
unsigned char Flag_Was_Running_Chademo = 0;
volatile uint8_t debugTick = 0;

int16_t lowestFreeRAM = 2048;

uint16_t uint16Val;
uint16_t uint16Val2;
uint8_t uint8Val;
uint8_t print8Val;
uint8_t state8Val;
byte limpMode;

String strVal;
String cmdStr;

EESettings settings;
#define EEPROM_VALID	0xEE

void MCP2515_ISR()
{
    //CAN.handleInt();
	Flag_Recv = 1;
}

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
		timerFastCounter = 0;
		timerIntCounter++;
		if (timerIntCounter < 10)
		{
			bGetTemperature = 1;
			sensorReadPosition++;
		}
		if (timerIntCounter == 10)
		{
			bStartConversion = 1;
			sensorReadPosition = 255;
		}
		if (timerIntCounter == 18)
		{
			timerIntCounter = 0;
		}
	}
}
  
void setup()
{ 
  //first thing configure the I/O pins and set them to a sane state
	pinMode(IN0, INPUT);
	pinMode(IN1, INPUT);
	pinMode(OUT0, OUTPUT);
	pinMode(OUT1, OUTPUT);
	digitalWrite(OUT0, LOW);
	digitalWrite(OUT1, LOW);
	pinMode(A1, OUTPUT); //KEY - Must be HIGH
	pinMode(A0, INPUT); //STATE
	digitalWrite(A1, HIGH);
  pinMode(2, INPUT);
	pinMode(3, INPUT_PULLUP); //enable weak pull up on MCP2515 int pin connected to INT1 on MCU

	Serial.begin(115200);
	BTSerial.begin(115200);
  //BTSerial.println(F("AT+NAMEJLD505"));
	//altSerial.begin(9600);
	
	//TAM -- sensors.begin();
	//TAM -- sensors.setWaitForConversion(false); //we're handling the time delay ourselves so no need to wait when asking for temperatures

	//CAN.begin(CAN_250KBPS); // For Elcon 250kb/s
  CAN.begin(CAN_500KBPS);
	attachInterrupt(1, MCP2515_ISR, FALLING);     // start interrupt

	ina.begin(69);
	ina.configure(INA226_AVERAGES_16, INA226_BUS_CONV_TIME_1100US, INA226_SHUNT_CONV_TIME_1100US, INA226_MODE_SHUNT_BUS_CONT);

	EEPROM_readAnything(EEPROM_BASE, settings);
	if (settings.valid != EEPROM_VALID) //not proper version so reset to defaults
	{
    settings.packSizeKWH = 33.0; //just a random guess. Maybe it should default to zero though?
		settings.valid = EEPROM_VALID;
		settings.ampHours = 0.0;
		settings.kiloWattHours = 0.0; // Is empty kiloWattHours count up
		settings.currentCalibration = 600.0/0.075; //800A 75mv shunt
		settings.voltageCalibration = (100000.0*830000.0/930000.0+1000000.0)/(100275.0*830000.0/930000.0); // (Voltage Divider with (100k in parallel with 830k) and 1M )
		settings.maxChargeAmperage = MAX_CHARGE_A;
		settings.maxChargeVoltage = MAX_CHARGE_V;
		settings.targetChargeVoltage = TARGET_CHARGE_V;
		settings.minChargeAmperage = MIN_CHARGE_A;
    settings.SOC=INITIAL_SOC;
    settings.capacity=CAPACITY;
		settings.debuggingLevel = 1;
    settings.serialOutput = 1; // USB
    settings.out0Mode = OUTMODE_OFF;
    settings.out0Thresh = 0;
    settings.out0Hys = 0;
    settings.out1Mode = OUTMODE_OFF;
    settings.out1Thresh = 0;
    settings.out1Hys = 0;
    settings.currentOffset = 0;
    Save();
	}
  
  //settings.serialOutput = 0; // Output to USB by default
  print8Val = PRINT_USB;
  state8Val = 0;
	if(settings.debuggingLevel>1)
	  settings.debuggingLevel = 0; //locked in to max debugging for now.

  limpMode = 0;

	attachInterrupt(digitalPinToInterrupt(2), Save, RISING); // Modification to board to use a rising level interrupt when board powers down
  //attachInterrupt(digitalPinToInterrupt(2), Save, FALLING);
  
	FrequencyTimer2::setPeriod(25000); //interrupt every 25ms
	FrequencyTimer2::setOnOverflow(timer2Int);
  
	updateTargetAV();
}

void(* resetFunc) (void) = 0; //declare reset function @ address 0

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
		Serial.println(millis());
	}
#endif 

	chademo.loop();
 
	if(CurrentMillis - PreviousMillis >= Interval)
	{
		Time = CurrentMillis - PreviousMillis;
		PreviousMillis = CurrentMillis;
    
		Count++;
		Voltage = ina.readBusVoltage() * settings.voltageCalibration;
		Current = ina.readShuntVoltage() * settings.currentCalibration + settings.currentOffset;
    if(settings.debuggingLevel==3){ //Current counting debugging
      if(Voltage<1) // not connected - just set a dummy value
        Voltage = settings.targetChargeVoltage;
      Current = Debug_Amps;
    }
		settings.ampHours += Current * (float)Time / 1000.0 / 3600.0;
		Power = Voltage * Current / 1000.0;
    
    //Count down kiloWattHours when drawing current. 
    //Count up when charging (Current/Power is negative)
		settings.kiloWattHours -= Power * (float)Time / 1000.0 / 3600.0; 
    settings.SOC=((settings.capacity-settings.ampHours)/settings.capacity)*100;

    if(settings.ampHours > settings.capacity && Current > 50)
      limpMode = 1;

		chademo.doProcessing();

		if (Count >= 50)
		{
			Count = 0;

      USB();  
      BT(); 
      
      if(print8Val>0)
        printSettings();
      else if(state8Val>0)
        outputState();		
      else if(settings.serialOutput>0){
        state8Val = settings.serialOutput;
        outputState();						
      }

			if (chademo.bChademoMode)
      {   
        Flag_Was_Running_Chademo = 1;
        if (settings.debuggingLevel > 0) {
          Serial.print(F("Chademo Mode: "));
          Serial.println(chademo.getState());
        }
      }
      else
			{					
        checkChargingState(); // looking for reset of charging.
        updateOutput(OUT0);
        updateOutput(OUT1);

        if(Flag_Was_Running_Chademo>0){
          //If we were just running chademo, perform save on return to normal mode.
          Save();
          Flag_Was_Running_Chademo = 0;
        }			

#ifdef SCOTT_BMS
      wakeUpScott();
#endif
			}

      if(settings.debuggingLevel!=3)
			  saveStateToEEPROM();
     
#ifdef CHECK_FREE_RAM
			Serial.print(F("Lowest free RAM: "));
			Serial.println(lowestFreeRAM);
#endif
		}
	}

	if (Flag_Recv || (CAN.checkReceive() == CAN_MSGAVAIL)) {
		Flag_Recv = 0;
		CAN.receiveFrame(inFrame);
#ifdef USE_ELCON
    ELCON(inFrame);
#endif
//#ifdef SCOTT_BMS
//    SCOTT(inFrame);
//#endif
		chademo.handleCANFrame(inFrame);
	}

 //if(CAN.checkError() > 0){
 // if(Count==0)
 //   Serial.print("CAN ERROR: ");
// }
  
	checkRAM();
}

void updateOutput(int output){
  //Don't do this during Chademo Mode
  if (!chademo.bChademoMode){
    //If we are charging on a conventional charger
    if(output==OUT0){
      uint16Val = settings.out0Thresh;
      uint16Val2 = settings.out0Hys;
      uint8Val = settings.out0Mode;
    }else if(output==OUT1){
      uint16Val = settings.out1Thresh;
      uint16Val2 = settings.out1Hys;
      uint8Val = settings.out1Mode;
    }else
      return; // don't allow action on any other pins
    
    switch(uint8Val){
      case OUTMODE_OFF:
          digitalWrite(output, LOW);
        break;
      case OUTMODE_V_LT:
        if(Voltage > uint16Val)
          digitalWrite(output, LOW);
        else if(Voltage < uint16Val - uint16Val2)
          digitalWrite(output, HIGH);
        break;
      case OUTMODE_V_LT_AH:
        if(Voltage > uint16Val || limpMode>0)
          digitalWrite(output, LOW);
        else if(Voltage < uint16Val - uint16Val2)
          digitalWrite(output, HIGH);
        break;
      case OUTMODE_V_GT:
        if(Voltage > uint16Val)
          digitalWrite(output, HIGH);
        else if(Voltage < uint16Val - uint16Val2)
          digitalWrite(output, LOW);
        break;
      case OUTMODE_A_LT: 
        if(Current < uint16Val)
          digitalWrite(output, HIGH);
        else if(Current > uint16Val + uint16Val2)
          digitalWrite(output, LOW);
        break;
      case OUTMODE_A_GT: 
        if(Current > uint16Val)
          digitalWrite(output, HIGH);
        else if(Current < uint16Val - uint16Val2)
          digitalWrite(output, LOW);
        break;
      case OUTMODE_S_LT: 
        if(settings.SOC < uint16Val)
          digitalWrite(output, HIGH);
        else if(settings.SOC >= uint16Val + uint16Val2)
          digitalWrite(output, LOW);
        break;
      case OUTMODE_S_GT: 
        if(settings.SOC > uint16Val)
          digitalWrite(output, HIGH);
        else if(settings.SOC <= uint16Val - uint16Val2)
          digitalWrite(output, LOW);
        break;
      default: break;
    }
  }
}

void updateTargetAV()
{
  chademo.setTargetAmperage(settings.maxChargeAmperage);
  chademo.setTargetVoltage(settings.targetChargeVoltage);
}

void saveStateToEEPROM(){
  //Don't want to save too frequently or will soon damage EEPROM
  //There is a Save() triggered when power is removed. However its not 100% robust.
  //Want to save regularly enough that we don't lose data.
  //The main values that change during regular use are the AH, KWH count and SOC.
  //Have two basic cases - Under load - 
  // 1) relatively significant current draw > 5A
  //    If the AH change by more than 0.5AH then we save().
  // 2) idle or low current draw <= 5A
  //    If the AH change by more than 0.1AH then we save().
  float val = settings.ampHours - lastSavedAH;
  if(Current > 5 || Current < -5){
    if(fabs(val) > 0.5)
      Save();
  }else{
    if(fabs(val) > 0.1)
      Save();
  }
}

void Save()
{
  noInterrupts();
	EEPROM_writeAnything(EEPROM_BASE, settings);
  lastSavedAH = settings.ampHours;
  Serial.println (F("SAVED"));
  interrupts();
}  

void RngErr(){
  Serial.println(F("Range Error"));
}
void NotImpl(){
  Serial.println(F("Not Implemented"));
}

void SetAH(float ah){
  settings.ampHours = ah;
  float val = ((settings.capacity-settings.ampHours)/settings.capacity);
  settings.kiloWattHours = val * settings.packSizeKWH;
  settings.SOC=val*100;
}

void SetKwh(float kwh){
  settings.kiloWattHours = kwh;
  float val = kwh / settings.packSizeKWH;
  settings.ampHours = (1.0 - val)*settings.capacity;
  settings.SOC = val*100;
}

void ParseCommand(int bt){
  float fltVal = 0;
  if(cmdStr == "AH"){
    fltVal = bt>0 ? BTSerial.parseFloat() : Serial.parseFloat();
    if(fltVal>=0 && fltVal<1000){
      SetAH(fltVal);
      Save();
      print8Val = bt>0 ? PRINT_BT : PRINT_USB;
    }else{
      RngErr();
    }
  }
  else if(cmdStr == "AMPS"){
    fltVal = bt>0 ? BTSerial.parseFloat() : Serial.parseFloat();
    if(fltVal>-1000 && fltVal<1000){
      Debug_Amps = fltVal;
    }else{
      RngErr();
    }
  }
  else if(cmdStr == "KWH"){
    fltVal = bt>0 ? BTSerial.parseFloat() : Serial.parseFloat();
    if(fltVal>0 && fltVal<1000){
      SetKwh(fltVal);
      Save();
      print8Val = bt>0 ? PRINT_BT : PRINT_USB;
    }else{
      RngErr();
    }
  }
  else if(cmdStr == "CCAL"){
    fltVal = bt>0 ? BTSerial.parseFloat() : Serial.parseFloat();
    if(fltVal>0){
      settings.currentCalibration = fltVal;
      Save();
      print8Val = bt>0 ? PRINT_BT : PRINT_USB;
    }else{
      RngErr();
    }
  }
  else if(cmdStr == "COFF"){
    fltVal = bt>0 ? BTSerial.parseFloat() : Serial.parseFloat();
    if(fltVal>=0){
      settings.currentOffset = fltVal;
      Save();
      print8Val = bt>0 ? PRINT_BT : PRINT_USB;
    }else{
      RngErr();
    }
  }
  else if(cmdStr == "VCAL"){
    fltVal = bt>0 ? BTSerial.parseFloat() : Serial.parseFloat();
    if(fltVal>0){
      settings.voltageCalibration = fltVal;
      Save();
      print8Val = bt>0 ? PRINT_BT : PRINT_USB;
    }else{
      RngErr();
    }
  }
  else if(cmdStr == "V"){
    uint16Val = bt>0 ? BTSerial.parseInt() : Serial.parseInt();
    if(uint16Val>0 && uint16Val<1000){
      settings.targetChargeVoltage = uint16Val;
      Save();
      print8Val = bt>0 ? PRINT_BT : PRINT_USB;
    }else{
      RngErr();
    }
  }
  else if(cmdStr == "VMAX"){
    uint16Val = bt>0 ? BTSerial.parseInt() : Serial.parseInt();
    if(uint16Val>0 && uint16Val<1000){
      settings.maxChargeVoltage = uint16Val;
      settings.out1Thresh = uint16Val;
      Save();
      print8Val = bt>0 ? PRINT_BT : PRINT_USB;
    }else{
      RngErr();
    }
  }
  else if(cmdStr == "AMIN"){
     uint8Val = bt>0 ? BTSerial.parseInt() : Serial.parseInt();
    if(uint8Val>0 && uint8Val<256){
      settings.minChargeAmperage = uint8Val;
      Save();
      print8Val = bt>0 ? PRINT_BT : PRINT_USB;
    }else{
      RngErr();
    }
  }
  else if(cmdStr == "AMAX"){
     uint8Val = bt>0 ? BTSerial.parseInt() : Serial.parseInt();
    if(uint8Val>0 && uint8Val<256){
      settings.maxChargeAmperage = uint8Val;
      Save();
      print8Val = bt>0 ? PRINT_BT : PRINT_USB;
    }else{
      RngErr();
    }
  }
  else if(cmdStr == "CAPAH"){
    uint8Val = bt>0 ? BTSerial.parseInt() : Serial.parseInt();
    if(uint8Val>0 && uint8Val<256){
      settings.capacity = uint8Val;
      Save();
      print8Val = bt>0 ? PRINT_BT : PRINT_USB;
    }else{
      RngErr();
    }
  }
  else if(cmdStr == "CAPKWH"){
    fltVal = bt>0 ? BTSerial.parseFloat() : Serial.parseFloat();
    if(fltVal>0 && fltVal<100){
      settings.packSizeKWH = fltVal;
      Save();
      print8Val = bt>0 ? PRINT_BT : PRINT_USB;
    }else{
      RngErr();
    }
  }
  else if(cmdStr == "DBG"){
     uint8Val = bt>0 ? BTSerial.parseInt() : Serial.parseInt();
    if(uint8Val>=0 && uint8Val<4){
      settings.debuggingLevel = uint8Val;
      Save();
      print8Val = bt>0 ? PRINT_BT : PRINT_USB;
    }else{
      RngErr();
    }
  }
  else if(cmdStr == "SS"){
     uint8Val = bt>0 ? BTSerial.parseInt() : Serial.parseInt();
    if(uint8Val>=0 && uint8Val<4){
      settings.serialOutput = uint8Val;
      //Save();
      print8Val = bt>0 ? PRINT_BT : PRINT_USB;
    }else{
      RngErr();
    }
  }
  else if(cmdStr == "OUT0"){
    SetOutTriggerMode(OUT0, bt);
    reportOutputConfig(OUT0, bt);
    print8Val = bt>0 ? PRINT_BT : PRINT_USB;
  }
  else if(cmdStr == "OUT1"){
    SetOutTriggerMode(OUT1, bt);
    reportOutputConfig(OUT1, bt);
    print8Val = bt>0 ? PRINT_BT : PRINT_USB;
  }
  else if(cmdStr == "HYS0"){
    uint16Val = bt>0 ? BTSerial.parseInt() : Serial.parseInt();
    if(uint16Val>=0 && uint16Val<256){
      settings.out0Hys = uint16Val;
      Save();
      reportOutputConfig(OUT0, bt);
      print8Val = bt>0 ? PRINT_BT : PRINT_USB;
    }else{
      RngErr();
    }
  }
  else if(cmdStr == "HYS1"){
    uint16Val = bt>0 ? BTSerial.parseInt() : Serial.parseInt();
    if(uint16Val>=0 && uint16Val<256){
      settings.out1Hys = uint16Val;
      Save();
      reportOutputConfig(OUT1, bt);
      print8Val = bt>0 ? PRINT_BT : PRINT_USB;
    }else{
      RngErr();
    }
  }
  else if(cmdStr == "SSUSB"){
    state8Val = STATE_USB;
  }
  else if(cmdStr == "SSBT"){
    state8Val = STATE_BT;
  }
  else if(cmdStr == "SAVE"){
    Save();
  }
  else if(cmdStr == "FULLRESET"){
    settings.valid = 0;
    Save();
    resetFunc();
  }
}

void SetOutTriggerMode(int output, int bt){
  //Only Apply to OUT0 and OUT1
  if(!(output==OUT0 || output == OUT1))
    return;
  strVal = bt>0 ? BTSerial.readString() : Serial.readString();
  if(strVal.startsWith("OFF"))
    uint8Val = OUTMODE_OFF;
  else if(strVal.startsWith("V<"))
    uint8Val = OUTMODE_V_LT;
  else if(strVal.startsWith("V>"))
    uint8Val = OUTMODE_V_GT;
  else if(strVal.startsWith("A<"))
    uint8Val = OUTMODE_A_LT;
  else if(strVal.startsWith("A>"))
    uint8Val = OUTMODE_A_GT;
  else if(strVal.startsWith("S<"))
    uint8Val = OUTMODE_S_LT;
  else if(strVal.startsWith("S>"))
    uint8Val = OUTMODE_S_GT;
  else if(strVal.startsWith("H<"))
    uint8Val = OUTMODE_V_LT_AH;
  else
    return; //Can't set.
  strVal = strVal.substring(2);
  if(output==OUT0){
    settings.out0Mode = uint8Val;
    settings.out0Thresh = strVal.toInt();
  }else if(output==OUT1){
    settings.out1Mode = uint8Val;
    settings.out1Thresh = strVal.toInt();
  }
  Save();
}

void reportOutputConfig(int output, int bt){
  if(bt>0){
   BTSerial.print (F("{R:")); 
   BTSerial.print ("OUT");
   BTSerial.print (output==OUT0?F("0"):F("1"));
   BTSerial.print (F(" = "));
   determineOutMode(output==OUT0?settings.out0Mode:settings.out1Mode);
   BTSerial.print (strVal);
   BTSerial.print (F(" "));
   BTSerial.print (output==OUT0?settings.out0Thresh:settings.out1Thresh);
   BTSerial.print (F(", Hys="));
   BTSerial.print (output==OUT0?settings.out0Hys:settings.out1Hys);
   BTSerial.print (F("}"));
  }
}

void checkChargingState(){
  //If the current is negative we are either charging or regenerating.
  //If we are charging we expect a relatively constant current decreasing over
  //a period of time. Ie. a small rate of change of current while charging, throughout the charging period.
  unsigned long CurrentSecs = CurrentMillis / 1000;
  if(Current < -2.0 && Current > -20 && Voltage >= settings.targetChargeVoltage){ // Charging or regenerating at less than 20 amps
    //while we are charging batteries at less than 20 amps we assume if this occurs for longer than 5 minutes
    //we are charging and not regenerating
    if(Current > -15 && (CurrentSecs - ChargeTimeRefSecs) > 30){ // have been between 0 and -15 amps for more than 5 minutes
      //At this point we assume that we have been charging for the last 5 minutes or more prior to hitting -4 amps
      ResetChargeState();
      ChargeTimeRefSecs = CurrentSecs;
    }
  }else // Normal driving conditions or we don't care. So we keep resetting the reference.
    ChargeTimeRefSecs = CurrentSecs;
}

void ResetChargeState(){
  Serial.println(F("Reset Ah & Wh"));
  settings.ampHours = 0.0;
  settings.kiloWattHours = settings.packSizeKWH;
  Save();
  print8Val = PRINT_USB;
  Serial.println(F("Done!!!")); 
}

void outputState(){
  //USB OUPUT
  if((state8Val & 1) > 0){
    Serial.print (F("JLD505: "));
    Serial.print (Voltage, 3);   
    Serial.print (F("v "));
    Serial.print (Current, 2);    
    Serial.print (F("A "));
    Serial.print (settings.ampHours, 1);    
    Serial.print (F("Ah "));
    Serial.print (Power, 1);        
    Serial.print (F("kW "));
    Serial.print (settings.kiloWattHours, 1);    
    Serial.print (F("kWh "));
    Serial.print (settings.SOC, 1);    
    Serial.print (F("% SOC "));
    Serial.print (F("OUT0"));
    Serial.print (digitalRead(OUT0)>0?F(":1 "):F(":0 "));
    Serial.print (F("OUT1"));
    Serial.print (digitalRead(OUT1)>0?F(":1"):F(":0 "));
    Serial.print (F("IN0"));
    Serial.print (digitalRead(IN0)>0?F(":1 "):F(":0 "));
    Serial.print (F("IN1"));
    Serial.print (digitalRead(IN1)>0?F(":1"):F(":0 "));
    Serial.print (F("CHG T: "));
    Serial.println (CurrentMillis / 1000 - ChargeTimeRefSecs);
  }

  if((state8Val & 1<<1) > 0){
    BTSerial.print (F("{S:")); 
    BTSerial.print (Voltage, 1);   
    BTSerial.print (F(":"));
    BTSerial.print (Current, 1);    
    BTSerial.print (F(":"));
    BTSerial.print (settings.ampHours, 1);    
    BTSerial.print (F(":"));
    BTSerial.print (Power, 1);        
    BTSerial.print (F(":"));
    BTSerial.print (settings.kiloWattHours, 1);    
    BTSerial.print (F(":"));
    BTSerial.print (settings.SOC, 1);
    BTSerial.print (F(":"));
    BTSerial.print (digitalRead(OUT0)>0?F("1"):F("0"));
    BTSerial.print (F(":"));
    BTSerial.print (digitalRead(OUT1)>0?F("1"):F("0"));
    BTSerial.print (F("}")); 
  }
  state8Val=0;
}

void USB(){
  SerialCommand(0);
}

void BT(){
  SerialCommand(1);
}

void SerialCommand(int bt)
{
  int available = bt>0 ? BTSerial.available() : Serial.available();
  if (available > 0)
  {
    Command = bt>0 ? BTSerial.peek() : Serial.peek();
    if (Command == 'z')
    {
      ResetChargeState();
    }
    if (Command == '+')
    {
      settings.voltageCalibration +=0.004;
      print8Val = bt>0 ? PRINT_BT : PRINT_USB;
      Save();   
    }
    if (Command == '-')
    {
      settings.voltageCalibration -=0.004;
      print8Val = bt>0 ? PRINT_BT : PRINT_USB;
      Save(); 
    }
    if (Command == 'p')
    {
      print8Val = bt>0 ? PRINT_BT : PRINT_USB;
    }
    if (Command == 'u')
    {
      updateTargetAV();  
    }
    if(Command == 'r'){
      resetFunc();
    }
    
    cmdStr = bt>0 ? BTSerial.readStringUntil('=') : Serial.readStringUntil('=');
    ParseCommand(bt);

    if(bt>0)
      while(BTSerial.available()>0) BTSerial.read();
    else
      while(Serial.available()>0) Serial.read();
  }
  checkRAM();
}

/**
 * Processes frames received from the ELCON CHARGER
 * uncomment #define USE_ELCON to enable this functionality
 */
#ifdef USE_ELCON
void ELCON(CAN_FRAME frame)
{
  //Message from Elcon
  if(frame.id == 0x18FF50E5){
    Serial.print(F("Elcon V: "));
    float fltVal = ((float)frame.data.byte[1] + (float)(frame.data.byte[0] * 256))/10;
    Serial.print(fltVal,2);
    Serial.print(F(" A: "));
    fltVal = ((float)frame.data.byte[3] + (float)(frame.data.byte[2] * 256))/10;
    Serial.print(fltVal,2);
    Serial.print(F(" Status: "));
    Serial.print(frame.data.byte[4]);
    Serial.print(F(" /Enable: "));
    Serial.println(digitalRead(IN0));
    
    //Serial.print(F("SEND CAN: "));
    CAN_FRAME outFrame;
    outFrame.id = 0x1806E5F4;
    outFrame.extended = 1;
    outFrame.length = 8;
    
    outFrame.data.byte[0] = highByte((int)(settings.targetChargeVoltage * 10)); // Voltage High Byte
    outFrame.data.byte[1] = lowByte((int)(settings.targetChargeVoltage * 10)); // Voltage Low Byte
    outFrame.data.byte[2] = highByte((int)(settings.maxChargeAmperage * 10)); // Current High Byte
    outFrame.data.byte[3] = lowByte((int)(settings.maxChargeAmperage * 10)); // Current Low Byte
    outFrame.data.byte[4] = digitalRead(IN0); // 
    outFrame.data.byte[5] = 0;
    outFrame.data.byte[6] = 0;
    outFrame.data.byte[7] = 0;
    INT8U result = CAN.sendFrame(outFrame);
    //Serial.print(F("Result: "));
    //Serial.println(result);
    checkRAM();
  }
}
#endif
#ifdef SCOTT_BMS
void wakeUpScott(){
    //Serial.print(F("WAKEUP SCOTT: "));
    CAN_FRAME outFrame;
    outFrame.id = 0x382;
    outFrame.extended = 0;
    outFrame.length = 4;
    
    outFrame.data.byte[0] = 0;
    outFrame.data.byte[1] = 0;
    outFrame.data.byte[2] = 1;
    outFrame.data.byte[3] = 1;
    outFrame.data.byte[4] = 0;
    outFrame.data.byte[5] = 0;
    outFrame.data.byte[6] = 0;
    outFrame.data.byte[7] = 0;
    INT8U result = CAN.sendFrame(outFrame);
    //Serial.print(F("Result: "));
    //Serial.println(result);
    checkRAM();
}

void SCOTT(CAN_FRAME frame)
{
  //Message from SCOTT

    Serial.print(F("SCOTT V: "));
    Serial.println(frame.id);
    float fltVal = ((float)frame.data.byte[1] + (float)(frame.data.byte[0] * 256))/10;
    Serial.print(fltVal,2);
    Serial.print(F(" A: "));
    fltVal = ((float)frame.data.byte[3] + (float)(frame.data.byte[2] * 256))/10;
    Serial.print(fltVal,2);
    Serial.print(F(" Status: "));
    Serial.print(frame.data.byte[4]);
    Serial.print(F(" /Enable: "));
    
    checkRAM();
  
}
#endif

void printSettings(){
  if((print8Val & 1<<1) > 0){
    BTSerial.print (F("{C:"));
    BTSerial.print (settings.ampHours, 1);
    BTSerial.print (F(":"));
    BTSerial.print (settings.kiloWattHours, 1);   
    BTSerial.print (F(":"));
    BTSerial.print (settings.voltageCalibration, 5);    
    BTSerial.print (F(":"));
    BTSerial.print (settings.currentCalibration, 2);
    BTSerial.print (F(":"));
    BTSerial.print (settings.maxChargeAmperage, 1);  
    BTSerial.print (F(":"));
    BTSerial.print (settings.maxChargeVoltage, 1);  
    BTSerial.print (F(":"));
    BTSerial.print (settings.targetChargeVoltage, 1);                
    BTSerial.print (F(":"));
    BTSerial.print (settings.minChargeAmperage, 1);              
    BTSerial.print (F(":"));
    BTSerial.print (settings.capacity, 1);  
    BTSerial.print (F(":"));
    BTSerial.print (settings.packSizeKWH, 2);    
    BTSerial.print (F("}"));
  }
  
  if((print8Val & 1) > 0){
    Serial.println (F("-"));
    Serial.print (F("AH: "));
    Serial.println (settings.ampHours);
    Serial.print (F("KWH: "));
    Serial.println (settings.kiloWattHours);   
    Serial.print (F("VCAL: "));
    Serial.println (settings.voltageCalibration, 5);    
    Serial.print (F("CCAL: "));
    Serial.println (settings.currentCalibration, 2);
    Serial.print (F("AMIN "));
    Serial.println (settings.minChargeAmperage, 1);   
    Serial.print (F("AMAX "));
    Serial.println (settings.maxChargeAmperage, 1);  
    Serial.print (F("V "));
    Serial.println (settings.targetChargeVoltage, 1);  
    Serial.print (F("VMAX "));
    Serial.println (settings.maxChargeVoltage, 1);                           
    Serial.print (F("CAPAH "));
    Serial.println (settings.capacity, 1);  
    Serial.print (F("CAPKWH: "));
    Serial.println (settings.packSizeKWH, 2);  
    Serial.print (F("COFF: "));
    Serial.println (settings.currentOffset, 2);  
    Serial.print (F("DBG "));
    Serial.println (settings.debuggingLevel, 1);  
    Serial.print (F("OUT0 Mode "));
    determineOutMode(settings.out0Mode);
    Serial.println (strVal);
    Serial.print (F("OUT0 Thr "));
    Serial.println (settings.out0Thresh);
    Serial.print (F("OUT0 Hys "));
    Serial.println (settings.out0Hys);
    Serial.print (F("OUT1 Mode "));
    determineOutMode(settings.out1Mode);
    Serial.println (strVal);
    Serial.print (F("OUT1 Thr "));
    Serial.println (settings.out1Thresh);
    Serial.print (F("OUT1 Hys "));
    Serial.println (settings.out1Hys);
    Serial.println (F("-")); 
  }

  print8Val = 0;
}

void determineOutMode(int mode){
  switch(mode){
      case OUTMODE_V_LT : strVal = F("V LT"); break;
      case OUTMODE_V_GT : strVal = F("V GT"); break;
      case OUTMODE_A_LT : strVal = F("A LT"); break;
      case OUTMODE_A_GT : strVal = F("A FT"); break;
      case OUTMODE_S_LT : strVal = F("S LT"); break;
      case OUTMODE_S_GT : strVal = F("S GT"); break;
      case OUTMODE_V_LT_AH : strVal = F("VLT & AH"); break;
      default: strVal = F("INACTIVE");break;
  }
}

void timestamp()
{
	int milliseconds = (int) (millis()/1) %1000 ;
	int seconds = (int) (millis() / 1000) % 60 ;
	int minutes = (int) ((millis() / (1000*60)) % 60);
	int hours   = (int) ((millis() / (1000*60*60)) % 24);
            
	Serial.print(F(" Time:"));
	Serial.print(hours);
	Serial.print(F(":"));
	Serial.print(minutes);
	Serial.print(F(":"));
	Serial.print(seconds);
	Serial.print(F("."));
	Serial.println(milliseconds);    
}

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

inline void checkRAM()
{
	int freeram = freeRam();
	if (freeram < lowestFreeRAM) lowestFreeRAM = freeram;
}
