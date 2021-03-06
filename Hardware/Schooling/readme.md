# Description

Some lessons I learned doing Gravimetric.

# Table Of Contents:

1. ^1 Move Resistor So Manager Can Lockout Manual Switch
1. ^1 I2C Use Of Invalid Values
1. ^1 Reflow Disaster
1. ^0 N-CH Cutoff
1. ^0 Power Protection 
1. ^0 Manager Should Do More
1. ^0 Alternat Power Diode


## ^1 Move Resistor So Manager Can Lockout Manual Switch

Move resistor (R237) so that it limits current to the switch from a strong pull that can override a manual operation. A weak pull up is used to allow the switch to control shutdown.

![MoveR237](./Gravimetric^1,Move_R237.png "Move R237")

![R237LayoutRef](./Gravimetric^1,Gravimetric^1,R237_LayoutRef.png "R237 LayoutRef")

I see how to do this one in a drawing. 

![R237Rework](./Gravimetric^1,Rework_R237.jpg "R237 Rework")

That went Ok I guess, need to add some hot glue to keep it in place.


## ^1 I2C Use Of Invalid Values

daynight_morning_debounce and daynight_evening_debounce i2c receive events were saving invalid numbers and then testing them when they tried saving to EEPROM. They need to be tested in the TWI receive event so that invalid data is not placed where the state machine (or other users) can access it. Also, the function that saves to EEPROM does not restore defaults of invalid data at this time, the bloat of restoring an invalid value looks worse than checking in the TWI event.

I need to look things over and make sure this is not happening with other values.


## ^1 Reflow Disaster

When humidity is high, it may be more productive to procrastinate. Almost 30% of the parts went flying. I have seen this before, but this time I did it to my board, I stopped at the start of the flux activation stage when I recognized what was happening. It reflowed after sorting through the missing parts, and some preheat treatment at about 80 deg C to dry things before going through the typical profile. Do I need to log the RH and reflow results, probably that would be useful to figure out what range I can tolerate.


## ^0 N-CH Cutoff

Q105 and Q118 need to go into cut off (or hi-z) when the MOSFET gate is low. The source and drain need to be swapped to work correctly, and sadly I have ^1 on order, so that means I will have to fix them on ^2.


## ^0  Power Protection

U1 provides ALT_I curr sense but it causes power protection to fail.

Move U1 sense (R4) resistor between Q2 and D1 so that Q2 can block a reverse polarity.

![MoveR4](./Gravimetric^0,Move_R4.png "Move R4")

![R4LayoutRef](./Gravimetric^0,R4_LayoutRef.png "R4 LayoutRef")

I am not sure how to show a drawing of what needs to be done so it is time to improvise. 

![R4Rework](./Gravimetric^0,Rework_R4.png "R4 Rework")

That is a hack, and not clear what I did. D1 was never placed, so that is where I cut the trace between Q2 and D1. That node (between D1 and Q2) has reverse polarity protection now, and it should also protect the alternate power input as long as ALT_EN is not turned on when alternate power is reversed.


## ^0  Manager Should Do More

Analog: ALT_V, ALT_I, PWR_V, and PWR_I. Digital SHLD_VIN_EN and ALT_EN. If I can move some inputs around and use some of the ICSP pins, then the manager can take care of these functions, so the user program does not need to. Moving those functions will also take the R-Pi shutdown out of the hands of the user, which is probably an excellent idea.


## ^0  Alternat Power Diode

The MBRD1045 will protect a solar panel if it is connected wrong but will get too hot with about 3A. The IRFR5305 should be able to handle about 5A before it gets too hot. Replacing the MBRD1045 with an IRFR5305 requires that the body diode line up with the replaced diode and the gate can be controlled with the existing gate control (which has a 7V Zener from the NPN base to emitter).  The problem is that the battery can be shorted to a wrong connected solar panel, so that needs to difficult to enable with an improper connection.  


