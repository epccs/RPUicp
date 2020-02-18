#ifndef Rpu_Mgr_h
#define Rpu_Mgr_h

extern uint8_t twi_errorCode;

extern void i2c_ping(void);
extern uint8_t i2c_set_Rpu_shutdown(void);
extern uint8_t i2c_detect_Rpu_shutdown(void);
extern char i2c_get_Rpu_address(void);
extern int i2c_get_adc_from_manager(uint8_t);
extern uint8_t i2c_read_status(void);
extern uint8_t i2c_uint8_access_cmd(uint8_t, uint8_t);
extern unsigned long i2c_ul_access_cmd(uint8_t, unsigned long);
extern int i2c_int_access_cmd(uint8_t, int);

// values used for i2c_get_analogRead_from_manager
#define ALT_I 0
#define ALT_V 1
#define PWR_I 6
#define PWR_V 7

// values used for i2c_uint8_access_cmd
#define DAYNIGHT_STATE 23

// values used for i2c_ul_access_cmd
#define CHARGE_BATTERY_ABSORPTION 20
#define EVENING_DEBOUNCE 52
#define MORNING_DEBOUNCE 53
#define DAYNIGHT_TIMER 54

// values used for i2c_int_access_cmd
#define CHARGE_BATTERY_START 18
#define CHARGE_BATTERY_STOP 19
#define MORNING_THRESHOLD 21
#define EVENING_THRESHOLD 22


#endif // Rpu_Mgr_h
