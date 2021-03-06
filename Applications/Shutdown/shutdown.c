/*
shutdown is a library that accesses the managers R-Pi/SBC shutdown control.
Copyright (C) 2020 Ronald Sutherland

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
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "../lib/parse.h"
#include "../lib/rpu_mgr.h"
#include "../lib/rpu_mgr_callback.h"
#include "../lib/timers_bsd.h"
#include "../Adc/references.h"
#include "../DayNight/day_night.h"
#include "main.h"
#include "shutdown.h"

static unsigned long hs_serial_print_started_at;
volatile HOSTSHUTDOWN_STATE_t hs_state;
uint8_t hs_bring_up;

// /0/hs
// enable the host shutdown and set the callback so it can tell me what it is doing.
void EnableShutdownCntl(void)
{
    if ( (command_done == 10) )
    {
        switch (hs_state)
        {
        case HOSTSHUTDOWN_STATE_UP:
            hs_bring_up = 0; // take the host DOWN
            break;
        case HOSTSHUTDOWN_STATE_DOWN:
            hs_bring_up = 1; // bring the host UP
            break;

        default:
            printf_P(PSTR("{\"hs_en\":\"BUSY\"}\r\n"));
            initCommandBuffer();
            return;  // "one entry point and one exit point" is a weak idiom, I'm in the check preconditions and exit early camp
        }
        
        i2c_shutdown_cmd(I2C0_APP_ADDR, CB_ROUTE_HS_STATE, hs_bring_up);
        printf_P(PSTR("{\"hs_en\":"));
        command_done = 11;
        return;
    }
    else if ( (command_done == 11) )
    {
        if (hs_bring_up)
        {
            printf_P(PSTR("\"UP\""));
        }
        else
        {
            printf_P(PSTR("\"DOWN\""));
        }
        command_done = 12;
        return;
    }
    else if ( (command_done == 12) )
    {
        printf_P(PSTR("}\r\n"));
        initCommandBuffer();
    }
    else
    {
        initCommandBuffer();
    }
}

// /0/hscntl?
// Reports host shutdown control values. 
// hs_state, hs_halt_curr, adc_pwr_i, hs_ttl, hs_delay, hs_wearlv
void ReportShutdownCntl(unsigned long serial_print_delay_milsec)
{
    if ( (command_done == 10) )
    {
        hs_serial_print_started_at = milliseconds();
        command_done = 11;
        return;
    }
    if ( (command_done == 11) )
    {
        printf_P(PSTR("{\"hs_state\":\"0x%X\","),hs_state); // print a hex value
        command_done = 12;
        return;
    }
    else if ( (command_done == 12) ) 
    {
        int local_copy = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            local_copy = i2c_int_rwoff_access_cmd(SHUTDOWN_INT_CMD,SHUTDOWN_HALT_CURR_OFFSET,0,&loop_state);
        }
        printf_P(PSTR("\"hs_halt_curr\":"));
        if (mgr_twiErrorCode)
        {
            printf_P(PSTR("\"err%d\","),mgr_twiErrorCode);
        }
        else
        {
            printf_P(PSTR("\"%u\","),local_copy);
        }
        command_done = 13;
        return;
    }
    else if ( (command_done == 13) ) 
    {
        int adc_reads = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            adc_reads = i2c_get_adc_from_manager(ADC_CH_MGR_PWR_I,&loop_state);
        }
        printf_P(PSTR("\"adc_pwr_i\":"));
        if (mgr_twiErrorCode)
        {
            printf_P(PSTR("\"err%d\","),mgr_twiErrorCode);
        }
        else
        {
            printf_P(PSTR("\"%u\","),adc_reads);
        }
        command_done = 14;
        return;
    }
    else if ( (command_done == 14) ) 
    {
        unsigned long local_copy = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            local_copy = i2c_ul_rwoff_access_cmd(SHUTDOWN_UL_CMD,SHUTDOWN_TTL,0,&loop_state);
        }
        printf_P(PSTR("\"hs_ttl\":"));
        if (mgr_twiErrorCode)
        {
            printf_P(PSTR("\"err%d\","),mgr_twiErrorCode);
        }
        else
        {
            printf_P(PSTR("\"%lu\","),local_copy);
        }
        command_done = 15;
        return;
    }
    else if ( (command_done == 15) ) 
    {
        unsigned long local_copy = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            local_copy = i2c_ul_rwoff_access_cmd(SHUTDOWN_UL_CMD,SHUTDOWN_DELAY,0,&loop_state);
        }
        printf_P(PSTR("\"hs_delay\":"));
        if (mgr_twiErrorCode)
        {
            printf_P(PSTR("\"err%d\","),mgr_twiErrorCode);
        }
        else
        {
            printf_P(PSTR("\"%lu\","),local_copy);
        }
        command_done = 16;
        return;
    }

    else if ( (command_done == 16) ) 
    {
        unsigned long local_copy = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            local_copy = i2c_ul_rwoff_access_cmd(SHUTDOWN_UL_CMD,SHUTDOWN_WEARLEVEL,0,&loop_state);
        }
        printf_P(PSTR("\"hs_wearlv\":"));
        if (mgr_twiErrorCode)
        {
            printf_P(PSTR("\"err%d\","),mgr_twiErrorCode);
        }
        else
        {
            printf_P(PSTR("\"%lu\","),local_copy);
        }
        command_done = 17;
        return;
    }
    else if ( (command_done == 17) ) 
    {
        unsigned long local_copy = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            local_copy = i2c_ul_rwoff_access_cmd(SHUTDOWN_UL_CMD,SHUTDOWN_KRUNTIME,0,&loop_state);
        }
        printf_P(PSTR("\"hs_timer\":")); // elapsed timer used by state machine
        if (mgr_twiErrorCode)
        {
            printf_P(PSTR("\"err%d\""),mgr_twiErrorCode);
        }
        else
        {
            printf_P(PSTR("\"%lu\""),local_copy);
        }
        command_done = 24;
        return;
    }
    else if ( (command_done == 24) ) 
    {
        printf_P(PSTR("}\r\n"));
        command_done = 25;
        return;
    }
    else if ( (command_done == 25) ) 
    {
        unsigned long kRuntime= elapsed(&hs_serial_print_started_at);
        if ((kRuntime) > (serial_print_delay_milsec))
        {
            hs_serial_print_started_at += serial_print_delay_milsec;
            command_done = 11; /* This keeps looping output forever (until a Rx char anyway) */
        }
    }
} 

// /0/hshaltcurr? [1..1024]
// Befor host shutdown is done PWR_I current must be bellow this limit.
void ShutdownHaltCurrLimit(void)
{
    if ( (command_done == 10) )
    {
        // if got argument[0] check its rage (it is used with 10 bit ADC)
        if (arg_count == 1)
        {
            if ( ( !( isdigit(arg[0][0]) ) ) || (atoi(arg[0]) < 0) || (atoi(arg[0]) >= (1<<10) ) )
            {
                printf_P(PSTR("{\"err\":\"HSHaltCurrMax 1023\"}\r\n"));
                initCommandBuffer();
                return;
            }
            command_done = 11;
        }
        else
        {
            command_done = 12;
        }
        printf_P(PSTR("{\"hs_halt_curr\":")); // shutdown_halt_curr_limit on manager
    }
    if ( (command_done == 11) ) 
    {
        int int_to_send = 0;
        if (arg_count == 1) int_to_send = atoi(arg[0]);
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            // update the manager, the old value is returned, but not needed
            i2c_int_rwoff_access_cmd(SHUTDOWN_INT_CMD,SHUTDOWN_HALT_CURR_OFFSET+RW_WRITE_BIT,int_to_send,&loop_state);
        }
        command_done = 12;
        return;
    }
    if ( (command_done == 12) ) 
    {
        int int_to_get = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            int_to_get = i2c_int_rwoff_access_cmd(SHUTDOWN_INT_CMD,SHUTDOWN_HALT_CURR_OFFSET+RW_READ_BIT,0,&loop_state);
        }
        printf_P(PSTR("\"%u\"}\r\n"),int_to_get);
        initCommandBuffer();
        return;
    }
}

// /0/hsttl? [value]
// Time to wait for PWR_I to be bellow shutdown_halt_curr_limit and then stable for wearleveling
void ShutdownTTLimit(void)
{
    if ( (command_done == 10) )
    {
        // if got argument[0] check
        if (arg_count == 1)
        {
            if ( !( isdigit(arg[0][0]) ) )
            {
                printf_P(PSTR("{\"err\":\"HSttl NaN\"}\r\n"));
                initCommandBuffer();
                return;
            }
            command_done = 11;
        }
        else
        {
            command_done = 12;
        }
        printf_P(PSTR("{\"hs_ttl\":")); // shutdown_ttl_limit on manager
        return;
    }
    if ( (command_done == 11) ) 
    {
        unsigned long ul_to_send = 0;
        if (arg_count == 1) ul_to_send = strtoul(arg[0], (char **)NULL, 10);
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            // update the manager, disregard the old value
            i2c_ul_rwoff_access_cmd(SHUTDOWN_UL_CMD,SHUTDOWN_TTL+RW_WRITE_BIT,ul_to_send,&loop_state);
        }
        command_done = 12;
        return;
    }
    if ( (command_done == 12) ) 
    {
        unsigned long ul_to_get = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            ul_to_get = i2c_ul_rwoff_access_cmd(SHUTDOWN_UL_CMD,SHUTDOWN_TTL+RW_READ_BIT,0,&loop_state);
        }
        printf_P(PSTR("\"%lu\"}\r\n"),ul_to_get);
        initCommandBuffer();
        return;
    }
}

// /0/hsdelay? [value]
// Time to wait after droping bellow shutdown_halt_curr_limit, but befor checking wearleveling for stable readings.
void ShutdownDelayLimit(void)
{
    if ( (command_done == 10) )
    {
        // if got argument[0] check
        if (arg_count == 1)
        {
            if ( !( isdigit(arg[0][0]) ) )
            {
                printf_P(PSTR("{\"err\":\"HSdelay NaN\"}\r\n"));
                initCommandBuffer();
                return;
            }
            command_done = 11;
        }
        else
        {
            command_done = 12;
        }
        printf_P(PSTR("{\"hs_delay\":")); // shutdown_delay_limit on manager
        return;
    }
    if ( (command_done == 11) ) 
    {
        unsigned long ul_to_send = 0;
        if (arg_count == 1) ul_to_send = strtoul(arg[0], (char **)NULL, 10);
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            // update the manager, disregard the old value
            i2c_ul_rwoff_access_cmd(SHUTDOWN_UL_CMD,SHUTDOWN_DELAY+RW_WRITE_BIT,ul_to_send,&loop_state);
        }
        command_done = 12;
        return;
    }
    if ( (command_done == 12) ) 
    {
        unsigned long ul_to_get = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            ul_to_get = i2c_ul_rwoff_access_cmd(SHUTDOWN_UL_CMD,SHUTDOWN_DELAY+RW_READ_BIT,0,&loop_state);
        }
        printf_P(PSTR("\"%lu\"}\r\n"),ul_to_get);
        initCommandBuffer();
        return;
    }
}

// /0/hswearlv? [value]
// Time PWR_I must be stable for.
void ShutdownWearlevelingLimit(void)
{
    if ( (command_done == 10) )
    {
        // if got argument[0] check
        if (arg_count == 1)
        {
            if ( !( isdigit(arg[0][0]) ) )
            {
                printf_P(PSTR("{\"err\":\"HSwearlv NaN\"}\r\n"));
                initCommandBuffer();
                return;
            }
            command_done = 11;
        }
        else
        {
            command_done = 12;
        }
        printf_P(PSTR("{\"hs_wearlv\":")); // shutdown_wearleveling_limit on manager
        return;
    }
    if ( (command_done == 11) ) 
    {
        unsigned long ul_to_send = 0;
        if (arg_count == 1) ul_to_send = strtoul(arg[0], (char **)NULL, 10);
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            // update the manager, disregard the old value
            i2c_ul_rwoff_access_cmd(SHUTDOWN_UL_CMD,SHUTDOWN_WEARLEVEL+RW_WRITE_BIT,ul_to_send,&loop_state);
        }
        command_done = 12;
        return;
    }
    if ( (command_done == 12) ) 
    {
        unsigned long ul_to_get = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            ul_to_get = i2c_ul_rwoff_access_cmd(SHUTDOWN_UL_CMD,SHUTDOWN_WEARLEVEL+RW_READ_BIT,0,&loop_state);
        }
        printf_P(PSTR("\"%lu\"}\r\n"),ul_to_get);
        initCommandBuffer();
        return;
    }
}
