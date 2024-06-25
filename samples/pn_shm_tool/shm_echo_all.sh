#!/bin/sh
# Write shm outputs to inputs
# For test running this command forever
# may be handy:
# while true; do /usr/bin/shm_echo_all.sh; done
# Utility script using the pn_shm_tool. Intended to be
# used for testing the pn_lan9662 application.
#


pn_shm_tool -r pnet-out-2-1-digital_output_1x8     | pn_shm_tool -w pnet-in-1-1-digital_input_1x8
pn_shm_tool -r pnet-out-7-1-digital_out_1x64       | pn_shm_tool -w pnet-in-3-1-digital_input_1x64
pn_shm_tool -r pnet-out-8-1-digital_output_2x32_a  | pn_shm_tool -w pnet-in-4-1-digital_input_2x32_a
pn_shm_tool -r pnet-out-9-1-digital_output_2x32_b  | pn_shm_tool -w pnet-in-5-1-digital_input_2x32_b
pn_shm_tool -r pnet-out-10-1-digital_output_1x800  | pn_shm_tool -w pnet-in-6-1-digital_input_1x800
