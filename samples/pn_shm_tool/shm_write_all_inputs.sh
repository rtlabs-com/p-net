#!/bin/sh
# Utility script using the pn_shm_tool. Intended to be
# used for testing the pn_lan9662 application.
#
# Write a character / byte to all shared memory inputs
# supported by the lan9662 profinet sample application
# Usage:
#         shm_write_all_inputs.sh a
#         shm_write_all_inputs.sh "\x02"

echo -n -e "$1" | \
pn_shm_tool -w pnet-in-1-1-digital_input_1x8

echo -n -e "$1$1$1$1$1$1$1$1" | \
pn_shm_tool -w pnet-in-3-1-digital_input_1x64

echo -n -e "$1$1$1$1$1$1$1$1" | \
pn_shm_tool -w pnet-in-4-1-digital_input_2x32_a

echo -n -e "$1$1$1$1$1$1$1$1" | \
pn_shm_tool -w pnet-in-5-1-digital_input_2x32_b

data=''
for i in `seq 1 100`;
do
data="${data}$1"
done
echo -n -e "$data" | \
pn_shm_tool -w pnet-in-6-1-digital_input_1x800
