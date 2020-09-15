#ifndef GLOBAL_H_
#define GLOBAL_H_

//set the proper digital pins for these
#define IN1   6//'start-of-charge' signal, CHAdeMO pin 2
#define IN2		7//charging ready signal, CHAdeMO pin 10

#define OUT1	48//'start permission' signal, CHAdeMO pin 4
#define OUT2  49//contactor relay control
#define OUT3  50//HV request

typedef struct
{
  uint8_t valid; //a token to store EEPROM version and validity. If it matches expected value then EEPROM is not reset to defaults //0
  float ampHours; //floats are 4 bytes //1
  float kiloWattHours; //5
  float packSizeKWH; //9
  uint16_t maxChargeVoltage; //21
  uint16_t targetChargeVoltage; //23
  uint8_t maxChargeAmperage; //25
  uint8_t minChargeAmperage; //26
  uint8_t capacity; //27
  uint8_t debuggingLevel; //29
} EESettings;

extern EESettings settings;
extern float Voltage;
extern float Current;
extern unsigned long CurrentMillis;
extern int Count;

#endif
