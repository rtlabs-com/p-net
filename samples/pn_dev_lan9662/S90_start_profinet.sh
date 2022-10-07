#!/bin/sh
#
# Initialization script for automatic
# start of the pn_lan9662 sample application.
# Copy this script to overlay/etc/init.d/
# to start the application automatically.
#

start() {
    # Assume a valid partition on /dev/mmcblk0p2
    mkdir /tmp/pn_data
    mount -o sync /dev/mmcblk0p2 /tmp/pn_data

    echo "Configure network settings for pn_lan9662 profinet sample application"
    # Create bridge
    ip link add name br0 type bridge
    symreg ANA_PGID[61] 0x1ff
    ip link set br0 address 12:A9:2D:16:93:89
    ip link set br0 up
    sysctl -w net.ipv6.conf.br0.disable_ipv6=1

    ip link set eth0 master br0
    ip link set eth1 master br0

    ip link set eth0 up
    ip link set eth1 up

    # Enable forwarding to chip port 4 (RTE)
    symreg qsys_sw_port_mode[4] 0x45000

    # Start Profinet application
    # Supported modes: none, cpu, full
    # Read mode from file
    mode="full"
    if test -f /tmp/pn_data/mode.txt; then
        mode=$(cat /tmp/pn_data/mode.txt)
    fi

    echo "Starting LAN9662 Profinet sample application"
    echo "RTE mode: $mode"

    /usr/bin/pn_lan9662 -m $mode -vvvv -p /tmp/pn_data &

    # Enable for interleaved ART-Tester logs
    #tcpdump -i br0 udp port 514 -v -a &
}

stop() {
    printf "Todo - Stop Profinet application"
}

case "$1" in
  start)
        start
        ;;
  stop)
        stop
        ;;
  restart|reload)
        stop
        start
        ;;
  *)
        echo "Usage: $0 {start|stop|restart}"
        exit 1
esac

exit $?
