A program which turns on and off two LEDs periodically with predefined patterns.

This program is intended to be run with microcontroller PIC12F683.

Terminals GP0 and GP1 are connected to LEDs.
Terminal GP2 is connected to push switch.
If GP2 input level is low, the LED flash pattern changes.

The brightness of each LED is controlled by software pulse width modulation.
The brightness varies from 0 for complete off to 64 for complete on.
The frame rate is approximately 100 frames per second.
