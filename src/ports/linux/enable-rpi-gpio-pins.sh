#!/bin/sh

# Avoid exporting gpios several times
gpio_export() {
    if ! [ -d /sys/class/gpio/gpio$1 ]; then
        echo $1 > /sys/class/gpio/export
    fi
}

# Enable GPIO pins for digital inputs (buttons) on Raspberry Pi
gpio_export 22
gpio_export 27

# GPIO pins for digital outputs (LEDs) are enabled in another script
