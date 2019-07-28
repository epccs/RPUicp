/* 
Gravimetric SelfTest
Copyright (C) 2019 Ronald Sutherland

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

For a copy of the GNU General Public License use
http://www.gnu.org/licenses/gpl-2.0.html
*/ 

#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/io.h>
#include "../lib/timers.h"
#include "../lib/uart.h"
#include "../lib/twi0.h"
#include "../lib/adc.h"
#include "../lib/rpu_mgr.h"
#include "../lib/pin_num.h"
#include "../lib/pins_board.h"
#include "../Adc/references.h"

#define BLINK_DELAY 1000UL

// Save the Value of the References for ADC converter 
// measure AVCC and put it hear in uV 
#define REF_EXTERN_AVCC 4958300UL


#define ICP3AND4_TERM 50.0
#define ICP1_TERM 100.0

static unsigned long blink_started_at;
static unsigned long blink_delay;
static char rpu_addr;
static uint8_t passing;

void setup_pins_off(void)
{
    // Turn Off Curr Sources
    pinMode(CS0_EN,OUTPUT);
    digitalWrite(CS0_EN,LOW);
    pinMode(CS1_EN,OUTPUT);
    digitalWrite(CS1_EN,LOW);
    pinMode(CS2_EN,OUTPUT);
    digitalWrite(CS2_EN,LOW);
    pinMode(CS3_EN,OUTPUT);
    digitalWrite(CS3_EN,LOW);
    pinMode(CS4_EN,OUTPUT);
    digitalWrite(CS4_EN,LOW);
    pinMode(CS_ICP1,OUTPUT);
    digitalWrite(CS_ICP1,LOW);
    pinMode(CS_ICP3,OUTPUT);
    digitalWrite(CS_ICP3,LOW);
    pinMode(CS_ICP4,OUTPUT);
    digitalWrite(CS_ICP4,LOW);
    pinMode(CS_FAST,OUTPUT);
    digitalWrite(CS_FAST,LOW);
    pinMode(CS_DIVERSION,OUTPUT);
    digitalWrite(CS_DIVERSION,LOW);

    // DIO and Analog
    pinMode(RX1,INPUT);
    digitalWrite(RX1,LOW);
    pinMode(TX1,INPUT);
    digitalWrite(TX1,LOW);
    pinMode(RX2,INPUT);
    digitalWrite(RX2,LOW);
    pinMode(TX2,INPUT);
    digitalWrite(TX2,LOW);
    pinMode(SDA1,INPUT);
    digitalWrite(SDA1,LOW);
    pinMode(SCL1,INPUT);
    digitalWrite(SCL1,LOW);
    pinMode(28,INPUT); // ADC0
    pinMode(29,INPUT);
    pinMode(30,INPUT);
    pinMode(31,INPUT);
    pinMode(32,INPUT);
    pinMode(33,INPUT);
    pinMode(34,INPUT);
    pinMode(35,INPUT); // ADC7
    
    // R-Pi power control
    pinMode(SHLD_VIN_EN,OUTPUT);
    digitalWrite(SHLD_VIN_EN,LOW);
    
    // Alternate power control
    pinMode(ALT_EN,OUTPUT);
    digitalWrite(ALT_EN,LOW);
}


void setup(void) 
{
    setup_pins_off();

    // Initialize Timers, ADC, and clear bootloader, Arduino does these with init() in wiring.c
    initTimers(); //Timer0 Fast PWM mode, Timer1 & Timer2 Phase Correct PWM mode.
    init_ADC_single_conversion(EXTERNAL_AVCC); // warning AREF must not be connected to anything
    init_uart0_after_bootloader(); // bootloader may have the UART setup

    /* Initialize UART, it returns a pointer to FILE so redirect of stdin and stdout works*/
    stdout = stdin = uartstream0_init(BAUD);

    /* Initialize I2C, with the internal pull-up*/
    twi0_init(TWI_PULLUP);

    // Enable global interrupts to start TIMER0 and UART
    sei(); 

    blink_started_at = millis();

    rpu_addr = get_Rpu_address();
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

    // With current sources off measure input current
    _delay_ms(1000) ; // busy-wait to let the 1uF settle
    
    // Input voltage
    int adc_pwr_v = analogRead(PWR_V);
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
    float adc0_v = analogRead(ADC0)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    printf_P(PSTR("ADC0 at ICP3&4 TERM /W all CS off: %1.3f V\r\n"), adc0_v);
    float adc1_v = analogRead(ADC1)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    printf_P(PSTR("ADC1 at ICP1 TERM /w all CS off: %1.3f V\r\n"), adc1_v);
    float adc2_v = analogRead(ADC2)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    printf_P(PSTR("ADC2 at ICP3&4 TERM /W all CS off: %1.3f V\r\n"), adc2_v);
    float adc3_v = analogRead(ADC3)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    printf_P(PSTR("ADC3 at ICP3&4 TERM /W all CS off: %1.3f V\r\n"), adc3_v);
    if ( (adc0_v > 0.01)  || (adc1_v > 0.01) || (adc2_v > 0.01) || (adc3_v > 0.01))
    { 
        passing = 0; 
        printf_P(PSTR(">>> ADC is to high, is the self-test wiring right?\r\n"));
        return;
    }

    // ICP1 pin is inverted from the plug interface, its 100 Ohm Termination should have zero mA. 
    printf_P(PSTR("ICP1 input should be HIGH with 0mA loop current: %d \r\n"), digitalRead(ICP1));
    if (!digitalRead(ICP1)) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> ICP1 should be high.\r\n"));
    }

    // enable CS_ICP1 (with green LED)
    digitalWrite(CS_ICP1,HIGH);
    _delay_ms(100); // busy-wait delay

    // ICP1_TERM has CS_ICP1 on it
    float adc1_cs_icp1_v = analogRead(ADC1)*((ref_extern_avcc_uV/1.0E6)/1024.0);
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
    printf_P(PSTR("ICP1 /w 17mA on termination reads: %d \r\n"), digitalRead(ICP1));
    if (digitalRead(ICP1)) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> ICP1 should be low with 17mA.\r\n"));
    }
    digitalWrite(CS_ICP1,LOW);

    // ICP3 pin is inverted from the plug interface, its Termination should have zero mA. 
    printf_P(PSTR("ICP3 input should be HIGH with 0mA loop current: %d \r\n"), digitalRead(ICP3));
    if (!digitalRead(ICP3)) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> ICP3 should be high.\r\n"));
    }

    // enable CS_ICP3
    digitalWrite(CS_ICP3,HIGH);
    _delay_ms(100); // busy-wait delay

    // ICP3_TERM and ICP4_TERM (50Ohm) has CS_ICP3 on it
    float adc0_cs_icp3_v = analogRead(ADC0)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    float adc0_cs_icp3_i = adc0_cs_icp3_v / ICP3AND4_TERM;
    printf_P(PSTR("CS_ICP3 on ICP3AND4 TERM: %1.3f A\r\n"), adc0_cs_icp3_i);
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
    printf_P(PSTR("ICP3 /w 8mA on termination reads: %d \r\n"), digitalRead(ICP3));
    if (digitalRead(ICP1)) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> ICP3 should be low with 8mA.\r\n"));
    }

    //Swap ADC referances and find the band-gap voltage
    init_ADC_single_conversion(INTERNAL_1V1); 
    _delay_ms(100); // busy-wait delay
    int adc0_used_for_ref_intern_1v1_uV = analogRead(ADC0);
    printf_P(PSTR("   ADC0 reading used to calculate ref_intern_1v1_uV: %d int\r\n"), adc0_used_for_ref_intern_1v1_uV);
    float _ref_intern_1v1_uV = 1.0E6*1024.0 * ((adc0_cs_icp3_i * ICP3AND4_TERM) / adc0_used_for_ref_intern_1v1_uV);
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
    digitalWrite(CS_ICP3,LOW);
    _delay_ms(100); // busy-wait delay

    // ICP4 pin is inverted from the plug interface, its Termination should have zero mA. 
    printf_P(PSTR("ICP4 input should be HIGH with 0mA loop current: %d \r\n"), digitalRead(ICP4));
    if (!digitalRead(ICP4)) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> ICP4 should be high.\r\n"));
    }

    // enable CS_ICP4
    digitalWrite(CS_ICP4,HIGH);
    _delay_ms(100); // busy-wait delay

    // ICP3_TERM and ICP4_TERM (50Ohm) has CS_ICP4 on it
    float adc0_cs_icp4_v = analogRead(ADC0)*((ref_intern_1v1_uV/1.0E6)/1024.0);
    float adc0_cs_icp4_i = adc0_cs_icp4_v / ICP3AND4_TERM;
    printf_P(PSTR("CS_ICP4 on ICP3AND4 TERM: %1.3f A\r\n"), adc0_cs_icp4_i);
    if (adc0_cs_icp4_i < 0.012) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS_ICP4 curr is to low.\r\n"));
    }
    if (adc0_cs_icp4_i > 0.020) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS_ICP4 curr is to high.\r\n"));
    }

    // ICP4 pin is inverted from to the plug interface, and should have 17 mA on the combined Termination
    printf_P(PSTR("ICP4 /w 8mA on termination reads: %d \r\n"), digitalRead(ICP3));
    if (digitalRead(ICP1)) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> ICP4 should be low with 8mA.\r\n"));
    }
    digitalWrite(CS_ICP4,LOW);
    _delay_ms(100); // busy-wait delay
    
    // Input current at no load with 1V1 band-gap referance
    float input_i = analogRead(PWR_I)*((ref_intern_1v1_uV/1.0E6)/1024.0)/(0.068*50.0);
    printf_P(PSTR("PWR_I at no load use INTERNAL_1V1: %1.3f A\r\n"), input_i);
    if (input_i > 0.033) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> Input curr is to high.\r\n"));
        return;
    }
    if (input_i < 0.020) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> Input curr is to low.\r\n"));
    }

    //swap back to the AVCC referance 
    init_ADC_single_conversion(EXTERNAL_AVCC); 
    _delay_ms(100); // busy-wait delay

    // enable CS0 (through red LED)
    digitalWrite(CS0_EN,HIGH);
    _delay_ms(100); // busy-wait delay

    // CS0 drives ICP3 and ICP4 termination which should make a 50 Ohm drop
    float adc0_cs0_v = analogRead(ADC0)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    float adc0_cs0_i = adc0_cs0_v / ICP3AND4_TERM;
    printf_P(PSTR("CS0 on ICP3&4 TERM: %1.3f A\r\n"), adc0_cs0_i);
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
    digitalWrite(CS0_EN,LOW);

    // enable CS1
    digitalWrite(CS1_EN,HIGH);
    _delay_ms(100); // busy-wait delay
    
    // CS1  drives ICP3 and ICP4 termination which should make a 50 Ohm drop
    float adc0_cs1_v = analogRead(ADC0)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    float adc0_cs1_i = adc0_cs1_v / ICP3AND4_TERM;
    printf_P(PSTR("CS1 on ICP3&4 TERM: %1.3f A\r\n"), adc0_cs1_i);
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
    digitalWrite(CS1_EN,LOW);

    // enable CS2
    digitalWrite(CS2_EN,HIGH);
    _delay_ms(100); // busy-wait delay
    
    // CS2  drives ICP3 and ICP4 termination which should make a 50 Ohm drop
    float adc0_cs2_v = analogRead(ADC0)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    float adc0_cs2_i = adc0_cs2_v / ICP3AND4_TERM;
    printf_P(PSTR("CS2 on ICP3&4 TERM: %1.3f A\r\n"), adc0_cs2_i);
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
    digitalWrite(CS2_EN,LOW);

    // enable CS3
    digitalWrite(CS3_EN,HIGH);
    _delay_ms(100); // busy-wait delay
    
    // CS3  drives ICP3 and ICP4 termination which should make a 50 Ohm drop
    float adc0_cs3_v = analogRead(ADC0)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    float adc0_cs3_i = adc0_cs3_v / ICP3AND4_TERM;
    printf_P(PSTR("CS3 on ICP3&4 TERM: %1.3f A\r\n"), adc0_cs3_i);
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
    digitalWrite(CS3_EN,LOW);

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
        digitalWrite(CS_ICP1,HIGH);
    }
    else
    {
        digitalWrite(CS0_EN,HIGH);
    }
}

void blink(void)
{
    unsigned long kRuntime = millis() - blink_started_at;
    if ( kRuntime > blink_delay)
    {
        if (passing)
        {
            digitalToggle(CS_ICP1);
        }
        else
        {
            digitalToggle(CS0_EN);
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

