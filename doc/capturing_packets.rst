Capturing and analyzing Ethernet packets
========================================
In order to understand the Profinet traffic, it is useful to capture network
packets and analyze them in a tool like Wireshark.


Wireshark
---------
To install a relatively new Wireshark version on Ubuntu::

    sudo add-apt-repository ppa:wireshark-dev/stable
    sudo apt update
    sudo apt -y install wireshark

In order to be able to capture packets you need to add yourself to the
``wireshark`` user group, or run the program as root::

    sudo wireshark

For details on how to add yourself to the ``wireshark`` user group, see
https://linuxhint.com/install_wireshark_ubuntu/


tcpdump
-------
When running on an embedded Linux board, it can be convenient to run without
a graphical user interface. To capture packets for later display in Wireshark,
use the tool `tcpdump`.

Install it, for example like::

    sudo apt-get install tcpdump

Run it with::

    sudo tcpdump -i enp0s31f6 -n -w outputfile.pcap


Use the ``-i`` argument to specify Ethernet interface.


Capturing packets on network
----------------------------
Profinet is a point-to-point protocol. If the Profinet controller or device
software is running on your machine, you can use Wireshark (or tcpdump)
directly to capture the packets.

If you would like to capture packets between other units (Profinet
controllers/devices) you need special hardware to do the capturing. A network
tap is a network switch with packet monitoring to send a copy of each packet
to another Ethernet connector. Connect the tap on the network link between the
IO-device and IO-controller. Connect the mirroring port to the machine where
you run Wireshark or tcpdump.
