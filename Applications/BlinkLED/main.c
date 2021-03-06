/* Blink LED
Copyright (C) 2017 Ronald Sutherland

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
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "../lib/uart0_bsd.h"
#include "../lib/io_enum_bsd.h"
#include "../lib/timers_bsd.h"
#include "../lib/twi1_bsd.h"

#define BLINK_DELAY 1000UL
static unsigned long blink_started_at;

static int got_a;

void setup(void) 
{
    ioDir(MCU_IO_CS0_EN,DIRECTION_OUTPUT);
    ioWrite(MCU_IO_CS0_EN,LOGIC_LEVEL_HIGH);

    /* Initialize UART to 38.4kbps, it returns a pointer to FILE so redirect of stdin and stdout works*/
    stderr = stdout = stdin = uart0_init(38400UL, UART0_RX_REPLACE_CR_WITH_NL);

    //Timer0 Fast PWM mode, Timer1 & Timer2 Phase Correct PWM mode.
    initTimers(); 

    /* Initialize I2C*/
    twi1_init(100000UL, TWI1_PINS_PULLUP);

    sei(); // Enable global interrupts to start TIMER0
    
    blink_started_at = milliseconds();
    
    got_a = 0;
}

// don't block (e.g. _delay_ms(1000) ), just ckeck if time has elapsed to toggle 
void blink(void)
{
    unsigned long kRuntime = elapsed(&blink_started_at);
    if ( kRuntime > BLINK_DELAY)
    {
        ioToggle(MCU_IO_CS0_EN);
        
        // next toggle 
        blink_started_at += BLINK_DELAY; 
    }
}

// abort++. 
void abort_safe(void)
{
    // make sure pins are safe befor waiting on UART 
    ioDir(MCU_IO_CS0_EN,DIRECTION_OUTPUT);
    ioWrite(MCU_IO_CS0_EN,LOGIC_LEVEL_LOW);
    // flush the UART befor halt
    uart0_flush();
    _delay_ms(20); // wait for last byte to send
    uart0_init(0, 0); // disable UART hardware 
    // turn off interrupts and then spin loop a LED toggle 
    cli();
    while(1) 
    {
        _delay_ms(100); 
        ioToggle(MCU_IO_CS0_EN);
    }
    
    /*sleep notes, not yet done
    #include <avr/sleep.h>
    #include <avr/wdt.h>
    
    for (i = 0; i < 20; i++) {             // all pins to one rail or the other
        ioDir(i,DIRECTION_OUTPUT);
        ioWrite(i,LOGIC_LEVEL_LOW);
    }
    ADCSRA = 0;                            // disable ADC for power saving
    wdt_disable();                         // disable WDT for power saving
    PRR = 0xEF;                            // power down all modules m328p
    set_sleep_mode (SLEEP_MODE_PWR_DOWN);  // Deep sleep
    sleep_enable();
    sleep_bod_disable();                   // disable brownout detector during sleep
    sleep_cpu();                           // now go to sleep
    
    while(1) {
    }
    
    // or 
    // Prepare the sleep in power down mode
    SMCR |= (1<<SE) | (0<<SM2) | (1<<SM1) | (0<<SM0);
    // Disable brown out detection in sleep
    tmp = MCUCR | (1<<BODS) | (1<<BODSE);
    MCUCR = tmp;
    MCUCR = tmp & (~(1<<BODSE));
    // delay_us(100); //note a delay will cause sleep to fail
    // Enter sleep mode
    #asm("sleep");
    */
    
}

int main(void)
{
    setup(); 
    
    int abort_yet = 0;
    
    while (1) 
    {
        if(uart0_available())
        {
            // standard C has a libc function getchar() 
            // which gets a byte from stdin.
            // Since I redirected stdin to be from the UART0 this works.
            int input = getchar();

            // standard C has a libc function printf() 
            // which sends a formated string to stdout.
            // stdout was also redirected to UART0, so this also works.
            printf("%c\r", input); 

            if (input == '$') 
            {
                // Variant of printf() that uses a format string that resides in flash memory.
                printf_P(PSTR("{\"abort\":\"egg found\"}\r\n")); 
                abort_safe();
            }

            // press 'a' to stop blinking.
            if(input == 'a') 
            {
                got_a = 1; 
                ++abort_yet; 
            }
            else
            {
              got_a = 0;
            }

            // press 'a' more than five times to hault
            if (abort_yet >= 5) 
            {
                abort_safe();
            }
        }
        if (!got_a)
        {
            blink();
        }
    }    
}

