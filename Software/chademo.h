#ifndef CHADEMO_H_
#define CHADEMO_H_
#include <Arduino.h>
#include "globals.h"
#include "mcp_can.h"

enum CHADEMOSTATE 
{
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
};

typedef struct 
{
	uint8_t supportWeldCheck;
	uint16_t availVoltage;
	uint8_t availCurrent;
	uint16_t thresholdVoltage; //evse calculates this. It is the voltage at which it'll abort charging to save the battery pack in case we asked for something stupid
} EVSE_PARAMS;

typedef struct 
{
	uint16_t presentVoltage;
	uint8_t presentCurrent;
	uint8_t status;
	uint16_t remainingChargeSeconds;
} EVSE_STATUS;

typedef struct 
{
	uint16_t targetVoltage; //what voltage we want the EVSE to put out
	uint8_t targetCurrent; //what current we'd like the EVSE to provide
	uint8_t remainingKWH; //report # of KWh in the battery pack (charge level)
	uint8_t battOverVolt : 1; //we signal that battery or a cell is too high of a voltage
	uint8_t battUnderVolt : 1; //we signal that battery is too low
	uint8_t currDeviation : 1; //we signal that measured current is not the same as EVSE is reporting
	uint8_t battOverTemp : 1; //we signal that battery is too hot
	uint8_t voltDeviation : 1; //we signal that we measure a different voltage than EVSE reports
	uint8_t chargingEnabled : 1; //ask EVSE to enable charging
	uint8_t notParked : 1; //advise EVSE that we're not in park.
	uint8_t chargingFault : 1; //signal EVSE that we found a fault
	uint8_t contactorOpen : 1; //tell EVSE whether we've closed the charging contactor 
	uint8_t stopRequest : 1; //request that the charger cease operation before we really get going
} CARSIDE_STATUS;

//The IDs for chademo comm - both carside and EVSE side so we know what to listen for
//as well.
#define CARSIDE_BATT_ID			0x100
#define CARSIDE_CHARGETIME_ID	0x101
#define CARSIDE_CONTROL_ID		0x102

#define EVSE_PARAMS_ID			0x108
#define EVSE_STATUS_ID			0x109

#define CARSIDE_FAULT_OVERV		1 //over voltage
#define CARSIDE_FAULT_UNDERV	2 //Under voltage
#define CARSIDE_FAULT_CURR		4 //current mismatch
#define CARSIDE_FAULT_OVERT		8 //over temperature
#define CARSIDE_FAULT_VOLTM		16 //voltage mismatch

#define CARSIDE_STATUS_CHARGE	1 //charging enabled
#define CARSIDE_STATUS_NOTPARK	2 //shifter not in safe state
#define CARSIDE_STATUS_MALFUN	4 //vehicle did something dumb
#define CARSIDE_STATUS_CONTOP	8 //main contactor open
#define CARSIDE_STATUS_CHSTOP	16 //charger stop before even charging

#define EVSE_STATUS_CHARGE		1 //charger is active
#define EVSE_STATUS_ERR			2 //something went wrong
#define EVSE_STATUS_CONNLOCK	4 //connector is currently locked
#define EVSE_STATUS_INCOMPAT	8 //parameters between vehicle and charger not compatible
#define EVSE_STATUS_BATTERR		16 //something wrong with battery?!
#define EVSE_STATUS_STOPPED		32 //charger is stopped

class CHADEMO
{
public:
	CHADEMO();
	void setDelayedState(int newstate, uint16_t delayTime);
	CHADEMOSTATE getState();
	void setTargetAmperage(uint8_t t_amp);
	void setTargetVoltage(uint16_t t_volt);
	void loop();
	void doProcessing();
	void handleCANFrame(CAN_FRAME &frame);
	void setChargingFault();
	void setBattOverTemp();

	//these need to be accessed quickly in tight spots so they're public in an attempt at efficiency
	uint8_t bChademoMode; //accessed but not modified in ISR so it should be OK non-volatile
	uint8_t bChademoSendRequests; //should we be sending periodic status updates?
	volatile uint8_t bChademoRequest;  //is it time to send one of those updates?

protected:
private:
	uint8_t bStartedCharge; //we have started a charge since the plug was inserted. Prevents attempts to restart charging if it stopped previously
	uint8_t bChademo10Protocol; //can we use 1.0 protocol?
	//target values are what we send with periodic frames and can be changed.
	uint8_t askingAmps; //how many amps to ask for. Trends toward targetAmperage
	uint8_t bListenEVSEStatus; //should we pay attention to stop requests and such yet?
	uint8_t bDoMismatchChecks; //should we be checking for voltage and current mismatches?
	uint8_t vMismatchCount; //count # of consecutive voltage mismatches. Don't trigger until we get enough
	uint8_t cMismatchCount; //same but for current
	uint8_t vCapCount; //# of EVSE voltage capacity checks that have failed in a row.
	uint8_t vOverFault; //over volt fault counter like above.
	uint8_t faultCount; //force faults to count up a bit before we actually fault.
	uint32_t mismatchStart;
	const uint16_t mismatchDelay = 10000; //don't start mismatch checks for 10 seconds
	uint32_t stateMilli;
	uint16_t stateDelay;
	uint32_t insertionTime;
	uint32_t lastCommTime;
	const uint16_t lastCommTimeout = 1000; //allow up to 1 second of comm fault before getting angry

	CHADEMOSTATE chademoState;
	CHADEMOSTATE stateHolder;
	EVSE_PARAMS evse_params;
	EVSE_STATUS evse_status;
	CARSIDE_STATUS carStatus;

	void sendCANStatus();
	void sendCANBattSpecs();
	void sendCANChargingTime();
};

extern CHADEMO chademo;

#endif
