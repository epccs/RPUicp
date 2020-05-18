#ifndef I2C_cmds_H
#define I2C_cmds_H

#define I2C_BUFFER_LENGTH 32

#define GROUP  4
#define MGR_CMDS  8

extern uint8_t i2c0Buffer[I2C_BUFFER_LENGTH];
extern uint8_t i2c0BufferLength;

extern void receive_i2c_event(uint8_t*, uint8_t);
extern void transmit_i2c_event(void);

// Prototypes for point 2 multipoint commands
extern void fnMgrAddr(uint8_t*); // 0 for I2C
extern void fnMgrAddrQuietly(uint8_t*); // 0 for SMBus
// not used //1
extern void fnBootldAddr(uint8_t*); // 2
extern void fnArduinMode(uint8_t*);  // 3
// not used  // 4
// not used  // 5
extern void fnStatus(uint8_t*); // 6
// not used // 7

// Prototypes for PV and Battery Management
extern void fnBatteryMgr(uint8_t*); // 16 
// not used  // 17
extern void fnBatChrgLowLim(uint8_t*); // 18
extern void fnBatChrgHighLim(uint8_t*); // 19
extern void fnRdBatChrgTime(uint8_t*); // 20
extern void fnMorningThreshold(uint8_t*); // 21
extern void fnEveningThreshold(uint8_t*); // 22
extern void fnDayNightState(uint8_t*); // 23

// Prototypes for Analog commands
extern void fnAnalogRead(uint8_t*); //32
extern void fnCalibrationRead(uint8_t*); //33
// not used //34
// not used //35
extern void fnRdTimedAccum(uint8_t*); //36
// not used  //37
extern void fnReferance(uint8_t*); //38
// not used  //39


// Prototypes for test mode commands
extern void fnStartTestMode(uint8_t*); //48
extern void fnEndTestMode(uint8_t*); //49
extern void fnRdXcvrCntlInTestMode(uint8_t*); //50
extern void fnWtXcvrCntlInTestMode(uint8_t*); //51
extern void fnMorningDebounce(uint8_t*); //52
extern void fnEveningDebounce(uint8_t*); //53
extern void fnDayNightTimer(uint8_t*); //54
// not used  //55

/* Dummy function */
extern  void fnNull(uint8_t*);

#endif // I2C_cmds_H 
