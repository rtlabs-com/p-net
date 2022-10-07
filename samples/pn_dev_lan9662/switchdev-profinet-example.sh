#!/bin/sh
#
# Start Profinet sample application
# By default RTE mode is set to full / RTE-FGPA
# The RTE mode can be selected by passing
# "none", "cpu" or "full" to this script.
#
# Application RTE modes
# none
#  - LAN9662 RTE:      Disabled
#  - I/O Data mapping: Shared Memory
#  - P-Net data path:  P-Net default
# cpu
#  - LAN9662 RTE:      Enabled
#  - I/O Data mapping: Shared Memory
#  - P-Net data path:  RTE - SRAM
# full
#  - LAN9662 RTE:      Enabled
#  - I/O Data mapping: iofpga
#  - P-Net data path:  RTE - QSPI
#
# P-Net profinet requires a file system for
# configuration storage. This example assumes
# a rw file system mounted on /tmp/pn_data
#

echo "Starting switchdev-profinet-example"

# Assume a valid partition on /dev/mmcblk0p2
mkdir /tmp/pn_data
mount -o sync /dev/mmcblk0p2 /tmp/pn_data

# Configure bridge
ip link add name br0 type bridge

# Needed for bridge to work
symreg ANA_PGID[61] 0x1ff

ip link set br0 address 12:A9:2D:16:93:89
ip link set br0 up
sysctl -w net.ipv6.conf.br0.disable_ipv6=1

ip link set eth0 up
ip link set eth1 up

ip link set eth0 master br0
ip link set eth1 master br0

# Enable forwarding to chip port 4 (RTE)
symreg qsys_sw_port_mode[4] 0x45000

# Start Profinet application
if [[ $# -eq 0 ]] ; then
    mode=full
else
    mode=$1
fi

echo "Starting LAN9662 Profinet sample application"
echo "RTE mode: $mode"

/usr/bin/pn_lan9662 -m "$mode" -vvvv -p /tmp/pn_data &

# When enabling log level "DEBUG" for the P-Net stack
# it may be useful to write log to file. The log file
# can be uploaded from target using scp:
# scp root@192.168.0.50:/tmp/pn_dev_log.txt pn_dev_log.txt

#echo "Setup dropbear to support scp"
#mkdir -p /var/run/dropbear/
#dropbear -R -B

# Enable to to write p-net log to file.
#touch /tmp/pn_dev_log.txt
#chmod a+w /tmp/pn_dev_log.txt
#echo " Application log will be stored to /tmp/pn_dev_log.txt"
#/usr/bin/pn_lan9662 -m "$mode" -vvvv -p /tmp/pn_data > /tmp/pn_dev_log.txt
