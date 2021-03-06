# To Do

```
(done) change cmd 2 and 3 to use one cmd 2 to access the bootload address
(done) change Adc application to use manager data: channels (ALT_I, ALT_V,PWR_I,PWR_V), referance, and calibration.
(done) change cmd 4 and 5 to use one cmd 4 to access shutdown of host
(done) change cmd 6 and 7 to use one cmd 6 to an access status of manager
(done) try to do daynight state machine so it acts as a i2c master to send state change and work events once enabled (e.g., multi-master)
(done) Timed Accumulation needs a half LSB added (e.g. an LSB every other accumulation) because the ADC max value represents the selected reference voltage minus one LSB.
(done) Timed Accumulation overflows to soon.
(done) change cmd 16 and 17 to use one cmd 16 to access point2point mode of host
(done) move cmd 16 to 3, rename PointToPoint.md to PVandBattery.md
(done) daynight state enum
(done) PowerManagement.md to Analog.md
(done) move alt_enable from status bit 4 to its own cmd 16 to setup callback
(done) alt_enable name changed to enable_alternate_callback_address, and holds callback
(done) when setting enable_alternate_callback_address init the state machine so that a callback update occurs
(done) bm pwm mode high time/low time needs to be locked in at the start of a period, it has problems otherwise
(done) verify battery_manager with Battery applicaiton
(nix) Turn on enable_alternate_power and clear alt_pwm_accum_charge_time when daynight state is at DAYNIGHT_DAYWORK_STATE
(nix) Turn off enable_alternate_power when daynight state is at DAYNIGHT_NIGHTWORK_STATE
(done) bm runs only when night_state == DAYNIGHT_STATE_DAY
(nix) A status bit 4 write sets enable_alternate_power and clears alt_pwm_accum_charge_time, but is that a good approch?
(bug found and fixed) reading after seting a value returns zero, (daynight_morning_debounce)
(done) remove status bit 4 (report alternat power)
(done) remvoe status bit 5 (report SBC power)
(done) remove status bit 6 (report daynight state machine fail)
(done) if manager resets application then clear enable_bm_callback_address and daynight_callback_address
(done) remove shutdown_detect since the shutdown manager will be controling it
(done) move status cmd 6 to cmd 1 which is not used
(done) i2c cmd 4 used to set Host Shutdown i2c callback
(done) i2c cmd 5 used to access shutdown_halt_curr_limit
(done) i2c cmd 6 used to access shutdown[_halt_ttl_limit|delay_limit|wearleveling_limit]
(done) sd_state is stuck on CURR_CHK (sd_state = 3) when going DOWN. Was loading uninitialized EEPROM into the limit values.
(done) hs_timer was not available from manager.
(done) shutdown application to check shutdown state machine
(done) move battery limits in EEPROM
(done) cmd 17 (fnBatteryIntAccess) access battery manager uint16 values. battery_[high_limit|low_limit|host_limit]
(done) remove cmd 18 since cmd 17 does battery_low_limit
(done) remove cmd 19 since cmd 17 does battery_high_limit
(done) cmd 18 (fnBatteryULAccess) access battery manager unsigned long values. alt_pwm_accum_charge_time
(done) remove cmd 20 since cmd 18 does alt_pwm_accum_charge_time
(done) move cmd 23 to cmd 19 does alt_pwm_accum_charge_time
(done) cmd 20 (fnDayNightIntAccess) access daynight manager uint16 values. daynight_[morning_threshold|evening_threshold]
(done) remove cmd 21 since cmd 20 does daynight_morning_threshold
(done) remove cmd 22 since cmd 20 does daynight_evening_threshold
(done) cmd 21 (fnDayNightULAccess) access daynight manager uint32 values. daynight_[morning_debounce|evening_debounce|timer]
(done) remove cmd 52 since cmd 21 does daynight_morning_debounce
(done) remove cmd 53 since cmd 21 does daynight_evening_debounce
(done) remove cmd 54 since cmd 21 offset 2 does elapsed daynight_timer
(done) add to cmd 21 elapsed daynight_timer_at_night at offset 3
(done) add to cmd 21 elapsed daynight_timer_at_day at offset 4
(done) add to cmd 21 accumulate_alt_mega_ti_at_night at offset 5 
(done) add to cmd 21 accumulate_pwr_mega_ti_at_night at offset 6 
(done) add to cmd 21 accumulate_alt_mega_ti_at_day at offset 7 
(done) add to cmd 21 accumulate_pwr_mega_ti_at_day at offset 8 
(done) cmd 16 (bm enable), add a byte to enable/disable
(done) daynight state machine over loads i2c when reporting fail state
(done) battery_manager should do pwm over range battery_low_limit to battery_high_limit
(done) cmd 18 with alt_pwm_accum_charge_time is not accumulating
(done) battery manager (in BATTERYMGR_STATE_CC_MODE) should halt the host if it is UP and battery < battery_halt_limit
(done) PWM at battery low limit is a discontinuity (should transition from 100% to 90% then to 10% continuous).
(done) set up i2c callback poke for shutdown (byte[3] = bring host UP[1], take host DOWN[0], poke[2..254].)
(done) set up i2c callback poke for bm (byte[3] = enable bm [1], disable bm [0], poke bm [2..254].)
(done) sbc power cntl, ioWrite(MCU_IO_PIPWR_EN,LOGIC_LEVEL_HIGH), /hs command should turn off SBC power
(done) bm daemon will now see low bat if is not charging
(done) host will have power if manager is in rest, so make that the default at power up (do not use manual swithc)
(done) board needs hack to allow manual switch to be locked out by the manager
(done) lockout the manual shutdown switch for some time so the host can finish booting, this needs above board hack. 
(done) add low battery bit to status and set it when the host shutdown process is triggered.
(done) set bm_enable durring setup, it will not operate unless the ALT input has power
(done) status battery low bit should not be set unless voltage is low (oops) 
(npf) init status for daynight is stuck. The applicaion was change to show START state better.
(done) bm loops on FAIL state rather than changing to START
(done) bm_enable is not reported but I can infer it has been enabled based on bm_state.
(doc) bm_enable reports on bit 8 of bm state machine updates 
(done) at power up app was getting bm_state 5 rather than 3 (i2c was setting bm_enable to 2 and bm daemon as adding state and enable to get 5).
(test) cmd 18 offset 0 is alt_pwm_accum_charge_time, an approximation for absorption time, it needs to be check with a battery.
(???) if eeprom value set and the battery goes lower than _host_limit-(_host_limit>>4) manager can hold application in reset and sleep.
```

??? - need to do testing and debug, then see if there is room.


# Manager

This firmware is for the board manager. 


## Overview

The manager operates the multi-drop serial so that the host can bootload an application controller (point-to-point) or communicate with the connected application controllers (point-to-multipoint). It can sense voltage and current on both the input and auxiliary input as well as control the auxiliary input and SBC power output. It operates a simple battery manager state machine for charging. Also, it operates an SBC shutdown state machine from the manual switch or commands from the SBC or application as well as from the battery manager. A simple day-night state machine is also operated when the auxiliary power input is connected to a solar panel. The software is a work in progress and so is its testing.


## Multi-Drop Serial

In normal mode, the RX and TX signals (each a twisted pair) are connected through transceivers to the application microcontroller UART (RX and TX pins) while the DTR pair is connected to the manager UART and is used to set the system-wide serial bus state (e.g., point-to-point or point-to-multipoint).

During lockout mode, the application controller RX and TX serial lines are disconnected by way of the transceivers control lines that are connected to the manager. Use LOCKOUT_DELAY to set the time in mSec.

Bootload mode occurs when a byte (and its check) on the DTR pair matches the RPU_ADDRESS of the shield. It will cause a pulse on the reset pin to activate the bootloader on the local microcontroller board. After the bootloader is done, the local microcontroller will run its application, which, if it reads the RPU_ADDRESS, will cause the manager to send an RPU_NORMAL_MODE byte on the DTR pair. The RPU_ADDRESS is read with a command from the controller board. If the RPU_ADDRESS is not read, the bootload mode will timeout but not connect the RX and TX transceivers to the local application controller. Use BOOTLOADER_ACTIVE to set the time in mSec; it needs to be less than LOCKOUT_DELAY.

The lockout mode occurs when a byte on the DTR pair does not match the RPU_ADDRESS of the manager. It will cause the lockout condition and last for a duration determined by the LOCKOUT_DELAY or when an RPU_NORMAL_MODE byte (and its check) is seen on the DTR pair.

When nRTS is active, the manager will control the transceiver to connect the HOST_TX, and HOST_RX lines to the multi-drop RX and TX twisted pairs. The manager will pull the nCTS line active to let the host know it is Ok to send. If the multi-drop bus has another active host, the local nRTS will be ignored, and the transceiver will remain disconnected. Note the manager firmware sets a status bit at startup that prevents the host from connecting until it is cleared with an I2C command.

Arduino Mode is a permanent bootload mode so that the IDE can connect to a specific address (e.g., point to point). It needs to be enabled by I2C or SMBus. The command will cause a byte to be sent on the DTR pair that sets the arduino_mode, thus overriding the lockout timeout and the bootload timeout (e.g., lockout and bootload mode are everlasting). 

Test Mode. I2C command to swithch to test_mode (save trancever control values). I2C command to recover trancever control bits after test_mode.

Power Management commands allow reading ADC channels, saving reference values, battery limits, day-night state machine threshold and status. 


## Multi-Master i2C

The application starts as the only master on the private I2C bus between the manager and application. If the application controller enables callbacks (day-night state machine, battery charger) then the related events will be sent from the manager as an I2C master to the application controller as a slave.

The advantage of doing this event propagation with a multi-master mode is the nearly instantaneous flow from the manager to the application. The disadvantage is complexity.


## Analog 

Analog channels are connected to both primary and alternate power inputs. The primary power input has ADC channels PWR_V and PWR_I that allow gauging the power usage by the board. The alternate power input has ADC channels ALT_V and ALT_I for gauging power from a charging source, for example, if primary power is from a battery and alternate is from a charger. The alternate input needs to be a current source (charger, PV, TEG, or ilk), not a voltage source.

Timed Accumulation is work in progress.

Referances are multi-byte floats. The idea is to allow access to the values so they can be used with channel corrections.

Each channel has a correction that, when combined with the reference (and timed accumulation), will give the corrected value.


## Firmware Upload

Use an ICSP tool connected to the bus manager (set the ISP_PORT in Makefile) run 'make' to compile then 'make isp' to flash the bus manager (Notes of fuse setting included).

```
sudo apt-get install make git gcc-avr binutils-avr gdb-avr avr-libc avrdude
git clone https://github.com/epccs/Gravimetric/
cd /Gravimetric/Manager/manager
make
make isp
...
avrdude -v -p atmega328pb -C +../lib/avrdude/328pb.conf -c stk500v1 -P /dev/ttyACM0 -b 19200 -U eeprom:r:manager_atmega328pb_eeprom.hex:i
...
avrdude: safemode: Fuses OK (E:F7, H:D9, L:62)
...
avrdude -v -p atmega328pb -C +../lib/avrdude/328pb.conf -c stk500v1 -P /dev/ttyACM0 -b 19200 -e -U lock:w:0xFF:m -U lfuse:w:0xFF:m -U hfuse:w:0xD6:m -U efuse:w:0xFD:m
...
avrdude: safemode: Fuses OK (E:FD, H:D6, L:FF)
...
avrdude -v -p atmega328pb -C +../lib/avrdude/328pb.conf -c stk500v1 -P /dev/ttyACM0 -b 19200 -e -U flash:w:manager.hex -U lock:w:0xEF:m
...
avrdude done.  Thank you.
```

## R-Pi Prep

```
sudo apt-get install i2c-tools python3-smbus
sudo usermod -a -G i2c your_username
# logout for the change to take
```


## RPUbus Addressing

The Address '1' on the multidrop serial bus is 0x31, (e.g., not 0x1 but the ASCII value for the character).

When HOST_nRTS is pulled active from a host trying to connect to the serial bus, the local bus manager will set localhost_active and send the bootloader_address over the DTR pair. If an address received by way of the DTR pair matches the local RPU_ADDRESS the bus manager will enter bootloader mode (marked with bootloader_started), and connect the shield RX/TX to the RS-422 (see connect_bootload_mode() function), all other addresses are locked out. After a LOCKOUT_DELAY time or when a normal mode byte is seen on the DTR pair, the lockout ends and normal mode resumes. The node that has bootloader_started broadcast the return to normal mode byte on the DTR pair when that node has the RPU_ADDRESS read from its bus manager over I2C (otherwise it will time out and not connect the controller RX/TX to serial).


## Bus Manager Modes

In Normal Mode, the manager connects the local application controller (ATmega324pb) to the RPUbus if it is RPU aware (e.g., ask for RPU_ADDRESS over I2C). Otherwise, it will not attach the local MCU. The host side of the transceivers connects unless it is foreign (e.g., keep out the locals when foreigners are around).

In bootload mode, the manager connects the local application controller to the multi-drop serial bus. Also, the host will be connected unless it is foreign. It is expected that all other nodes are in lockout mode. Note the BOOTLOADER_ACTIVE delay is less than the LOCKOUT_DELAY, but it needs to be in bootload mode long enough to allow uploading. A slow bootloader will require longer delays.

In lockout mode, if the host is foreign, both the local controller and Host are disconnected from the bus. Otherwise, the host remains connected.


## I2C and SMBus Interfaces

There are two TWI interfaces one acts as an I2C slave and is used to connect with the local microcontroller, while the other is an SMBus slave and connects with the local host (e.g., an R-Pi.) The commands sent are the same in both cases, but Linux does not like repeated starts or clock stretching so the SMBus read is done as a second bus transaction. I'm not sure the method is correct, but it seems to work, I echo back the previous transaction for an SMBus read. The masters sent (slave received) data is used to size the reply, so add a byte after the command for the manager to fill in with the reply. The I2C address is 0x29 (dec 41) and SMBus is 0x2A (dec 42). It is organized as an array of commands. 


[Point To Multi-Point] commands 0..15 (Ox00..0x0F | 0b00000000..0b00001111)

[Point To Multi-Point]: ./PointToMultiPoint.md

0. Access the multi-drop address, range 48..122 (ASCII '0'..'z').
1. Access status bits.
2. Access the multi-drop bootload address that will be sent when DTR/RTS toggles.
3. Access arduino_mode.
4. Set host shutdown i2c callback (set shutdown_callback_address and shutdown_callback_route).
5. Access shutdown manager uint16 values. shutdown_halt_curr_limit
6. Access shutdown manager uint32 values. shutdown_[halt_ttl_limit|delay_limit|wearleveling_limit]
7. not used.

[PV and Battery] Management commands 16..31 (Ox10..0x1F | 0b00010000..0b00011111)

[PV and Battery]: ./PVandBattery.md

16. Set battery manager bm_callback_address (i2c) and bm_callback_route.
17. Access battery manager uint16 values. battery_[high_limit|low_limit|host_limit]
18. Access battery manager uint32 values. alt_pwm_accum_charge_time
19. Set daynight_callback_address and routs [daynight|day_work|night_work]_callback_route.
20. Access daynight manager uint16 values. daynight_[morning_threshold|evening_threshold]
21. Access daynight manager uint32 values. daynight_[morning_debounce|evening_debounce|...]
22. not used.
23. not used.

Note: arduino_mode is point to point.


[Analog] commands 32..47 (Ox20..0x2F | 0b00100000..0b00101111)

[Analog]: ./Analog.md

32. adc[channel] (uint16_t: send channel (ALT_I, ALT_V,PWR_I,PWR_V), return adc reading)
33. calMap[channelMap.cal_map[channel]] (uint8_t+uint32_t: send channel (ALT_I+CALIBRATION_SET) and float (as uint32_t), return channel and calibration)
34. not used.
35. not used.
36. analogTimedAccumulation for (uint32_t: send channel (ALT_IT,PWR_IT), return reading)
37. not used.
38. Analog referance for EXTERNAL_AVCC (uint32_t)
39. Analog referance for INTERNAL_1V1 (uint32_t)


[Test Mode] commands 48..63 (Ox30..0x3F | 0b00110000..0b00111111)

[Test Mode]: ./TestMode.md

48. save trancever control bits HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE for test_mode.
49. recover trancever control bits after test_mode.
50. read trancever control bits durring test_mode, e.g. 0b11101010 is HOST_nRTS = 1, HOST_nCTS =1, DTR_nRE =1, TX_nRE = 1, TX_DE =0, DTR_nRE =1, DTR_DE = 0, RX_nRE =1, RX_DE = 0.
51. set trancever control bits durring test_mode, e.g. 0b11101010 is HOST_nRTS = 1, HOST_nCTS =1, TX_nRE = 1, TX_DE =0, DTR_nRE =1, DTR_DE = 0, RX_nRE =1, RX_DE = 0.
52. not used.
53. not used.
54. not used.
55. not used.

Note: debounce is for day-night state machine (it is not a test thing and may move).


Connect to i2c-debug on an RPUno with an RPU shield using picocom (or ilk). 

``` 
picocom -b 38400 /dev/ttyUSB0
``` 


## Scan I2C0 with i2c-debug

Scan for the address on I2C0.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/id?
{"id":{"name":"I2Cdebug^1","desc":"RPUno (14140^9) Board /w atmega328p","avr-gcc":"5.4.0"}}
/1/iscan?
{"scan":[{"addr":"0x29"}]}
```


## Scan I2C1 with Raspberry Pi

The I2C1 port is for SMBus access. A Raspberry Pi is set up as follows.

```
i2cdetect 1
WARNING! This program can confuse your I2C bus, cause data loss and worse!
I will probe file /dev/i2c-1.
I will probe address range 0x03-0x77.
Continue? [Y/n] Y
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- -- -- -- -- -- -- -- --
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
20: -- -- -- -- -- -- -- -- -- -- 2a -- -- -- -- --
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
70: -- -- -- -- -- -- -- --
```


# EEPROM Memory map 

A map of valuses in EEPROM. 

```
function            type        ee_addr:
id                  UINT16      30
ref_extern_avcc     UINT32      32
ref_intern_1v1      UINT32      36
"RPUid\0"           ARRAY       40
md_serial_addr      UINT8       50
morning_threshold   UINT16      70
evening_threshold   UINT16      72
morning_debounce    UINT32      74
evening_debounce    UINT32      78
calibration_0       UINT32      82
calibration_1       UINT32      86
calibration_2       UINT32      90
calibration_3       UINT32      94
calibration_4*      UINT32      98
calibration_5*      UINT32      102
calibration_6*      UINT32      106
calibration_7*      UINT32      110
calibration_8*      UINT32      114
calibration_9*      UINT32      118
calibration_10*     UINT32      122
calibration_11*     UINT32      126
shutdown_halt_curr  UINT16      130
shutdown_ttl        UINT32      132
shutdown_delay      UINT32      136
shutdown_wearlevel  UINT32      140
bat_high_limit      UINT16      150
bat_low_limit       UINT16      152
bat_host_limit      UINT16      154
```

Some values are reserved (*)

The AVCC pin is used to power the analog to digital converter and is also used as a reference. The AVCC pin is powered by a switchmode supply that can be measured and used as a reference.

The internal 1V1 bandgap is not trimmed by the manufacturer, so it is nearly useless until it is measured. However, once it is known it is a good reference.

The SelfTest loads calibration values for references into EEPROM.

The multi drop serial rpu_address default is ASCII '0', e.g., from python ord('0'). If the Array "RPUid\0" is programmed in eeprom then the serial rpu_address is loaded from the md_serial_addr eeprom location after power up.

