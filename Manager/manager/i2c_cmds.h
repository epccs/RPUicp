#ifndef I2C_cmds_H
#define I2C_cmds_H

#define I2C_BUFFER_LENGTH 32

#define GROUP  4
#define MGR_CMDS  8

extern uint8_t i2c0Buffer[I2C_BUFFER_LENGTH];
extern uint8_t i2c0BufferLength;

extern void receive_i2c_event(uint8_t*, int);
extern void transmit_i2c_event(void);

// Prototypes for point 2 multipoint commands
extern void fnRdMgrAddr(uint8_t*); // 0
extern void fnRdMgrAddrQuietly(uint8_t*); // 0 for SMBus
// 1 not used
extern void fnRdBootldAddr(uint8_t*); // 2
extern void fnWtBootldAddr(uint8_t*); // 3
extern void fnRdShtdnDtct(uint8_t*); // 4
extern void fnWtShtdnDtct(uint8_t*); // 5
extern void fnRdStatus(uint8_t*); // 6
extern void fnWtStatus(uint8_t*); // 7

// Prototypes for point 2 point commands
extern void fnWtArduinMode(uint8_t*); // 16
extern void fnRdArduinMode(uint8_t*); // 17
extern void fnBatStartChrg(uint8_t*); // 18
extern void fnBatDoneChrg(uint8_t*); // 19
extern void fnRdBatChrgTime(uint8_t*); // 20
extern void fnMorningThreshold(uint8_t*); // 21
extern void fnEveningThreshold(uint8_t*); // 22
extern void fnDayNightState(uint8_t*); // 23

// Prototypes for Power Management commands
extern void fnAnalogRead(uint8_t*); //1
// todo calibrationRead
// todo calibrationWrite
// not used
extern void fnRdTimedAccum(uint8_t*);
// not used 
extern void fnAnalogRefExternAVCC(uint8_t*);
extern void fnAnalogRefIntern1V1(uint8_t*);


// Prototypes for test mode commands
extern void fnStartTestMode(uint8_t*);
extern void fnEndTestMode(uint8_t*);
extern void fnRdXcvrCntlInTestMode(uint8_t*);
extern void fnWtXcvrCntlInTestMode(uint8_t*);
extern void fnMorningDebounce(uint8_t*);
extern void fnEveningDebounce(uint8_t*);
extern void fnDayNightTimer(uint8_t*);

/* Dummy function */
extern  void fnNull(uint8_t*);

#endif // I2C_cmds_H 
