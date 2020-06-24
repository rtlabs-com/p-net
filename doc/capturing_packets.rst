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


Parsing Profinet data with Wireshark
------------------------------------
It is possible to load a GSDML file into recent versions of Wireshark, for
parsing the cyclic data.
In the Wireshark menu, select Edit > Preferences > Protocols > PNIO.
Enter the directory where you have your GSDML file.

For this functionality to work, the Wireshark capture must include the start-up
sequence. When a packet is interpreted according to a GSDML file, the name of
the GSDML file is displayed in the detail view of the packet.


Show transmission time periodicity using Wireshark
--------------------------------------------------
In order to study the periodicity of sent frames, in the filter heading on the
main screen select the MAC address of the p-net IO-device, for example::

    eth.src == 54:ee:75:ff:95:a6

In the column header, right-click and select "Column Preferences ...". Press "+"
to add a new column. Change "Title" to "Delta displayed" and "Type" to
"Delta time displayed".


Plot transmission time periodicity using Wireshark
--------------------------------------------------
To plot the periodicity of sent frames, use the menu "Statistics" -> "I/O Graph".

* Display filter: ``eth.src == 54:ee:75:ff:95:a6 and pn_io``
* Y Axis: AVG(Y Field)
* Y Field: ``frame.time_delta_displayed``
* SMA (sample moving average) Period: None

Adapt the MAC address to your p-net IO-device.
Use an "Interval" setting of 10 or 100 ms.
You need to zoom the Y-axis to an interesting range, maybe 0-10 ms.

It is also interesting to add a line "MAX(Y Field)" and a line "MIN(Y Field)"
in the same graph as the first line.

The lines should be interpreted as the average, minimum and maximum
packet-to-packet times during the interval (for example 100 ms).


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
