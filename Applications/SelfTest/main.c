/* 
Gravimetric SelfTest
Copyright (C) 2019 Ronald Sutherland

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES 
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF 
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE 
FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY 
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, 
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, 
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

https://en.wikipedia.org/wiki/BSD_licenses#0-clause_license_(%22Zero_Clause_BSD%22)
*/ 

#include <stdbool.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/io.h>
#include "../lib/timers_bsd.h"
#include "../lib/uart0_bsd.h"
#include "../lib/twi0_bsd.h"
#include "../lib/twi1_bsd.h"
#include "../lib/adc_bsd.h"
#include "../lib/rpu_mgr.h"
#include "../lib/io_enum_bsd.h"
#include "../Adc/references.h"

#define BLINK_DELAY 1000UL

// Save the Value of the References for ADC converter 
// measure AVCC and put it hear in uV
#define REF_EXTERN_AVCC 4959800UL

// ICP3 has 100 Ohm and R1 in parallel
#define ICP3PARL100_TERM 50.0
#define ICP4_TERM 100.0
#define ICP1_TERM 100.0

static unsigned long blink_started_at;
static unsigned long blink_delay;
static char rpu_addr;
static uint8_t passing;
static uint8_t test_mode_clean;
static uint8_t delayed_outputs;
static uint8_t delayed_data[8];

void setup_pins_off(void)
{
    // Turn Off Curr Sources
    ioDir(MCU_IO_CS0_EN,DIRECTION_OUTPUT);
    ioWrite(MCU_IO_CS0_EN,LOGIC_LEVEL_LOW);
    ioDir(MCU_IO_CS1_EN,DIRECTION_OUTPUT);
    ioWrite(MCU_IO_CS1_EN,LOGIC_LEVEL_LOW);
    ioDir(MCU_IO_CS2_EN,DIRECTION_OUTPUT);
    ioWrite(MCU_IO_CS2_EN,LOGIC_LEVEL_LOW);
    ioDir(MCU_IO_CS3_EN,DIRECTION_OUTPUT);
    ioWrite(MCU_IO_CS3_EN,LOGIC_LEVEL_LOW);
    ioDir(MCU_IO_CS4_EN,DIRECTION_OUTPUT);
    ioWrite(MCU_IO_CS4_EN,LOGIC_LEVEL_LOW);
    ioDir(MCU_IO_CS_ICP1,DIRECTION_OUTPUT);
    ioWrite(MCU_IO_CS_ICP1,LOGIC_LEVEL_LOW);
    ioDir(MCU_IO_CS_ICP3,DIRECTION_OUTPUT);
    ioWrite(MCU_IO_CS_ICP3,LOGIC_LEVEL_LOW);
    ioDir(MCU_IO_CS_ICP4,DIRECTION_OUTPUT);
    ioWrite(MCU_IO_CS_ICP4,LOGIC_LEVEL_LOW);
    ioDir(MCU_IO_CS_FAST,DIRECTION_OUTPUT);
    ioWrite(MCU_IO_CS_FAST,LOGIC_LEVEL_LOW);
    ioDir(MCU_IO_CS_DIVERSION,DIRECTION_OUTPUT);
    ioWrite(MCU_IO_CS_DIVERSION,LOGIC_LEVEL_LOW);

    // DIO and Analog
    ioDir(MCU_IO_RX1,DIRECTION_INPUT);
    ioWrite(MCU_IO_RX1,LOGIC_LEVEL_LOW);
    ioDir(MCU_IO_TX1,DIRECTION_INPUT);
    ioWrite(MCU_IO_TX1,LOGIC_LEVEL_LOW);
    ioDir(MCU_IO_RX2,DIRECTION_INPUT);
    ioWrite(MCU_IO_RX2,LOGIC_LEVEL_LOW);
    ioDir(MCU_IO_TX2,DIRECTION_INPUT);
    ioWrite(MCU_IO_TX2,LOGIC_LEVEL_LOW);
    ioDir(MCU_IO_ADC0,DIRECTION_INPUT);
    ioDir(MCU_IO_ADC1,DIRECTION_INPUT);
    ioDir(MCU_IO_ADC2,DIRECTION_INPUT);
    ioDir(MCU_IO_ADC3,DIRECTION_INPUT);
    ioDir(MCU_IO_ADC4,DIRECTION_INPUT);
    ioDir(MCU_IO_ADC5,DIRECTION_INPUT);
    ioDir(MCU_IO_ADC6,DIRECTION_INPUT);
    ioDir(MCU_IO_ADC7,DIRECTION_INPUT);
    
    // R-Pi power control
    //ioDir(SHLD_VIN_EN,DIRECTION_OUTPUT);
    //ioWrite(SHLD_VIN_EN,LOGIC_LEVEL_LOW);
    
    // Alternate power control
    //ioDir(ALT_EN,DIRECTION_OUTPUT);
    //ioWrite(ALT_EN,LOGIC_LEVEL_LOW);
    
    // SPI needs a loopback on the R-Pi connector between PI_MISO and PI_MOSI
    ioDir(MCU_IO_MISO,DIRECTION_INPUT);
    ioWrite(MCU_IO_MISO,LOGIC_LEVEL_HIGH); //a weak pull up will turn off the buffer that would otherwise pull down ICP3/MOSI
}


void setup(void) 
{
    setup_pins_off();

    // Initialize Timers, ADC, and clear bootloader, Arduino does these with init() in wiring.c
    initTimers(); //Timer0 Fast PWM mode, Timer1 & Timer2 Phase Correct PWM mode.
    init_ADC_single_conversion(EXTERNAL_AVCC); // warning AREF must not be connected to anything
    uart0_init(0,0); // bootloader may have the UART enabled, a zero baudrate will disconnect it.

    /* Initialize UART to 38.4kbps, it returns a pointer to FILE so redirect of stdin and stdout works*/
    stderr = stdout = stdin = uart0_init(38400UL, UART0_RX_REPLACE_CR_WITH_NL);

    /* Initialize I2C*/
    twi0_init(100000UL, TWI0_PINS_PULLUP);
    twi1_init(100000UL, TWI1_PINS_PULLUP);

    // Enable global interrupts to start TIMER0 and UART
    sei(); 

    blink_started_at = milliseconds();

    rpu_addr = i2c_get_Rpu_address();
    blink_delay = BLINK_DELAY;

    // blink fast if a default address from RPU manager not found
    if (rpu_addr == 0)
    {
        rpu_addr = '0';
        blink_delay = BLINK_DELAY/4;
    }
    
    // set the referances and save them in EEPROM
    ref_extern_avcc_uV = REF_EXTERN_AVCC;
}

void smbus_address(void)
{
    uint8_t smbus_address = 0x2A;
    uint8_t length = 2;
    uint8_t txBuffer[2] = {0x00,0x00}; //comand 0x00 should Read the mulit-drop bus addr;
    uint8_t twi1_returnCode = twi1_masterBlockingWrite(smbus_address, txBuffer, length, TWI1_PROTOCALL_STOP); 
    if (twi1_returnCode != 0)
    {
        passing = 0; 
        printf_P(PSTR(">>> SMBus write failed, twi1_returnCode: %d\r\n"), twi1_returnCode);
        return;
    }
    
    // read_i2c_block_data sends a command byte and then a repeated start followed by reading the data 
    uint8_t cmd_length = 1; // one byte command is sent befor read with the read_i2c_block_data
    txBuffer[0] = 0x00; //comand 0x00 matches the above write command
    twi1_returnCode = twi1_masterBlockingWrite(smbus_address, txBuffer, cmd_length, TWI1_PROTOCALL_REPEATEDSTART); 
    if (twi1_returnCode != 0)
    {
        passing = 0; 
        printf_P(PSTR(">>> SMBus read fail to send cmd byte, twi1_returnCode: %d\r\n"), twi1_returnCode);
        // bus is set for repeated start so it is locked until a Stop is done.
        twi1_masterBlockingWrite(smbus_address, txBuffer, 0, TWI1_PROTOCALL_STOP); 
    }
    uint8_t rxBuffer[2] = {0x00,0x00};
    uint8_t bytes_read = twi1_masterBlockingRead(smbus_address, rxBuffer, length, TWI1_PROTOCALL_STOP);
    if ( bytes_read != length )
    {
        passing = 0; 
        printf_P(PSTR(">>> SMBus read missing %d bytes \r\n"), (length-bytes_read) );
    }
    if ( (rxBuffer[0] == 0x0) && (rxBuffer[1] == '1') )
    {
        printf_P(PSTR("SMBUS cmd %d provided address %d from manager\r\n"), rxBuffer[0], rxBuffer[1]);
    } 
    else  
    { 
        passing = 0; 
        printf_P(PSTR(">>> SMBUS wrong addr %d for cmd %d\r\n"), rxBuffer[1], rxBuffer[0]);
    }
}

void print_twi0_read_status(TWI0_RD_STAT_t i2c_status)
{
    printf_P(PSTR(">>> I2C0 read failed status is "));
    if (i2c_status == TWI0_RD_STAT_ADDR_NACK)
    {
        printf_P(PSTR("ADDR_NACK"));
    }
    else if (i2c_status == TWI0_RD_STAT_DATA_NACK)
    {
        printf_P(PSTR("DATA_NACK"));
    }
    else if (i2c_status == TWI0_RD_STAT_ILLEGAL)
    {
        printf_P(PSTR("ILLEGAL"));
    }
    else if (i2c_status == TWI0_RD_STAT_BUSY)
    {
        printf_P(PSTR("BUSY"));
    }
    else // (i2c_status == TWI0_RD_STAT_SUCCESS)
    {
        printf_P(PSTR("SUCCESS"));
    }
    printf_P(PSTR("\r\n"));
}

void i2c_shutdown(void)
{
    uint8_t i2c_address = 0x29;
    uint8_t length = 2;
    uint8_t txBuffer[2] = {0x05,0x01};
    uint8_t twi_returnCode = twi0_masterBlockingWrite(i2c_address, txBuffer, length, TWI0_PROTOCALL_REPEATEDSTART); 
    if (twi_returnCode != 0)
    {
        passing = 0; 
        printf_P(PSTR(">>> I2C0 cmd 5 write fail, twi_returnCode: %d\r\n"), twi_returnCode);
        return;
    }
    uint8_t rxBuffer[2] = {0x00,0x00};
    uint8_t bytes_read = twi0_masterBlockingRead(i2c_address, rxBuffer, length, TWI0_PROTOCALL_STOP);
    if ( bytes_read == 0 )
    {
        passing = 0; 
        print_twi0_read_status(twi0_masterAsyncRead_status());
        return;
    }
    if ( (txBuffer[0] == rxBuffer[0]) && (txBuffer[1] == rxBuffer[1]) )
    {
        printf_P(PSTR("I2C0 Shutdown cmd is clean {%d, %d}\r\n"), rxBuffer[0], rxBuffer[1]);
    } 
    else  
    { 
        passing = 0; 
        printf_P(PSTR(">>> I2C0 Shutdown cmd echo bad {%d, %d}\r\n"), rxBuffer[0], rxBuffer[1]);
    }
}

void i2c_shutdown_detect(void)
{
    uint8_t i2c_address = 0x29;
    uint8_t length = 2;
    uint8_t txBuffer[2] = {0x04, 0xFF}; //comand 0x04 will return the value 0x01 (0xff is a byte for the ISR to replace)
    uint8_t twi_returnCode = twi0_masterBlockingWrite(i2c_address, txBuffer, length, TWI0_PROTOCALL_REPEATEDSTART);
    if (twi_returnCode != 0)
    {
        passing = 0; 
        printf_P(PSTR(">>> I2C0 cmd 4 write fail, twi_returnCode: %d\r\n"), twi_returnCode);
    }
    uint8_t rxBuffer[2] = {0x00,0x00};
    uint8_t bytes_read = twi0_masterBlockingRead(i2c_address, rxBuffer, length, TWI0_PROTOCALL_STOP);
    if ( bytes_read == 0 )
    {
        passing = 0; 
        print_twi0_read_status(twi0_masterAsyncRead_status());
        return;
    }
    if ( (txBuffer[0] == rxBuffer[0]) && (0x1 == rxBuffer[1]) )
    {
        printf_P(PSTR("I2C0 Shutdown Detect cmd is clean {%d, %d}\r\n"), rxBuffer[0], rxBuffer[1]);
    } 
    else  
    { 
        passing = 0; 
        printf_P(PSTR(">>> I2C0 Shutdown Detect cmd echo bad {%d, %d}\r\n"), rxBuffer[0], rxBuffer[1]);
    }
}

void i2c_testmode_start(void)
{
    uint8_t i2c_address = 0x29;
    uint8_t length = 2;
    uint8_t txBuffer[2] = {0x30, 0x01}; // preserve the trancever control bits
    uint8_t twi_returnCode = twi0_masterBlockingWrite(i2c_address, txBuffer, length, TWI0_PROTOCALL_REPEATEDSTART);
    if (twi_returnCode != 0)
    {
        passing = 0; 
        printf_P(PSTR(">>> I2C0 cmd 48 write fail, twi_returnCode: %d\r\n"), twi_returnCode);
    }
    uint8_t rxBuffer[2] = {0x00,0x00};
    uint8_t bytes_read = twi0_masterBlockingRead(i2c_address, rxBuffer, length, TWI0_PROTOCALL_STOP);
    if ( bytes_read == 0 )
    {
        passing = 0; 
        print_twi0_read_status(twi0_masterAsyncRead_status());
        return;
    }
    if ( (txBuffer[0] == rxBuffer[0]) && (txBuffer[1]  == rxBuffer[1]) )
    {
        test_mode_clean = 1;
        // delay print until the UART can be used
    } 
    else  
    { 
        passing = 0; 
        printf_P(PSTR(">>> I2C0 Start Test Mode cmd echo bad {%d, %d}\r\n"), rxBuffer[0], rxBuffer[1]);
    }
}

void i2c_testmode_end(void)
{
    uint8_t i2c_address = 0x29;
    uint8_t length = 2;
    uint8_t txBuffer[2] = {0x31, 0x01}; // recover trancever control bits and report values in data[1] byte
    uint8_t twi_returnCode = twi0_masterBlockingWrite(i2c_address, txBuffer, length, TWI0_PROTOCALL_REPEATEDSTART);
    if ( test_mode_clean )
    {
        printf_P(PSTR("I2C0 Start Test Mode cmd was clean {48, 1}\r\n"));
        test_mode_clean = 0;
    }
    if (twi_returnCode != 0)
    {
        passing = 0; 
        printf_P(PSTR(">>> I2C0 cmd 49 write fail, twi_returnCode: %d\r\n"), twi_returnCode);
    }
    uint8_t rxBuffer[2] = {0x00,0x00};
    uint8_t bytes_read = twi0_masterBlockingRead(i2c_address, rxBuffer, length, TWI0_PROTOCALL_STOP);
    if ( bytes_read == 0 )
    {
        passing = 0; 
        print_twi0_read_status(twi0_masterAsyncRead_status());
        return;
    }
    if ( (txBuffer[0] == rxBuffer[0]) && (0xD5 == rxBuffer[1]) )
    {
        printf_P(PSTR("I2C0 End Test Mode hex is Xcvr cntl bits {%d, 0x%X}\r\n"), rxBuffer[0], rxBuffer[1]);
    } 
    else  
    { 
        passing = 0; 
        printf_P(PSTR(">>> I2C0 problem /w End Test Mode {%d, %d}\r\n"), rxBuffer[0], rxBuffer[1]);
    }
}

void i2c_testmode_test_xcvrbits(uint8_t xcvrbits)
{
    // do not use UART durring testmode
    if ( test_mode_clean )
    {
        uint8_t i2c_address = 0x29;
        uint8_t length = 2;
        uint8_t txBuffer[2] = {0x32, 0x01};

        uint8_t twi_returnCode = twi0_masterBlockingWrite(i2c_address, txBuffer, length, TWI0_PROTOCALL_REPEATEDSTART);
        if (twi_returnCode != 0)
        {
            passing = 0;
            delayed_outputs |= (1<<4); // >>> I2C cmd 50 write fail, twi_returnCode: %d\r\n
            delayed_data[4] =  twi_returnCode;
        }
        uint8_t rxBuffer[2] = {0x00,0x00};
        uint8_t bytes_read = twi0_masterBlockingRead(i2c_address, rxBuffer, length, TWI0_PROTOCALL_STOP);
        if ( bytes_read == 0 )
        {
            passing = 0; 
            delayed_outputs |= (1<<5); // >>> I2C read missing %d bytes \r\n
            delayed_data[5] =  twi0_masterAsyncRead_status();
        }
        if ( (txBuffer[0] == rxBuffer[0]) && (xcvrbits == rxBuffer[1]) )
        { //0xe2 is 0b11100010
            // HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE
            delayed_outputs |= (1<<6); // Testmode: read  Xcvr cntl bits {50, 0x%X}\r\n
            delayed_data[6] =  rxBuffer[1];
        } 
        else  
        { 
            passing = 0;
            delayed_outputs |= (1<<7); // >>> Xcvr cntl bits should be %x but report was %x
            delayed_data[7] =  rxBuffer[1]; // used with second format output
        }
    }
}

void i2c_testmode_set_xcvrbits(uint8_t xcvrbits)
{
    // do not use UART durring testmode
    if ( test_mode_clean )
    {
        uint8_t i2c_address = 0x29;
        uint8_t length = 2;
        uint8_t txBuffer[2];
        txBuffer[0] = 0x33;
        txBuffer[1] = xcvrbits;
        uint8_t twi_returnCode = twi0_masterBlockingWrite(i2c_address, txBuffer, length, TWI0_PROTOCALL_REPEATEDSTART);
        if (twi_returnCode != 0)
        {
            passing = 0;
            delayed_outputs |= (1<<0); // >>> I2C cmd 51 write fail, twi_returnCode: %d\r\n
            delayed_data[0] =  twi_returnCode;
        }
        uint8_t rxBuffer[2] = {0x00,0x00};
        uint8_t bytes_read = twi0_masterBlockingRead(i2c_address, rxBuffer, length, TWI0_PROTOCALL_STOP);
        if ( bytes_read != length )
        {
            passing = 0; 
            delayed_outputs |= (1<<1); // >>> I2C read missing %d bytes \r\n
            delayed_data[1] =  twi0_masterAsyncRead_status();
        }
        if ( (txBuffer[0] == rxBuffer[0]) && (xcvrbits == rxBuffer[1]) )
        { //0xe2 is 0b11100010
            // HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE
            delayed_outputs |= (1<<2); // Testmode: set  Xcvr cntl bits {51, 0x%X}\r\n
            delayed_data[2] =  rxBuffer[1];
        } 
        else  
        { 
            passing = 0;
            delayed_outputs |= (1<<3); // >>> Xcvr cntl bits set as %x but report was %x
            delayed_data[3] =  rxBuffer[1]; // used with second format output
        }
    }
}

void test(void)
{
    // Info from some Predefined Macros
    printf_P(PSTR("Gravimetric Self Test date: %s\r\n"), __DATE__);
    printf_P(PSTR("avr-gcc --version: %s\r\n"),__VERSION__);
    
    // I2C is used to read serial bus manager address 
    if (rpu_addr == '1')
    {
        printf_P(PSTR("I2C provided address 0x31 from RPUadpt serial bus manager\r\n"));
    } 
    else  
    { 
        passing = 0; 
        printf_P(PSTR(">>> I2C failed, or address not 0x31 from serial bus manager\r\n"));
        return;
    }

    if (ref_extern_avcc_uV > 5100000UL) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> REF_EXTERN_AVCC is to high.\r\n"));
        return;
    }
    if (ref_extern_avcc_uV < 4900000UL) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> REF_EXTERN_AVCC is to low.\r\n"));
        return;
    }

    // With current sources off measure input current
    _delay_ms(1000) ; // busy-wait to let the 1uF settle
    
    // Input voltage
    int adc_pwr_v = i2c_get_adc_from_manager(PWR_V);
    printf_P(PSTR("adc reading for PWR_V: %d int\r\n"), adc_pwr_v);
    float input_v = adc_pwr_v*((ref_extern_avcc_uV/1.0E6)/1024.0)*(115.8/15.8);
    printf_P(PSTR("PWR at: %1.3f V\r\n"), input_v);
    if (input_v > 14.0) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> Input voltage is to high.\r\n"));
        return;
    }
    if (input_v < 12.0) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> Input voltage is to low.\r\n"));
        return;
    }

    //current sources are off, measure ADC0..ADC3 
    float adc0_v = adcSingle(ADC0)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    printf_P(PSTR("ADC0 at ICP3&4 TERM /W all CS off: %1.3f V\r\n"), adc0_v);
    float adc1_v = adcSingle(ADC1)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    printf_P(PSTR("ADC1 at ICP1 TERM /w all CS off: %1.3f V\r\n"), adc1_v);
    float adc2_v = adcSingle(ADC2)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    printf_P(PSTR("ADC2 at ICP3&4 TERM /W all CS off: %1.3f V\r\n"), adc2_v);
    float adc3_v = adcSingle(ADC3)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    printf_P(PSTR("ADC3 at ICP3&4 TERM /W all CS off: %1.3f V\r\n"), adc3_v);
    if ( (adc0_v > 0.01)  || (adc1_v > 0.01) || (adc2_v > 0.01) || (adc3_v > 0.01))
    { 
        passing = 0; 
        printf_P(PSTR(">>> ADC is to high, is the self-test wiring right?\r\n"));
        return;
    }

    // ICP1 pin is inverted from the plug interface, its 100 Ohm Termination should have zero mA. 
    printf_P(PSTR("ICP1 input should be HIGH with 0mA loop current: %d \r\n"), ioRead(MCU_IO_ICP1));
    if (!ioRead(MCU_IO_ICP1)) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> ICP1 should be high.\r\n"));
    }

    // enable CS_ICP1 (with green LED)
    ioWrite(MCU_IO_CS_ICP1,LOGIC_LEVEL_HIGH);
    _delay_ms(100); // busy-wait delay

    // ICP1_TERM has CS_ICP1 on it
    float adc1_cs_icp1_v = adcSingle(ADC1)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    float adc1_cs_icp1_i = adc1_cs_icp1_v / ICP1_TERM;
    printf_P(PSTR("CS_ICP1 on ICP1 TERM: %1.3f A\r\n"), adc1_cs_icp1_i);
    if (adc1_cs_icp1_i < 0.012) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS_ICP1 curr is to low.\r\n"));
    }
    if (adc1_cs_icp1_i > 0.020) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS_ICP1 curr is to high.\r\n"));
    }

    // ICP1 pin is inverted from to the plug interface, which should have 17 mA on its 100 Ohm Termination now
    printf_P(PSTR("ICP1 /w 17mA on termination reads: %d \r\n"), ioRead(MCU_IO_ICP1));
    if (ioRead(MCU_IO_ICP1)) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> ICP1 should be low with 17mA.\r\n"));
    }
    ioWrite(MCU_IO_CS_ICP1,LOGIC_LEVEL_LOW);

    // enable MCU_IO_CS4_EN
    ioWrite(MCU_IO_CS4_EN,LOGIC_LEVEL_HIGH);
    _delay_ms(100); // busy-wait delay

    // ICP1_TERM has MCU_IO_CS4_EN on it
    float adc1_cs4_v = adcSingle(ADC1)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    float adc1_cs4_i = adc1_cs4_v / ICP1_TERM;
    printf_P(PSTR("CS4 on ICP1 TERM: %1.3f A\r\n"), adc1_cs4_i);
    if (adc1_cs4_i < 0.018) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS4 curr is to low.\r\n"));
    }
    if (adc1_cs4_i > 0.026) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS4 curr is to high.\r\n"));
    }
    ioWrite(MCU_IO_CS4_EN,LOGIC_LEVEL_LOW);

    // enable CS_FAST
    ioWrite(MCU_IO_CS_FAST,LOGIC_LEVEL_HIGH);
    _delay_ms(100); // busy-wait delay

    // ICP1_TERM has CS_FAST on it
    float adc1_cs_fast_v = adcSingle(ADC1)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    float adc1_cs_fast_i = adc1_cs_fast_v / ICP1_TERM;
    printf_P(PSTR("CS_FAST on ICP1 TERM: %1.3f A\r\n"), adc1_cs_fast_i);
    if (adc1_cs_fast_i < 0.018) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS_FAST curr is to low.\r\n"));
    }
    if (adc1_cs_fast_i > 0.026) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS_FAST curr is to high.\r\n"));
    }
    ioWrite(MCU_IO_CS_FAST,LOGIC_LEVEL_LOW);

    // ICP3 pin is inverted from the plug interface, its Termination should have zero mA. 
    printf_P(PSTR("ICP3 input with 0mA current: %d \r\n"), ioRead(MCU_IO_ICP3_MOSI));
    if (!ioRead(MCU_IO_ICP3_MOSI)) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> ICP3 should be high.\r\n"));
    }

    // nSS must be HIGH for CS_ICP3 control to work. 
    printf_P(PSTR("nSS has pull up: %d \r\n"), ioRead(MCU_IO_nSS));
    if (!ioRead(MCU_IO_nSS)) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS_ICP3 will not enable with nSS active LOW.\r\n"));
    }

    // enable CS_ICP3 and CS3 to cause ICP3 to go low and enable CS_DIVERSION so that current can flow in ICP1 input
    unsigned long ICP3_one_shot_started_at = milliseconds();
    unsigned long ICP3_one_shot_time = 0;
    ioWrite(MCU_IO_CS_ICP3,LOGIC_LEVEL_HIGH); // 17mA
    ioWrite(MCU_IO_CS3_EN,LOGIC_LEVEL_HIGH);  // + 22mA flowing in 100 Ohm || with ICP3 input
    unsigned long ICP3_one_shot_delay = 0;
    uint8_t ICP3_during_one_shot = 0;
    ioWrite(MCU_IO_CS_ICP3,LOGIC_LEVEL_LOW);  // just long enough to triger the one-shot.
    ioWrite(MCU_IO_CS3_EN,LOGIC_LEVEL_LOW);
    uint8_t wait_for_ICP1_low = 0;
    uint8_t wait_for_ICP1_high = 0;
    uint8_t timeout = 0;
    ICP3_during_one_shot = ioRead(MCU_IO_ICP3_MOSI); // if this is not low then oneshot has not triggered 
    while(!wait_for_ICP1_low && !timeout)
    {
        if ( elapsed(&ICP3_one_shot_started_at) > 100) 
        {
            timeout =1;
            passing = 0; 
            printf_P(PSTR(">>> CS_DIVERSION not seen after ICP3 timeout: %d\r\n"),elapsed(&ICP3_one_shot_started_at));
        }
        if (!ioRead(MCU_IO_ICP1))
        {
            ICP3_one_shot_delay = elapsed(&ICP3_one_shot_started_at);
            wait_for_ICP1_low =1;
        }
    }
    if (wait_for_ICP1_low) 
    { 
        timeout = 0;
        while(!wait_for_ICP1_high && !timeout)
        {
            if ( (elapsed(&ICP3_one_shot_started_at)) > 1000)
            {
                timeout =1;
                passing = 0; 
                ICP3_one_shot_time = elapsed(&ICP3_one_shot_started_at);
                printf_P(PSTR(">>> CS_DIVERSION did not end from ICP3 befor timeout: %d\r\n"),ICP3_one_shot_time);
            }
            if (ioRead(MCU_IO_ICP1))
            {
                ICP3_one_shot_time = elapsed(&ICP3_one_shot_started_at);
                wait_for_ICP1_high =1;
            }
        }
        // time is measured so uart blocking is OK 
        printf_P(PSTR("ICP3 one-shot delay: %d mSec\r\n"), ICP3_one_shot_delay); 
        printf_P(PSTR("ICP3 one-shot time: %d mSec\r\n"), ICP3_one_shot_time);
        if (ICP3_one_shot_time > 5) 
        { 
            passing = 0; 
            printf_P(PSTR(">>> ICP3 one-shot is to long.\r\n"));
        }
        if (ICP3_one_shot_time < 1) 
        { 
            passing = 0; 
            printf_P(PSTR(">>> ICP3 one-shot is to short.\r\n"));
        }
    }
    else
    {
        passing = 0;
        printf_P(PSTR("ICP3 durring one shot: %d \r\n"), ICP3_during_one_shot); // should be LOW
        printf_P(PSTR(">>> ICP3 one-shot runs CS_DIVERSION but that curr was not sent to ICP1.\r\n"));
    }
    ioWrite(MCU_IO_CS_ICP3,LOGIC_LEVEL_HIGH);
    _delay_ms(100); // busy-wait delay

    // ICP3PARL100_TERM has R1 in parrallel (50Ohm) 
    float adc0_cs_icp3_v = adcSingle(ADC0)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    float adc0_cs_icp3_i = adc0_cs_icp3_v / ICP3PARL100_TERM;
    printf_P(PSTR("CS_ICP3 on ICP3||100Ohm TERM: %1.3f A\r\n"), adc0_cs_icp3_i);
    if (adc0_cs_icp3_i < 0.012) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS_ICP3 curr is to low.\r\n"));
    }
    if (adc0_cs_icp3_i > 0.020) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS_ICP3 curr is to high.\r\n"));
    }

    // ICP3 pin is inverted from to the plug interface, which should have 17 mA on its 100 Ohm Termination now
    printf_P(PSTR("ICP3 /w 8mA on termination reads: %d \r\n"), ioRead(MCU_IO_ICP3_MOSI));
    if (ioRead(MCU_IO_ICP1)) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> ICP3 should be low with 8mA.\r\n"));
    }

    //Swap ADC referances and find the band-gap voltage
    init_ADC_single_conversion(INTERNAL_1V1); 
    _delay_ms(100); // busy-wait delay
    int adc0_used_for_ref_intern_1v1_uV = adcSingle(ADC0);
    printf_P(PSTR("   ADC0 reading used to calculate ref_intern_1v1_uV: %d int\r\n"), adc0_used_for_ref_intern_1v1_uV);
    float _ref_intern_1v1_uV = 1.0E6*1024.0 * ((adc0_cs_icp3_i * ICP3PARL100_TERM) / adc0_used_for_ref_intern_1v1_uV);
    uint32_t temp_ref_intern_1v1_uV = (uint32_t)_ref_intern_1v1_uV;
    printf_P(PSTR("   calculated ref_intern_1v1_uV: %lu uV\r\n"), temp_ref_intern_1v1_uV);
    uint32_t temp_ref_extern_avcc_uV = ref_extern_avcc_uV;
    
    // check for old referance values
    ref_extern_avcc_uV = 0;
    ref_intern_1v1_uV = 0;
    LoadAnalogRefFromEEPROM();
    printf_P(PSTR("REF_EXTERN_AVCC old value found in eeprom: %lu uV\r\n"), ref_extern_avcc_uV);
    printf_P(PSTR("REF_INTERN_1V1 old value found in eeprom: %lu uV\r\n"), ref_intern_1v1_uV);
    ref_intern_1v1_uV = temp_ref_intern_1v1_uV;
    if (ref_extern_avcc_uV == temp_ref_extern_avcc_uV)
    {
        printf_P(PSTR("REF_EXTERN_AVCC from eeprom is same\r\n"));
    }
    else
    {
        ref_extern_avcc_uV = temp_ref_extern_avcc_uV;
        if ((ref_intern_1v1_uV > 1050000UL)  || (ref_intern_1v1_uV < 1150000UL) )
        {
            while ( !WriteEeReferenceId() ) {};
            while ( !WriteEeReferenceAvcc() ) {};
            while ( !WriteEeReference1V1() ) {};
            printf_P(PSTR("REF_EXTERN_AVCC saved into eeprom: %lu uV\r\n"), ref_extern_avcc_uV);
            printf_P(PSTR("REF_INTERN_1V1 saved into eeprom: %lu uV\r\n"), ref_intern_1v1_uV);
        }
        else
        { 
            passing = 0; 
            printf_P(PSTR(">>> REF_* for ADC not saved in eeprom.\r\n"));
        }
    }
    ioWrite(MCU_IO_CS_ICP3,LOGIC_LEVEL_LOW);
    _delay_ms(100); // busy-wait delay

    // ICP4 pin is inverted from the plug interface, its Termination should have zero mA. 
    printf_P(PSTR("ICP4 input should be HIGH with 0mA loop current: %d \r\n"), ioRead(MCU_IO_ICP4));
    if (!ioRead(MCU_IO_ICP4)) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> ICP4 should be high.\r\n"));
    }

    // enable CS_DIVERSION
    ioWrite(MCU_IO_CS_DIVERSION,LOGIC_LEVEL_HIGH);
    init_ADC_single_conversion(EXTERNAL_AVCC); 
    _delay_ms(100); // busy-wait delay

    // ICP1_TERM has CS_DIVERSION on it
    float adc1_cs_diversion_v = adcSingle(ADC1)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    float adc1_cs_diversion_i = adc1_cs_diversion_v / ICP1_TERM;
    printf_P(PSTR("CS_DIVERSION on ICP1 TERM: %1.3f A\r\n"), adc1_cs_diversion_i);
    if (adc1_cs_diversion_i < 0.018) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS_DIVERSION curr is to low.\r\n"));
    }
    if (adc1_cs_diversion_i > 0.026) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS_DIVERSION curr is to high.\r\n"));
    }

    // enable CS_ICP4 which will cause ICP4 to go low and disable CS_DIVERSION for one or two mSec
    if (ioRead(MCU_IO_ICP1)) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> ICP1 should be low befor turning on CS_ICP4.\r\n"));
    }
    ioWrite(MCU_IO_CS_ICP4,LOGIC_LEVEL_HIGH);
    unsigned long ICP4_one_shot_started_at = milliseconds();
    unsigned long ICP4_one_shot_event = 0;
    unsigned long ICP4_one_shot_delay = 0;
    wait_for_ICP1_high = 0;
    wait_for_ICP1_low = 0;
    timeout = 0;
    while(!wait_for_ICP1_high && !timeout)
    {
        if ( (elapsed(&ICP4_one_shot_started_at)) > 100) 
        {
            timeout =1;
            passing = 0; 
            printf_P(PSTR(">>> CS_DIVERSION did not end from ICP4 befor timeout: %d\r\n"), elapsed(&ICP4_one_shot_started_at));
        }
        if (ioRead(MCU_IO_ICP1))
        {
            ICP4_one_shot_delay = elapsed(&ICP4_one_shot_started_at);
            wait_for_ICP1_high =1;
        }
    }
    
    // ICP4_TERM with CS_ICP4 is measured with ADC3
    float adc3_cs_icp4_v = adcSingle(ADC3)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    float adc3_cs_icp4_i = adc3_cs_icp4_v / ICP4_TERM;

    // ICP4 pin is inverted logic, and should a low when 17 mA is on the ICP4_TERM Termination
    uint8_t  icp4_befor_cs_turned_off = ioRead(MCU_IO_ICP4);

    // ICP4 is on the gate of an n-channel level shift that will cut-off CS_DIVERSION when it is low
    ioWrite(MCU_IO_CS_ICP4,LOGIC_LEVEL_LOW);
    if (wait_for_ICP1_high) 
    { 
        timeout = 0;
        while(!wait_for_ICP1_low && !timeout)
        {
            if ( (elapsed(&ICP4_one_shot_started_at)) > 1000)
            {
                timeout =1;
                passing = 0; 
                ICP4_one_shot_event = elapsed(&ICP4_one_shot_started_at);
                printf_P(PSTR(">>> CS_DIVERSION did not restart from ICP4 befor timeout: %d\r\n"),ICP4_one_shot_event);
            }
            if (!ioRead(MCU_IO_ICP1))
            {
                ICP4_one_shot_event = elapsed(&ICP4_one_shot_started_at);
                wait_for_ICP1_low =1; // it was high befor turning off cs_icp4, so the one shot is the only thing holding cs_diversion off
            }
        }
        
        // time is measured so uart blocking is OK now
        printf_P(PSTR("CS_ICP4 on ICP4_TERM: %1.3f A\r\n"), adc3_cs_icp4_i);
        if (adc3_cs_icp4_i < 0.012) 
        { 
            passing = 0; 
            printf_P(PSTR(">>> CS_ICP4 curr is to low.\r\n"));
        }
        if (adc3_cs_icp4_i > 0.020) 
        { 
            passing = 0; 
            printf_P(PSTR(">>> CS_ICP4 curr is to high.\r\n"));
        }
        printf_P(PSTR("ICP4 /w 17mA on termination reads: %d \r\n"), icp4_befor_cs_turned_off);
        if (icp4_befor_cs_turned_off) 
        { 
            passing = 0; 
            printf_P(PSTR(">>> ICP4 should be low with 17mA.\r\n"));
        }
        printf_P(PSTR("ICP4 one-shot delay: %d mSec\r\n"), ICP4_one_shot_delay); 
        printf_P(PSTR("ICP4 one-shot time: %d mSec\r\n"), ICP4_one_shot_event);
        if (ICP4_one_shot_event > 3) 
        { 
            passing = 0; 
            printf_P(PSTR(">>> ICP4 one-shot is to long.\r\n"));
        }
        if (ICP4_one_shot_event  < 1) 
        { 
            passing = 0; 
            printf_P(PSTR(">>> ICP4 one-shot is to short.\r\n"));
        }
    }
    else
    {
        passing = 0; 
        printf_P(PSTR(">>> ICP4 one-shot ends CS_DIVERSION but that was not seen on ICP1.\r\n"));
    }

    // everything off for input current measurement
    ioWrite(MCU_IO_CS_DIVERSION,LOGIC_LEVEL_LOW);
    ioWrite(MCU_IO_CS_ICP4,LOGIC_LEVEL_LOW);
    // init_ADC_single_conversion(INTERNAL_1V1); // use ref_intern_1v1_uV
    _delay_ms(1500); // busy-wait delay
    
    // Input current at no load 
    float input_i =  i2c_get_adc_from_manager(PWR_I)*((ref_extern_avcc_uV/1.0E6)/1024.0)/(0.068*50.0);
    printf_P(PSTR("PWR_I at no load : %1.3f A\r\n"), input_i);
    if (input_i > 0.026) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> Input curr is to high.\r\n"));
        return;
    }
    if (input_i < 0.007) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> Input curr is to low.\r\n"));
    }

    //swap back to the AVCC referance and enable CS0 (through red LED)
    //init_ADC_single_conversion(EXTERNAL_AVCC); 
    ioWrite(MCU_IO_CS0_EN,LOGIC_LEVEL_HIGH);
    _delay_ms(100); // busy-wait delay

    // CS0 drives ICP3 and ICP4 termination which should make a 50 Ohm drop
    float adc0_cs0_v = adcSingle(ADC0)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    float adc0_cs0_i = adc0_cs0_v / ICP3PARL100_TERM;
    printf_P(PSTR("CS0 on ICP3||100: %1.3f A\r\n"), adc0_cs0_i);
    if (adc0_cs0_i < 0.018) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS0 curr is to low.\r\n"));
    }
    if (adc0_cs0_i > 0.026) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS0 curr is to high.\r\n"));
    }
    ioWrite(MCU_IO_CS0_EN,LOGIC_LEVEL_LOW);

    // enable CS1
    ioWrite(MCU_IO_CS1_EN,LOGIC_LEVEL_HIGH);
    _delay_ms(100); // busy-wait delay
    
    // CS1  drives ICP3 and ICP4 termination which should make a 50 Ohm drop
    float adc0_cs1_v = adcSingle(ADC0)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    float adc0_cs1_i = adc0_cs1_v / ICP3PARL100_TERM;
    printf_P(PSTR("CS1 on ICP3||100: %1.3f A\r\n"), adc0_cs1_i);
    if (adc0_cs1_i < 0.018) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS1 curr is to low.\r\n"));
    }
    if (adc0_cs1_i > 0.026) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS1 curr is to high.\r\n"));
    }
    ioWrite(MCU_IO_CS1_EN,LOGIC_LEVEL_LOW);

    // enable CS2
    ioWrite(MCU_IO_CS2_EN,LOGIC_LEVEL_HIGH);
    _delay_ms(100); // busy-wait delay
    
    // CS2  drives ICP3 and ICP4 termination which should make a 50 Ohm drop
    float adc0_cs2_v = adcSingle(ADC0)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    float adc0_cs2_i = adc0_cs2_v / ICP3PARL100_TERM;
    printf_P(PSTR("CS2 on ICP3||100: %1.3f A\r\n"), adc0_cs2_i);
    if (adc0_cs2_i < 0.018) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS2 curr is to low.\r\n"));
    }
    if (adc0_cs2_i > 0.026) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS2 curr is to high.\r\n"));
    }
    ioWrite(MCU_IO_CS2_EN,LOGIC_LEVEL_LOW);

    // enable CS3
    ioWrite(MCU_IO_CS3_EN,LOGIC_LEVEL_HIGH);
    _delay_ms(100); // busy-wait delay
    
    // CS3  drives ICP3 and ICP4 termination which should make a 50 Ohm drop
    float adc0_cs3_v = adcSingle(ADC0)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    float adc0_cs3_i = adc0_cs3_v / ICP3PARL100_TERM;
    printf_P(PSTR("CS3 on ICP3||100: %1.3f A\r\n"), adc0_cs3_i);
    if (adc0_cs3_i < 0.018) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS3 curr is to low.\r\n"));
    }
    if (adc0_cs3_i > 0.026) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS3 curr is to high.\r\n"));
    }
    ioWrite(MCU_IO_CS3_EN,LOGIC_LEVEL_LOW);

    // Serial One pins loopback, e.g., drive TX1 to test RX1.
    ioDir(MCU_IO_TX1,DIRECTION_OUTPUT);
    ioWrite(MCU_IO_TX1,LOGIC_LEVEL_HIGH);
    ioDir(MCU_IO_RX1,DIRECTION_INPUT);
    ioWrite(MCU_IO_RX1,LOGIC_LEVEL_LOW); // turn off the weak pullup
    _delay_ms(50) ; // busy-wait delay
    uint8_t tx1_rd = ioRead(MCU_IO_TX1);
    uint8_t rx1_rd = ioRead(MCU_IO_RX1);
    if (tx1_rd && rx1_rd) 
    { 
        printf_P(PSTR("TX1 loopback to RX1 == HIGH\r\n"));
    }
    else 
    { 
        passing = 0; 
        printf_P(PSTR(">>> TX1 %d did not loopback to RX1 %d\r\n"), tx1_rd, rx1_rd);
    }

    ioWrite(MCU_IO_TX1,LOGIC_LEVEL_LOW);
    _delay_ms(50) ; // busy-wait delay
    tx1_rd = ioRead(MCU_IO_TX1);
    rx1_rd = ioRead(MCU_IO_RX1);
    if ( (!tx1_rd) && (!rx1_rd) ) 
    { 
        printf_P(PSTR("TX1 loopback to RX1 == LOW\r\n"));
    }
    else 
    { 
        passing = 0; 
        printf_P(PSTR(">>> TX1 %d did not loopback to RX1 %d\r\n"), tx1_rd, rx1_rd);
    }

    // Serial Two pins loopback, e.g., drive TX2 to test RX2.
    ioDir(MCU_IO_TX2,DIRECTION_OUTPUT);
    ioWrite(MCU_IO_TX2,LOGIC_LEVEL_HIGH);
    ioDir(MCU_IO_RX2,DIRECTION_INPUT);
    ioWrite(MCU_IO_RX2,LOGIC_LEVEL_LOW); // turn off the weak pullup
    _delay_ms(50) ; // busy-wait delay
    uint8_t tx2_rd = ioRead(MCU_IO_TX2);
    uint8_t rx2_rd = ioRead(MCU_IO_RX2);
    if (tx2_rd && rx2_rd) 
    { 
        printf_P(PSTR("TX2 loopback to RX2 == HIGH\r\n"));
    }
    else 
    { 
        passing = 0; 
        printf_P(PSTR(">>> TX2 %d did not loopback to RX2 %d\r\n"), tx2_rd, rx2_rd);
    }

    ioWrite(MCU_IO_TX2,LOGIC_LEVEL_LOW);
    _delay_ms(50) ; // busy-wait delay
    tx2_rd = ioRead(MCU_IO_TX2);
    rx2_rd = ioRead(MCU_IO_RX2);
    if ( (!tx2_rd) && (!rx2_rd) ) 
    { 
        printf_P(PSTR("TX2 loopback to RX2 == LOW\r\n"));
    }
    else 
    { 
        passing = 0; 
        printf_P(PSTR(">>> TX2 %d did not loopback to RX2 %d\r\n"), tx2_rd, rx2_rd);
    }

    // XcvrTest
    
    // SMBus from manager needs connected to I2C1 master for testing, it is for write_i2c_block_data and read_i2c_block_data from
    // https://git.kernel.org/pub/scm/utils/i2c-tools/i2c-tools.git/tree/py-smbus/smbusmodule.c
    // write_i2c_block_data sends a command byte and data to the slave 
    smbus_address();

    // SPI loopback at R-Pi header. e.g., drive MISO to test MOSI.
    ioDir(MCU_IO_MISO, DIRECTION_OUTPUT);
    ioWrite(MCU_IO_MISO, LOGIC_LEVEL_HIGH);
    ioDir(MCU_IO_ICP3_MOSI, DIRECTION_INPUT);
    ioWrite(MCU_IO_ICP3_MOSI, LOGIC_LEVEL_LOW); // turn off the weak pullup on the 324pb side of open drain buffer since it has a 3k pullup resistor 
    _delay_ms(50) ; // busy-wait delay
    uint8_t miso_rd = ioRead(MCU_IO_MISO);
    uint8_t mosi_rd = ioRead(MCU_IO_ICP3_MOSI);
    if (miso_rd && mosi_rd) 
    { 
        printf_P(PSTR("MISO loopback to MOSI == HIGH\r\n"));
    }
    else 
    { 
        passing = 0; 
        printf_P(PSTR(">>> MISO %d did not loopback to MOSI %d\r\n"), miso_rd, mosi_rd);
    }

    ioWrite(MCU_IO_MISO,LOGIC_LEVEL_LOW);
    _delay_ms(50) ; // busy-wait delay
    miso_rd = ioRead(MCU_IO_MISO);
    mosi_rd = ioRead(MCU_IO_ICP3_MOSI);
    if ( (!miso_rd) && (!mosi_rd) ) 
    { 
        printf_P(PSTR("MISO loopback to MOSI == LOW\r\n"));
    }
    else 
    { 
        passing = 0; 
        printf_P(PSTR(">>> MISO %d did not loopback to MOSI %d\r\n"), miso_rd, mosi_rd);
        printf_P(PSTR(">>> R-Pi POL pin needs 5V for loopback to work\r\n"));
    }

    // My R-Pi Shutdown is on BCM6 (pin31) and that loops back into SCK on the test header
    ioDir(MCU_IO_SCK,DIRECTION_INPUT);
    ioWrite(MCU_IO_SCK,LOGIC_LEVEL_LOW); // turn off the weak pullup, the RPUpi board has a 3k pullup resistor 
    _delay_ms(50) ; // busy-wait delay
    uint8_t sck_rd = ioRead(MCU_IO_SCK);
    if (sck_rd ) 
    { 
        printf_P(PSTR("SCK with Shutdown loopback == HIGH\r\n"));
    }
    else 
    { 
        passing = 0; 
        printf_P(PSTR(">>> Shutdown HIGH did not loopback to SCK %d\r\n"), sck_rd);
    }

    // To cause a shutdown I can send the I2C command 5 with data 1
    // note:  this does not monitor the input power of the R-Pi or turn off power to the R-Pi at this time.    
    i2c_shutdown();

    _delay_ms(50) ; // busy-wait delay
    sck_rd = ioRead(MCU_IO_SCK);
    if (!sck_rd) 
    { 
        printf_P(PSTR("SCK with Shutdown loopback == LOW\r\n"));
    }
    else 
    { 
        passing = 0; 
        printf_P(PSTR(">>> Shutdown LOW did not loopback to SCK %d\r\n"), sck_rd);
    }

    // Manager should save the shutdown detected after SHUTDOWN_TIME timer runs (see rpubus_manager_state.h in Remote fw for value)
    _delay_ms(1100) ; // busy-wait delay
    i2c_shutdown_detect();

    // Set test mode which will save trancever control bits HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE
    test_mode_clean = 0;
    delayed_outputs = 0; // each bit selects a output to print
    printf_P(PSTR("\r\n"));
    printf_P(PSTR("Testmode: default trancever control bits\r\n"));
    i2c_testmode_start();
    //init_ADC_single_conversion(INTERNAL_1V1); // use ref_intern_1v1_uV
    _delay_ms(1000) ; // busy-wait delay for input current measurement
    // check xcvr bits after start of testmode. Note printf is done after end of testmode.
    uint8_t xcvrbits_after_testmode_start = 0xE2;
    i2c_testmode_test_xcvrbits(xcvrbits_after_testmode_start);
    float noload_i = i2c_get_adc_from_manager(PWR_I)*((ref_extern_avcc_uV/1.0E6)/1024.0)/(0.068*50.0);

    // End test mode 
    i2c_testmode_end();
    
    // show the delayed test results now that UART is on-line
    if (delayed_outputs & (1<<4))
    {
        printf_P(PSTR(">>> I2C0 cmd 50 write fail, twi_returnCode: %d\r\n"), delayed_data[4]);
    }
    if (delayed_outputs & (1<<5))
    {
        print_twi0_read_status(delayed_data[5]);
    }
    if (delayed_outputs & (1<<6))
    {
        printf_P(PSTR("Testmode: read  Xcvr cntl bits {50, 0x%X}\r\n"), delayed_data[6]);
    }
    if (delayed_outputs & (1<<7))
    {
        printf_P(PSTR(">>> Xcvr cntl bits should be %x but report was %x\r\n"), xcvrbits_after_testmode_start, delayed_data[7]);
    }
    printf_P(PSTR("PWR_I /w no load using REF_EXTERN_AVCC: %1.3f A\r\n"), noload_i);
    if (noload_i > 0.020) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> no load curr is to high.\r\n"));
    }
    if (noload_i < 0.006) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> no load curr is to low.\r\n"));
    }

    // set CTS low and verify it loops back to RTS
    // control bits  HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE
    test_mode_clean = 0;
    delayed_outputs = 0; // each bit selects a output to print
    printf_P(PSTR("\r\n"));
    printf_P(PSTR("Testmode: nCTS loopback to nRTS\r\n"));
    i2c_testmode_start();
    _delay_ms(50) ; // busy-wait delay

    // set nCTS durring testmode and check the loopback on nRTS. Note printf is done after end of testmode.
    uint8_t xcvrbits_change_cts_bit_low = 0xA2; //0b10100010
    i2c_testmode_set_xcvrbits(xcvrbits_change_cts_bit_low);
    uint8_t xcvrbits_cts_loopback_to_rts = 0x22; //0b00100010
    i2c_testmode_test_xcvrbits(xcvrbits_cts_loopback_to_rts);

    // End test mode 
    i2c_testmode_end();
    
    // show the delayed test results now that UART is on-line
    if (delayed_outputs & (1<<0))
    {
        printf_P(PSTR(">>> I2C0 cmd 51 write fail, twi_returnCode: %d\r\n"), delayed_data[0]);
    }
    if (delayed_outputs & (1<<1))
    {
        print_twi0_read_status(delayed_data[1]);
    }
    if (delayed_outputs & (1<<2))
    {
        printf_P(PSTR("Testmode: set  Xcvr cntl bits {51, 0x%X}\r\n"), delayed_data[2]);
    }
    if (delayed_outputs & (1<<3))
    {
        printf_P(PSTR(">>> Xcvr cntl bits should be %x but report was %x\r\n"), xcvrbits_change_cts_bit_low, delayed_data[3]);
    }
    if (delayed_outputs & (1<<4))
    {
        printf_P(PSTR(">>> I2C0 cmd 50 write fail, twi_returnCode: %d\r\n"), delayed_data[4]);
    }
    if (delayed_outputs & (1<<5))
    {
        print_twi0_read_status(delayed_data[5]);
    }
    if (delayed_outputs & (1<<6))
    {
        printf_P(PSTR("Testmode: read  Xcvr cntl bits {50, 0x%X}\r\n"), delayed_data[6]);
    }
    if (delayed_outputs & (1<<7))
    {
        printf_P(PSTR(">>> Xcvr cntl bits should be %x but report was %x\r\n"), xcvrbits_cts_loopback_to_rts, delayed_data[7]);
    }

    // Enable TX driver durring testmode and measure PWR_I.
    test_mode_clean = 0;
    delayed_outputs = 0; // each bit selects a output to print
    printf_P(PSTR("\r\n"));
    printf_P(PSTR("Testmode: Enable TX pair driver\r\n"));
    i2c_testmode_start();
    _delay_ms(50) ; // busy-wait delay
    UCSR0B &= ~( (1<<RXEN0)|(1<<TXEN0) ); // turn off UART 
    ioDir(MCU_IO_TX0,DIRECTION_OUTPUT);
    ioWrite(MCU_IO_TX0,LOGIC_LEVEL_LOW); // the TX pair will now be driven and load the transceiver 

    // control bits  HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE
    // TX_DE drives the twisted pair from this UART, and TX_nRE drives the line to the host (which is not enabled)
    uint8_t xcvrbits_enable_xtde = 0xF2; //0b11110010
    i2c_testmode_set_xcvrbits(xcvrbits_enable_xtde);
    i2c_testmode_test_xcvrbits(xcvrbits_enable_xtde); // to be clear "...set..." does not verify the setting
    _delay_ms(1000) ; // busy-wait delay
    float load_txde_i = i2c_get_adc_from_manager(PWR_I)*((ref_extern_avcc_uV/1.0E6)/1024.0)/(0.068*50.0);

    // End test mode 
    ioWrite(MCU_IO_TX0,LOGIC_LEVEL_HIGH); // strong pullup
    _delay_ms(10) ; // busy-wait delay
    ioDir(MCU_IO_TX0,DIRECTION_INPUT); // the TX pair should probably have a weak pullup when UART starts
    UCSR0B |= (1<<RXEN0)|(1<<TXEN0); // turn on UART
    i2c_testmode_end();
    
    // show the delayed test results now that UART is on-line
    if (delayed_outputs & (1<<0))
    {
        printf_P(PSTR(">>> I2C0 cmd 51 write fail, twi_returnCode: %d\r\n"), delayed_data[0]);
    }
    if (delayed_outputs & (1<<1))
    {
        print_twi0_read_status(delayed_data[1]);
    }
    if (delayed_outputs & (1<<2))
    {
        printf_P(PSTR("Testmode: set  Xcvr cntl bits {51, 0x%X}\r\n"), delayed_data[2]);
    }
    if (delayed_outputs & (1<<3))
    {
        printf_P(PSTR(">>> Xcvr cntl bits should be %x but report was %x\r\n"), xcvrbits_enable_xtde, delayed_data[3]);
    }
    if (delayed_outputs & (1<<4))
    {
        printf_P(PSTR(">>> I2C0 cmd 50 write fail, twi_returnCode: %d\r\n"), delayed_data[4]);
    }
    if (delayed_outputs & (1<<5))
    {
        print_twi0_read_status(delayed_data[5]);
    }
    if (delayed_outputs & (1<<6))
    {
        printf_P(PSTR("Testmode: read  Xcvr cntl bits {50, 0x%X}\r\n"), delayed_data[6]);
    }
    if (delayed_outputs & (1<<7))
    {
        printf_P(PSTR(">>> Xcvr cntl bits should be %x but report was %x\r\n"), xcvrbits_enable_xtde, delayed_data[7]);
    }
    printf_P(PSTR("PWR_I /w TX pair load: %1.3f A\r\n"), load_txde_i);
    if (load_txde_i > 0.055) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> TX pair load curr is to high.\r\n"));
    }
    if (load_txde_i < 0.025) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> TX pair load curr is to low.\r\n"));
    }

    // Enable TX and RX driver durring testmode and measure PWR_I.
    test_mode_clean = 0;
    delayed_outputs = 0; // each bit selects a output to print
    printf_P(PSTR("\r\n"));
    printf_P(PSTR("Testmode: Enable TX & RX(loopback) pair drivers\r\n"));
    i2c_testmode_start();
    _delay_ms(50) ; // busy-wait delay
    UCSR0B &= ~( (1<<RXEN0)|(1<<TXEN0) ); // turn off UART 
    ioDir(MCU_IO_TX0,DIRECTION_OUTPUT);
    ioWrite(MCU_IO_TX0,LOGIC_LEVEL_LOW); // the TX pair will now be driven and load the transceiver 

    // control bits  HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE
    // TX_DE drives the TX twisted pair from this UART, and TX_nRE drives the line to the host which is a loopback
    // RX_DE drives the RX twisted pair from the loopback, and RX_nRE drives the line to this RX pair.
    uint8_t xcvrbits_enable_xtrx = 0xD1; //0b11010001
    i2c_testmode_set_xcvrbits(xcvrbits_enable_xtrx);
    i2c_testmode_test_xcvrbits(xcvrbits_enable_xtrx); // to be clear "...set..." does not verify the setting
    _delay_ms(1000) ; // busy-wait delay
    float load_txrx_i = i2c_get_adc_from_manager(PWR_I)*((ref_extern_avcc_uV/1.0E6)/1024.0)/(0.068*50.0);
    uint8_t rx_loopback = ioRead(MCU_IO_RX0); 

    // End test mode 
    ioWrite(MCU_IO_TX0,LOGIC_LEVEL_HIGH); // strong pullup
    _delay_ms(10) ; // busy-wait delay
    ioDir(MCU_IO_TX0,DIRECTION_INPUT); // the TX pair should probably have a weak pullup when UART starts
    UCSR0B |= (1<<RXEN0)|(1<<TXEN0); // turn on UART
    i2c_testmode_end();
    
    // show the delayed test results now that UART is on-line
    if (delayed_outputs & (1<<0))
    {
        printf_P(PSTR(">>> I2C0 cmd 51 write fail, twi_returnCode: %d\r\n"), delayed_data[0]);
    }
    if (delayed_outputs & (1<<1))
    {
        print_twi0_read_status(delayed_data[1]);
    }
    if (delayed_outputs & (1<<2))
    {
        printf_P(PSTR("Testmode: set  Xcvr cntl bits {51, 0x%X}\r\n"), delayed_data[2]);
    }
    if (delayed_outputs & (1<<3))
    {
        printf_P(PSTR(">>> Xcvr cntl bits should be %x but report was %x\r\n"), xcvrbits_enable_xtrx, delayed_data[3]);
    }
    if (delayed_outputs & (1<<4))
    {
        printf_P(PSTR(">>> I2C0 cmd 50 write fail, twi_returnCode: %d\r\n"), delayed_data[4]);
    }
    if (delayed_outputs & (1<<5))
    {
        print_twi0_read_status(delayed_data[5]);
    }
    if (delayed_outputs & (1<<6))
    {
        printf_P(PSTR("Testmode: read  Xcvr cntl bits {50, 0x%X}\r\n"), delayed_data[6]);
    }
    if (delayed_outputs & (1<<7))
    {
        printf_P(PSTR(">>> Xcvr cntl bits should be %x but report was %x\r\n"), xcvrbits_enable_xtrx, delayed_data[7]);
    }
    printf_P(PSTR("PWR_I /w TX and RX pairs loaded: %1.3f A\r\n"), load_txrx_i);
    if (load_txrx_i > 0.075) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> TX and RX pairs load curr are too high.\r\n"));
    }
    if (load_txrx_i < 0.045) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> TX and RX pairs load curr are too low.\r\n"));
    }
    if (rx_loopback)
    {
        passing = 0;
        printf_P(PSTR(">>> RX loopback should be LOW but was HIGH\r\n"));
    }
    else
    {
        printf_P(PSTR("RX loopback checked\r\n"));
    }

   // Enable DTR driver durring testmode and measure PWR_I.
    test_mode_clean = 0;
    delayed_outputs = 0; // each bit selects a output to print
    printf_P(PSTR("\r\n"));
    printf_P(PSTR("Testmode: Enable DTR pair driver\r\n"));
    i2c_testmode_start();
    _delay_ms(50) ; // busy-wait delay

    // control bits  HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE
    // everything off except DTR_nRE stays on and DTR_DE is turned on
    // when the i2c commad sees DTR_DE set high it turns off the UART and
    // enables the DTR pair driver then pulls the DTR_TXD line low to load the DTR pair.
    uint8_t xcvrbits_enable_dtr = 0xE6; //0b11100110
    i2c_testmode_set_xcvrbits(xcvrbits_enable_dtr);
    i2c_testmode_test_xcvrbits(xcvrbits_enable_dtr); // to be clear "...set..." does not verify the setting
    _delay_ms(1000) ; // busy-wait delay
    float load_dtr_i = i2c_get_adc_from_manager(PWR_I)*((ref_extern_avcc_uV/1.0E6)/1024.0)/(0.068*50.0);

    // End test mode will setup the DTR_TXD line and enable the manager's UART as it recovers
    i2c_testmode_end();
    
    // show the delayed test results now that UART is on-line
    if (delayed_outputs & (1<<0))
    {
        printf_P(PSTR(">>> I2C0 cmd 51 write fail, twi_returnCode: %d\r\n"), delayed_data[0]);
    }
    if (delayed_outputs & (1<<1))
    {
        print_twi0_read_status(delayed_data[1]);
    }
    if (delayed_outputs & (1<<2))
    {
        printf_P(PSTR("Testmode: set  Xcvr cntl bits {51, 0x%X}\r\n"), delayed_data[2]);
    }
    if (delayed_outputs & (1<<3))
    {
        printf_P(PSTR(">>> Xcvr cntl bits should be %x but report was %x\r\n"), xcvrbits_enable_dtr, delayed_data[3]);
    }
    if (delayed_outputs & (1<<4))
    {
        printf_P(PSTR(">>> I2C0 cmd 50 write fail, twi_returnCode: %d\r\n"), delayed_data[4]);
    }
    if (delayed_outputs & (1<<5))
    {
        print_twi0_read_status(delayed_data[5]);
    }
    if (delayed_outputs & (1<<6))
    {
        printf_P(PSTR("Testmode: read  Xcvr cntl bits {50, 0x%X}\r\n"), delayed_data[6]);
    }
    if (delayed_outputs & (1<<7))
    {
        printf_P(PSTR(">>> Xcvr cntl bits should be %x but report was %x\r\n"), xcvrbits_enable_dtr, delayed_data[7]);
    }
    printf_P(PSTR("PWR_I /w DTR pair load: %1.3f A\r\n"), load_dtr_i);
    if (load_dtr_i > 0.055) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> DTR load curr is to high.\r\n"));
    }
    if (load_dtr_i < 0.005) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> DTR load curr is to low.\r\n"));
    }

    // final test status
    if (passing)
    {
        printf_P(PSTR("[PASS]\r\n"));
    }
    else
    {
        printf_P(PSTR("[FAIL]\r\n"));
    }
    printf_P(PSTR("\r\n\r\n\r\n"));
}

void led_setup_after_test(void)
{
    setup_pins_off();
    if (passing)
    {
        ioWrite(MCU_IO_CS_ICP1,LOGIC_LEVEL_HIGH);
    }
    else
    {
        ioWrite(MCU_IO_CS0_EN,LOGIC_LEVEL_HIGH);
    }
}

void blink(void)
{
    unsigned long kRuntime = elapsed(&blink_started_at);
    if ( kRuntime > blink_delay)
    {
        if (passing)
        {
            ioToggle(MCU_IO_CS_ICP1);
        }
        else
        {
            ioToggle(MCU_IO_CS0_EN);
        }
        
        // next toggle 
        blink_started_at += blink_delay; 
    }
}

int main(void)
{
    setup(); 
    
    passing = 1;
    test();
    led_setup_after_test();
    
    while (1) 
    {
        blink();
    }    
}

