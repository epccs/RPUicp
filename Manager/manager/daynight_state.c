/*
DayNight state machine is a library that is used to track day and night for manager functions
Copyright (C) 2019 Ronald Sutherland

All rights reserved, specifically, the right to Redistribut is withheld. Subject 
to your compliance with these terms, you may use this software and derivatives. 

Use in source and binary forms, with or without modification, are permitted 
provided that the following conditions are met:
1. Source code must retain the above copyright notice, this list of 
conditions and the following disclaimer.
2. Binary derivatives are exclusively for use with Ronald Sutherland 
products.
3. Neither the name of the copyright holders nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS SUPPLIED BY RONALD SUTHERLAND "AS IS". NO WARRANTIES, WHETHER
EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY
IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS
FOR A PARTICULAR PURPOSE.

IN NO EVENT WILL RONALD SUTHERLAND BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF RONALD SUTHERLAND
HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO
THE FULLEST EXTENT ALLOWED BY LAW, RONALD SUTHERLAND'S TOTAL LIABILITY ON ALL
CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT
OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO RONALD SUTHERLAND FOR THIS
SOFTWARE.
*/

#include <stdbool.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <avr/io.h>
#include "../lib/timers_bsd.h"
#include "../lib/uart0_bsd.h"
#include "../lib/adc_bsd.h"
#include "../lib/twi0_bsd.h"
#include "../lib/io_enum_bsd.h"
#include "i2c_callback.h"
#include "adc_burst.h"
#include "battery_manager.h"
#include "daynight_limits.h"
#include "daynight_state.h"

// allow some time for ALT_V to have valid data
#define STARTUP_DELAY 11000UL

// change to fail state if is to long
#define DAYNIGHT_TO_LONG 72000000UL

DAYNIGHT_STATE_t daynight_state;

unsigned long daynight_timer;
unsigned long daynight_timer_at_night;
unsigned long daynight_timer_at_day;
unsigned long accumulate_alt_mega_ti_at_night;
unsigned long accumulate_pwr_mega_ti_at_night;
unsigned long accumulate_alt_mega_ti_at_day;
unsigned long accumulate_pwr_mega_ti_at_day;

uint8_t daynight_callback_address;
uint8_t daynight_callback_route;
uint8_t daynight_callback_poke;
uint8_t day_work_callback_route;
uint8_t night_work_callback_route;
uint8_t daynight_fail_reported;
TWI0_LOOP_STATE_t loop_state;

/* check for day-night state durring program looping  
    with low nibble of daynight_state: range 0..7
    0 = default at startup, if above Evening Threshold set day, else set night.
    1 = day: wait for evening threshold, set for evening debounce.
    2 = evening_debounce: wait for debounce time, do night_work, however if debouce fails set back to day.
    3 = night_work: do night callback and set night.
    4 = night: wait for morning threshold, set daynight_morning_debounce.
    5 = daynight_morning_debounce: wait for debounce time, do day_work, however if debouce fails set back to night.
    6 = day_work: do day callback and set for day.
    7 = fail: fail state.

    ALT_V is used as the light sensor, it is converted like this 
    integer_from_adc*((ref_extern_avcc_uV/1.0E6)/1024.0)*(11.0/1.0))
    thus an ALT_V reading of 40 is about 2.1V
    ALT_V reading of 80 is about 4.3V
    ALT_V reading of 160 is about 8.6V
    ALT_V reading of 320 is about 17.18V
*/
void check_daynight(void)
{
    // TWI waiting to finish (ignore daynight changes while TWI is waiting)
    if (i2c_callback_waiting(&loop_state)) return; 
    
    // if somthing else is using twi then my loop_state should allow getting to this, but I want to skip this state machine
    if (i2c_is_in_use) return;

    // if daynight settins are changing skip this state machine
    if (daynight_values_loaded > DAYNIGHT_VALUES_DEFAULT) return;

    // poke, is used to update the application after it has been reset and restart daynight if it failed
    if (daynight_callback_poke)
    {
        if (daynight_callback_address && daynight_callback_route)
        {
            if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
            i2c_callback(daynight_callback_address, daynight_callback_route, daynight_state, &loop_state); // update application
        }
        daynight_callback_poke = 0;
        if (daynight_state == DAYNIGHT_STATE_FAIL) 
        {
            daynight_state = DAYNIGHT_STATE_START; // restart if daynight had failed
            daynight_fail_reported = 0;
        }
        return;
    }

    // check light on solar pannel with ALT_V, reading are only taken when !ALT_EN.
    int sensor_val = adcAtomic(MCU_IO_ALT_V);
    unsigned long kRuntime = elapsed(&daynight_timer);

    switch (daynight_state)
    {
    case DAYNIGHT_STATE_START:
        if (kRuntime > STARTUP_DELAY) 
        {
            if(sensor_val > daynight_evening_threshold ) 
            {
                daynight_state = DAYNIGHT_STATE_DAY;
                if (daynight_callback_address && daynight_callback_route)
                {
                    if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                    i2c_callback(daynight_callback_address, daynight_callback_route, daynight_state, &loop_state); // update application
                }
                daynight_timer = milliseconds();
            } 
            else 
            {
                daynight_state = DAYNIGHT_STATE_NIGHT;
                if (daynight_callback_address && daynight_callback_route)
                {
                    if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                    i2c_callback(daynight_callback_address, daynight_callback_route, daynight_state, &loop_state); // update application
                }
                daynight_timer = milliseconds();
            }
        }
        break;
    case DAYNIGHT_STATE_DAY:
        if (sensor_val < daynight_evening_threshold ) 
        {
            daynight_state = DAYNIGHT_STATE_EVENING_DEBOUNCE;
            if (daynight_callback_address && daynight_callback_route)
            {
                if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                i2c_callback(daynight_callback_address, daynight_callback_route, daynight_state, &loop_state); // update application
            }
            daynight_timer = milliseconds();
        }
        if (kRuntime > DAYNIGHT_TO_LONG) 
        {
            daynight_state = DAYNIGHT_STATE_FAIL;
            if (daynight_callback_address && daynight_callback_route)
            {
                if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                i2c_callback(daynight_callback_address, daynight_callback_route, daynight_state, &loop_state); // update remote
            }
            daynight_fail_reported = 0;
            daynight_timer = milliseconds();
        }
        break;
    case DAYNIGHT_STATE_EVENING_DEBOUNCE:
        if (sensor_val < daynight_evening_threshold ) 
        {
            if (kRuntime > daynight_evening_debounce) 
            {
                daynight_state = DAYNIGHT_STATE_NIGHTWORK;
                if (daynight_callback_address && daynight_callback_route)
                {
                    if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                    // remote daynight_state gets DAYNIGHT_STATE_NIGHT while DAYNIGHT_NIGHTWORK_STATE is used to operate night_work_callback_route
                    i2c_callback(daynight_callback_address, daynight_callback_route, DAYNIGHT_STATE_NIGHT, &loop_state); // update remote
                }
                daynight_timer = milliseconds();
            } 
        } 
        else 
        {
            daynight_state = DAYNIGHT_STATE_DAY;
            if (daynight_callback_address && daynight_callback_route)
            {
                if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                i2c_callback(daynight_callback_address, daynight_callback_route, daynight_state, &loop_state); // update remote
            }
            daynight_timer = milliseconds();
        }
        break;
    case DAYNIGHT_STATE_NIGHTWORK:
        daynight_state = DAYNIGHT_STATE_NIGHT;
        if (daynight_callback_address && night_work_callback_route)
        {
            if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
            i2c_callback(daynight_callback_address, night_work_callback_route, DAYNIGHT_STATE_NIGHTWORK, &loop_state); // night_work_callback remote
        }
        accumulate_alt_mega_ti_at_night = accumulate_alt_mega_ti;
        accumulate_pwr_mega_ti_at_night = accumulate_pwr_mega_ti;
        daynight_timer_at_night = milliseconds();
        break;
    case DAYNIGHT_STATE_NIGHT:
        if (sensor_val > daynight_morning_threshold ) 
        {
            daynight_state = DAYNIGHT_STATE_MORNING_DEBOUNCE;
            if (daynight_callback_address && daynight_callback_route)
            {
                if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                i2c_callback(daynight_callback_address, daynight_callback_route, daynight_state, &loop_state); // update remote
            }
            daynight_timer = milliseconds();
        }
        if (kRuntime > DAYNIGHT_TO_LONG) 
        {
            daynight_state = DAYNIGHT_STATE_FAIL;
            if (daynight_callback_address && daynight_callback_route)
            {
                if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                i2c_callback(daynight_callback_address, daynight_callback_route, daynight_state, &loop_state); // update remote
            }
            daynight_timer = milliseconds();
        }
        break;
    case DAYNIGHT_STATE_MORNING_DEBOUNCE:
        if (sensor_val > daynight_morning_threshold ) 
        {
            if (kRuntime > daynight_morning_debounce) 
            {
                daynight_state = DAYNIGHT_STATE_DAYWORK;
                if (daynight_callback_address && daynight_callback_route)
                {
                    if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                    // remote daynight_state gets DAYNIGHT_STATE_DAY while DAYNIGHT_STATE_DAYWORK is used to operate day_work_callback_route
                    i2c_callback(daynight_callback_address, daynight_callback_route, DAYNIGHT_STATE_DAY, &loop_state); // update remote
                }
                daynight_timer = milliseconds();
            }
        }
        else 
        {
            daynight_state = DAYNIGHT_STATE_NIGHT;
            if (daynight_callback_address && daynight_callback_route)
            {
                if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                i2c_callback(daynight_callback_address, daynight_callback_route, daynight_state, &loop_state); // update remote
            }
            daynight_timer = milliseconds();
        }
        break;
    case DAYNIGHT_STATE_DAYWORK:
        daynight_state = DAYNIGHT_STATE_DAY; // update local
        if (daynight_callback_address && day_work_callback_route)
        {
            if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
            i2c_callback(daynight_callback_address, day_work_callback_route, DAYNIGHT_STATE_DAYWORK, &loop_state); // day_work_callback remote
        }
        alt_pwm_accum_charge_time = 0; // clear charge time
        accumulate_alt_mega_ti_at_day = accumulate_alt_mega_ti;
        accumulate_pwr_mega_ti_at_day = accumulate_pwr_mega_ti;
        daynight_timer_at_day = milliseconds();
        return;
        break;
    case DAYNIGHT_STATE_FAIL: 
        if (daynight_fail_reported);
        {
            if (daynight_callback_address && daynight_callback_route)
            {
                if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                i2c_callback(daynight_callback_address, daynight_callback_route, daynight_state, &loop_state); // update remote
            }
            daynight_fail_reported = 1;
        }
        break;

    default:
        daynight_state = DAYNIGHT_STATE_FAIL;
        daynight_fail_reported = 0;
        break;
    }
}