#!/bin/sh
#
# Read shared memory and write to EVB-LAN9662 LED
# Utility script using the pn_shm_tool. Intended to be
# used for testing the pn_lan9662 application.
#


MEMORY_NAME="pnet-out-2-1-digital_output_1x8"
BIT_NUMBER=7
LED_NAME="twr1:yellow"

echo "Starting LED handler for Profinet data LED"

sleep 1

while true
do
   sleep 0.1

   led_state=$(pn_shm_tool -b "$MEMORY_NAME" -n "$BIT_NUMBER")
   exitcode=$?

   if [ $exitcode -ne 0 ]; then
      echo "Failed to get LED state from shared memory"
      exit 1
   fi

   echo ${led_state} > "/sys/class/leds/${LED_NAME}/brightness"

done
