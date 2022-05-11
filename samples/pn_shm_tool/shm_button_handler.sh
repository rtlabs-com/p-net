#!/bin/sh
#
# Read button on GPIO pin and write to shared memory
# Utility script using the pn_shm_tool. Intended to be
# used for testing the pn_lan9662 application.
#

MEMORY_NAME="pnet-in-1-1-digital_input_1x8"

# Set up digital input on "Raspberry Pi"-type header
# Button1, pin 7, use GND at pin 9
GPIO_NUMBER=39

echo "Starting button state monitor for GPIO $GPIO_NUMBER"

echo "$GPIO_NUMBER" > /sys/class/gpio/export
echo "1" > "/sys/class/gpio/gpio$GPIO_NUMBER/active_low"

sleep 1

while true
do
   sleep 0.1

   button_state=$(cat "/sys/class/gpio/gpio$GPIO_NUMBER/value")
   exitcode=$?

   if [ $exitcode -ne 0 ]; then
      echo "Failed to read button state"
      exit 1
   fi

   if [ "$button_state" = "1" ]; then
      echo -n -e '\x80' | pn_shm_tool -w "$MEMORY_NAME"
   else
      echo -n -e '\x00' | pn_shm_tool -w "$MEMORY_NAME"
   fi

done
