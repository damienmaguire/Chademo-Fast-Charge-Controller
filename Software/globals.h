#ifndef GLOBAL_H_
#define GLOBAL_H_

//set the proper digital pins for these
#define IN0		4
#define IN1		7
#define OUT0	5
#define OUT1	6

typedef struct
{
	uint8_t valid; //a token to store EEPROM version and validity. If it matches expected value then EEPROM is not reset to defaults //0
	float ampHours; //floats are 4 bytes //1
	float kiloWattHours; //5
	float packSizeKWH; //9
	float voltageCalibration; //13
	float currentCalibration; //17
	uint16_t maxChargeVoltage; //21
	uint16_t targetChargeVoltage; //23
	uint8_t maxChargeAmperage; //25
	uint8_t minChargeAmperage; //26
    uint8_t capacity; //27
    uint8_t SOC; //28
	uint8_t debuggingLevel; //29
    uint8_t serialOutput; //30
    uint8_t out0Mode; //31
    uint16_t out0Thresh; //32
    uint16_t out0Hys; //34
    uint8_t out1Mode; //36
    uint16_t out1Thresh; //37
    uint16_t out1Hys; //39
    float currentOffset; //43
} EESettings;

extern EESettings settings;
extern float Voltage;
extern float Current;
extern unsigned long CurrentMillis;
extern int Count;

#endif
