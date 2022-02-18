#!/bin/sh

# Enable GPIO pins for digital inputs (buttons) on Raspberry Pi
echo 22 > /sys/class/gpio/export
echo 27 > /sys/class/gpio/export

# GPIO pins for digital outputs (LEDs) are enabled in another script
