#!/bin/sh
# Utility script using the pn_shm_tool. Intended to be
# used for testing the pn_lan9662 application.
#
# Dump the contents of all shared memory areas
# supported by the lan9662 profinet sample application

echo "Inputs"
echo "------------------------------------------------------------------"
echo "pnet-in-1-1-digital_input_1x8"
pn_shm_tool -r pnet-in-1-1-digital_input_1x8 | hexdump -C
echo "pnet-in-3-1-digital_input_1x64"
pn_shm_tool -r pnet-in-3-1-digital_input_1x64 | hexdump -C
echo "pnet-in-4-1-digital_input_2x32_a"
pn_shm_tool -r pnet-in-4-1-digital_input_2x32_a | hexdump -C
echo "pnet-in-5-1-digital_input_2x32_b"
pn_shm_tool -r pnet-in-5-1-digital_input_2x32_b | hexdump -C
echo "pnet-in-6-1-digital_input_1x800"
pn_shm_tool -r pnet-in-6-1-digital_input_1x800 | hexdump -C

echo ""
echo "Outputs"
echo "------------------------------------------------------------------"
echo "pnet-out-2-1-digital_output_1x8"
pn_shm_tool -r pnet-out-2-1-digital_output_1x8 | hexdump -C
echo "pnet-out-7-1-digital_output_1x64"
pn_shm_tool -r pnet-out-7-1-digital_output_1x64 | hexdump -C
echo "pnet-out-8-1-digital_output_2x32_a"
pn_shm_tool -r pnet-out-8-1-digital_output_2x32_a | hexdump -C
echo "pnet-out-9-1-digital_output_2x32_b"
pn_shm_tool -r pnet-out-9-1-digital_output_2x32_b | hexdump -C
echo "pnet-out-10-1-digital_output_1x800"
pn_shm_tool -r pnet-out-10-1-digital_output_1x800 | hexdump -C
