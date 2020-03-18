Tutorial
========

Sample app description
----------------------
The sample application implements a Profinet IO-device, having an
IO-module with 8 digital inputs and 8 digital outputs. If running the sample
application on a Raspberry Pi or some other embedded board, the example uses
two buttons connected to inputs and one LED connected to one of the outputs.

The IO-device sample application can be running on:

* Raspberry Pi
* Some other embedded Linux board
* A Linux laptop
* An embedded board running the RT-kernel

A PLC (Programmable Logic Controller) is used as a Profinet IO-controller. It
could be a Siemens Simatic PLC, or a Codesys soft PLC.
It is possible to run the Codesys commercial soft PLC software on a (second)
Raspberry Pi.

.. image:: illustrations/TutorialOverview.png


Functionality
-------------
The LED on the IO-device is controlled by the IO-controller (PLC), and is
normally flashing. With button1 on the IO-device it is possible to turn on
and off the flashing of the LED. Button2 triggers sending an alarm from the
IO-device.

The resulting Ethernet traffic can be studied (see below).


Available files
---------------
The ``sample_app`` directory in the repository contains the source code for
this example. It also contains a GSD file (written in GSDML), which tells the
IO-controller how to communicate with the IO-device.


Modules and slots
-----------------
The GSDML file for the sample app defines these modules:

+----------------------+-------------------------------+----------------------------------+
| Module               | Input data (to IO-controller) | Output data (from IO-controller) |
+======================+===============================+==================================+
| 8 bit in + 8 bit out | 1 byte                        | 1 byte                           |
+----------------------+-------------------------------+----------------------------------+
| 8 bit in             | 1 byte                        |                                  |
+----------------------+-------------------------------+----------------------------------+
| 8 bit out            |                               | 1 byte                           |
+----------------------+-------------------------------+----------------------------------+

There are 4 slots for the sample app (except slot 0 which is used by the DAP
module), and the modules fit in any of the slots 1 to 4. In this example we
will use the "8 bit in + 8 bit out" module.


Sample app data payload
-----------------------
The periodic data sent from the sample application IO-device to IO-controller
is one byte:

* Lowest 7 bits: A counter which has its value updated every 10 ms
* Most significant bit: Button1

When looking in Wireshark, look at the "Profinet IO Cyclic Service Data Unit",
which is 40 bytes. The relevant byte it the fourth byte from left in this
block.

From the IO-controller to IO-device is one byte sent:

* Most significant bit: LED

When looking in Wireshark, look at the "Profinet IO Cyclic Service Data Unit",
which is 40 bytes. The relevant byte it the fifth byte from left in this
block. The value toggles between 0x80 and 0x00.


Set up the nodes
----------------
To setup Raspbian on a Raspberry Pi, and connect buttons and a LED, see
"Install Raspbian on the Raspberry Pi (by using a Linux laptop)"

See "Getting started on Linux" and "Using Codesys soft PLC".

On the page "Capturing and analyzing Ethernet packets" is a description given
on how to study the network traffic.


Print-out from Linux sample application
---------------------------------------

.. highlight:: none

This is the typical output from the Linux sample application at startup::

    ** Starting Profinet demo application **
    Verbosity level:    1
    Ethernet interface: eth0
    MAC address:        08:00:27:6E:99:77
    Station name:       rt-labs-dev
    LED file:           /sys/class/gpio/gpio17/value
    Button1 file:       /sys/class/gpio/gpio22/value
    Button2 file:       /sys/class/gpio/gpio27/value
    IP address:         192.168.137.4
    Netmask:            255.255.255.0
    Gateway:            192.168.137.1

    Waiting for connect request from IO-controller
    Module plug call-back
      Plug module.    API: 0 Slot: 0 Module ID: 0x1 Index in supported modules: 0
    Submodule plug call-back.
      Plug submodule. API: 0 Slot: 0 Module ID: 0x1 Subslot: 0x1 Submodule ID: 0x1 Index in supported submodules: 0
    Submodule plug call-back.
      Plug submodule. API: 0 Slot: 0 Module ID: 0x1 Subslot: 0x8000 Submodule ID: 0x8000 Index in supported submodules: 1
    Submodule plug call-back.
      Plug submodule. API: 0 Slot: 0 Module ID: 0x1 Subslot: 0x8001 Submodule ID: 0x8001 Index in supported submodules: 2
    Module plug call-back
      Plug module.    API: 0 Slot: 1 Module ID: 0x32 Index in supported modules: 3
    Submodule plug call-back.
      Plug submodule. API: 0 Slot: 1 Module ID: 0x32 Subslot: 0x1 Submodule ID: 0x1 Index in supported submodules: 3
    Connect call-back. Status codes: 0 0 0 0
    Callback on event PNET_EVENT_STARTUP
    New data callback. Status: 0x35
    Write call-back. API: 0 Slot: 1 Subslot: 1 Index: 123 Sequence: 2 Length: 4
      Bytes: 00 00 00 00
    Write call-back. API: 0 Slot: 1 Subslot: 1 Index: 124 Sequence: 3 Length: 4
      Bytes: 00 00 00 00
    Dcontrol call-back. Command: 1  Status codes: 0 0 0 0
    Callback on event PNET_EVENT_PRMEND
    Callback on event PNET_EVENT_APPLRDY
    Application signalled that it is ready for data. Return value: 0
    Ccontrol confirmation call-back. Status codes: 0 0 0 0
    Callback on event PNET_EVENT_DATA


Ethernet frames sent during start-up
------------------------------------
For this example, the IO-controller is started first, and then the IO-device.

+---------------------------+----------+------------------------------------------------------------------------------------------------------------+
| Sender                    | Protocol | Content                                                                                                    |
+===========================+==========+============================================================================================================+
| IO-controller (broadcast) | LLDP     | Name, MAC, IP address, port name (sent every 5 seconds)                                                    |
+---------------------------+----------+------------------------------------------------------------------------------------------------------------+
| IO-controller (broadcast) | PN-DCP   | "Ident req". Looking for "rt-labs-dev" (sent every 2.5 seconds)                                            |
+---------------------------+----------+------------------------------------------------------------------------------------------------------------+
| IO-controller (broadcast) | ARP      | Is someone else using my IP?                                                                               |
+---------------------------+----------+------------------------------------------------------------------------------------------------------------+
| IO-device (broadcast)     | LLDP     | Name, MAC, IP address, port name (sent every 5 seconds??)                                                  |
+---------------------------+----------+------------------------------------------------------------------------------------------------------------+
| IO-device (broadcast)     | PN-DCP   | "Hello req". Station name "rt-labs-dev", IP address, gateway, vendor, device (sent every 3 seconds)        |
+---------------------------+----------+------------------------------------------------------------------------------------------------------------+
| IO-device                 | PN-DCP   | "Ident Ok" Identify response. Station name "rt-labs-dev", IP address, gateway, VendorID, DeviceID, options |
+---------------------------+----------+------------------------------------------------------------------------------------------------------------+
| IO-controller             | PN-DCP   | "Set Req" Set IP request. Use IP address and gateway.                                                      |
+---------------------------+----------+------------------------------------------------------------------------------------------------------------+
| IO-device                 | PN-DCP   | "Set Ok" Status.                                                                                           |
+---------------------------+----------+------------------------------------------------------------------------------------------------------------+
| IO-controller (broadcast) | ARP      | Who has <IO-device IP address>?                                                                            |
+---------------------------+----------+------------------------------------------------------------------------------------------------------------+
| IO-device                 | ARP      | IP <IO-device IP address> is at <IO-device MAC address>                                                    |
+---------------------------+----------+------------------------------------------------------------------------------------------------------------+
| IO-controller             | PNIO-CM  | "Connect request" Controller MAC, timeout, input + output data (CR), modules + submodules in slots         |
+---------------------------+----------+------------------------------------------------------------------------------------------------------------+
| IO-device                 | PNIO-CM  | "Connect response" MAC address, UDP port, input + output + alarm CR, station name                          |
+---------------------------+----------+------------------------------------------------------------------------------------------------------------+
| IO-device                 | PNIO-PS  | FrameID 0x8001. Cycle counter, provider stopped. 40 bytes data.                                            |
+---------------------------+----------+------------------------------------------------------------------------------------------------------------+
| IO-controller             | PNIO-PS  | FrameID 0x8000. Cycle counter, provider running. 40 bytes data.                                            |
+---------------------------+----------+------------------------------------------------------------------------------------------------------------+
| IO-controller             | PNIO-CM  | "Write request" API, slot, subslot, data.                                                                  |
+---------------------------+----------+------------------------------------------------------------------------------------------------------------+
| IO-device                 | PNIO-CM  | "Write response" API, slot, subslot, status.                                                               |
+---------------------------+----------+------------------------------------------------------------------------------------------------------------+
| IO-controller             | PNIO-CM  | "Control request" (DControl). Command: ParameterEnd.                                                       |
+---------------------------+----------+------------------------------------------------------------------------------------------------------------+
| IO-device                 | PNIO-CM  | "Control response" Command: Done                                                                           |
+---------------------------+----------+------------------------------------------------------------------------------------------------------------+
| IO-device                 | PNIO-PS  | FrameID 0x8001. Cycle counter, provider running. 40 bytes data.                                            |
+---------------------------+----------+------------------------------------------------------------------------------------------------------------+
| IO-device                 | PNIO-CM  | "Control request" (CControl). Command: ApplicationReady                                                    |
+---------------------------+----------+------------------------------------------------------------------------------------------------------------+
| IO-controller             | PNIO-CM  | "Control response" Command: ApplicationReadyDone                                                           |
+---------------------------+----------+------------------------------------------------------------------------------------------------------------+

The order of the PNIO-PS frames is somewhat random in relation to PNIO-CM frames.

+------------+---------------------------------+
| Protocol   | Description                     |
+============+=================================+
| LLDP       |                                 |
+------------+---------------------------------+
| ARP        |                                 |
+------------+---------------------------------+
| PN-DCP     | Acyclic real-time data          |
+------------+---------------------------------+
| PNIO-PS    | Cyclic real-time data           |
+------------+---------------------------------+
| PNIO-AL    | Acyclic real-time alarm         |
+------------+---------------------------------+
| PNIO-CM    | UDP, port 34964 = 0x8892        |
+------------+---------------------------------+


Ethernet frames sent at alarm
-----------------------------
Frames sent when pressing Button2

+---------------+----------+----------------------------------------------------------------------------------------+
| Sender        | Protocol | Content                                                                                |
+===============+==========+========================================================================================+
| IO-device     | PN-AL    | Alarm notification high, slot, subslot, module, submodule, sequence, 1 byte user data  |
+---------------+----------+----------------------------------------------------------------------------------------+
| IO-controller | PN-AL    | ACK-RTA-PDU                                                                            |
+---------------+----------+----------------------------------------------------------------------------------------+
| IO-controller | PN-AL    | Alarm ack high, alarm type Process, slot, subslot, Status OK                           |
+---------------+----------+----------------------------------------------------------------------------------------+
| IO-device     | PN-AL    | ACK-RTA-PDU                                                                            |
+---------------+----------+----------------------------------------------------------------------------------------+


Using more modules
------------------
If necessary, increase the values for PNET_MAX_MODULES and PNET_MAX_SUBMODULES
in ``include/pnet_api.h``.

In the sample app, the input data is written to all input modules ("8 bit in +
8 bit out" and "8 bit in"). The LED is controlled by the output module ("8 bit
in + 8 bit out" or "8 bit out") with lowest slot number.


Cyclic data for the different slots
-----------------------------------

+------+---------+--------------------------------------------+-----------------------------------------+--------------------------------------------+
| Slot | Subslot | Description                                | | Contents of Input CR                  | | Contents of Output CR                    |
|      |         |                                            | | (to IO-controller)                    | | (from IO-controller)                     |
+======+=========+============================================+=========================================+============================================+
| 0    | 1       | The IO-Device itself                       | (data) + IOPS                           | IOCS                                       |
+------+---------+--------------------------------------------+-----------------------------------------+--------------------------------------------+
| 0    | 0x8000  | Interface 1                                | (data) + IOPS                           | IOCS                                       |
+------+---------+--------------------------------------------+-----------------------------------------+--------------------------------------------+
| 0    | 0x8001  | Port 0 of interface 1                      | (data) + IOPS                           | IOCS                                       |
+------+---------+--------------------------------------------+-----------------------------------------+--------------------------------------------+
| 1    | 1       | Input part of "8-bit in 8-bit out" module  | data (1 byte) + IOPS                    | IOCS                                       |
|      |         +--------------------------------------------+-----------------------------------------+--------------------------------------------+
|      |         | Output part of "8-bit in 8-bit out" module | IOCS                                    | data (1 byte) + IOPS                       |
+------+---------+--------------------------------------------+-----------------------------------------+--------------------------------------------+
| 2    | 1       | "8-bit in" module                          | data (1 byte) + IOPS                    | IOCS                                       |
+------+---------+--------------------------------------------+-----------------------------------------+--------------------------------------------+
| 3    | 1       | "8-bit out" module                         | IOCS                                    | data (1 byte) + IOPS                       |
+------+---------+--------------------------------------------+-----------------------------------------+--------------------------------------------+

Note that the submodules (in subslots) in slot 0 do not send any cyclic data, but they behave as inputs (as they send cyclic IOPS).
