Profinet basics
===============
Profinet is a field bus that communicates over Ethernet, typically at a speed
of 100 Mbit/s. For details on Profinet, see
https://en.wikipedia.org/wiki/PROFINET



Nodes classes and device details
--------------------------------
Profinet defines three node classes:

.. table::
    :widths: 25 50 25

    +---------------+-----------------------------------+----------------------------+
    | Node class    | Description                       | |  Supported by            |
    |               |                                   | |  this software           |
    +===============+===================================+============================+
    | IO-Device     | "Slave"                           | Yes                        |
    +---------------+-----------------------------------+----------------------------+
    | I0-Controller | "Master", often a PLC.            | No                         |
    +---------------+-----------------------------------+----------------------------+
    | IO-Supervisor | For commissioning and diagnostics | No                         |
    +---------------+-----------------------------------+----------------------------+


Depending on the capabilities, different conformance classes are assigned.

+------------------------+---------------------------------------------+----------------------------+----------------------------+
| |  Conformance         | Description                                 | |  Supported by            | |  Communication           |
| |  class (CC)          |                                             | |  this software           | |  profile (CP)            |
+========================+=============================================+============================+============================+
| A                      | Wireless is only allowed for class A        | Yes                        | CP 3/4                     |
+------------------------+---------------------------------------------+----------------------------+----------------------------+
| B                      | Class A + SNMP (network topology detection) | No                         | CP 3/5                     |
+------------------------+---------------------------------------------+----------------------------+----------------------------+
| B (PA)                 | Class B + System redundancy                 | No                         |                            |
+------------------------+---------------------------------------------+----------------------------+----------------------------+
| C                      | Class B + IRT communication                 | No                         | CP 3/6                     |
+------------------------+---------------------------------------------+----------------------------+----------------------------+
| D                      | Uses time sensitive networking Ethernet     | No                         | CP 3/7                     |
+------------------------+---------------------------------------------+----------------------------+----------------------------+

The first digit in the Communication Profile (CP) is the Communication Profile Family (CPF). Profinet and Profibus are CPF 3,
while for example Ethercat is CPF 12.


Communication
-------------
Profinet uses three protocol levels:

+-----------------------------+--------------------+-------------+---------------+----------------------------+
| Protocol                    | | Typical          | Description | Ethertype     | | Supported by             |
|                             | | cycle time       |             |               | | this software            |
+=============================+====================+=============+===============+============================+
| TCP/IP (UDP actually)       | 100 ms             |             | 0x0800 = IPv4 | Yes                        |
+-----------------------------+--------------------+-------------+---------------+----------------------------+
| RT (Real Time)              | 10 ms              |             | 0x8892        | Yes                        |
+-----------------------------+--------------------+-------------+---------------+----------------------------+
| IRT (Isochronous Real-Time) | 1 ms               |             | 0x8892?       | No                         |
+-----------------------------+--------------------+-------------+---------------+----------------------------+


Overview of all protocols used in a Profinet Application
--------------------------------------------------------

A typical Profinet application needs to handle a multitude of protocols.

+-------+----------+-------------+-----------------+-----------------------+---------------------------------+
| Layer | Protocol | Addressing  | | Content       | Protocol              | Notes                           |
|       |          |             | | description   |                       |                                 |
+=======+==========+=============+=================+=======================+=================================+
| 4     | TCP      | Port                          | SSH = 22              | Remote login on Linux           |
|       |          |                               +-----------------------+---------------------------------+
|       |          |                               | HTTP = 80 + 443       | Web server for configuration    |
+-------+----------+-------------------------------+-----------------------+---------------------------------+
| 4     | UDP      | Port                          | DHCP = 67 + 68        | IP address assignment           |
|       |          |                               +-----------------------+---------------------------------+
|       |          |                               | SNMP = 161 + 162      | Network topology query          |
|       |          |                               +-----------------------+---------------------------------+
|       |          |                               | ??                    | Profinet start-up, uses DCE/RPC |
+-------+----------+-------------+-----------------+-----------------------+---------------------------------+
| 3     | IPv4     | IP address  | Protocol number | ICMP = 1              | Used by ping                    |
|       |          |             |                 +-----------------------+---------------------------------+
|       |          |             |                 | TCP = 6               | Transmission control protocol   |
|       |          |             |                 +-----------------------+---------------------------------+
|       |          |             |                 | UDP = 17              | User Datagram Protocol          |
+-------+----------+-------------+-----------------+-----------------------+---------------------------------+
| 2     | Ethernet | MAC address | Ethertype       | LLDP = 0x88CC         | Link layer discovery            |
|       |          |             |                 +-----------------------+---------------------------------+
|       |          |             |                 | ARP = 0x0806          | IP address lookup               |
|       |          |             |                 +-----------------------+---------------------------------+
|       |          |             |                 | IPv4 = 0x0800         | Internet protocol               |
|       |          |             |                 +-----------------------+---------------------------------+
|       |          |             |                 | Profinet DCP = 0x8894 | Profinet start-up ??            |
|       |          |             |                 +-----------------------+---------------------------------+
|       |          |             |                 | Profinet RT = 0x8892  | PNIO = Cyclic IO realtime data  |
|       |          |             |                 +-----------------------+---------------------------------+
|       |          |             |                 | (VLAN = 0x8100)       | Not really a protocol           |
|       |          |             |                 +-----------------------+---------------------------------+
|       |          |             |                 | (VLAN = 0x9100)       | For double tagged frames        |
+-------+----------+-------------+-----------------+-----------------------+---------------------------------+

Technical details on some of the protocols:

+---------------------------+--------------+----------------+--------------------------------------+----------------------------------------------------+
| Layer                     | | Header     | | Footer       | Header contents                      | Footer contents                                    |
|                           | | size       | | size         |                                      |                                                    |
+===========================+==============+================+======================================+====================================================+
| Ethernet (layer 2)        | 14 or 18     |                | MAC 6+6, Ethertype 2 (VLAN 4)        |                                                    |
+---------------------------+--------------+----------------+--------------------------------------+----------------------------------------------------+
| IPv4                      | 20           |                | IP addr 4+4, len 2, protocol 1, etc  |                                                    |
+---------------------------+--------------+----------------+--------------------------------------+----------------------------------------------------+
| UDP                       | 8            |                | Port 2+2, len 2, checksum 2          |                                                    |
+---------------------------+--------------+----------------+--------------------------------------+----------------------------------------------------+
| DCE/RPC                   | 80           |                | UUID 16+16+16, etc                   |                                                    |
+---------------------------+--------------+----------------+--------------------------------------+----------------------------------------------------+
| Profinet cyclic realtime  | 2            | 4              | FrameId 2                            | Cycle counter 2, data status 1, transfer status 1  |
+---------------------------+--------------+----------------+--------------------------------------+----------------------------------------------------+

Sizes are given in bytes.
Note that "Profinet cyclic realtime" runs directly on Ethernet layer 2 (it does
not use IP or UDP).


UDP ports:

* 0x8894 = 34964
* 0xC001 = 49153


Profinet frame IDs:

* 0x8000  Output CR
* 0x8001  Input CR
* 0xfe01
* 0xfc01
* 0xfefc  DCP_HELLO
* 0xfefd  DCP_GET_SET
* 0xfefe  DCP_ID_REQ
* 0xfeff  DCP_ID_RES


Communication relations
-----------------------

+-----------------------------+----------------------------------------------------------------------------------------------+
| Communication Relation (CR) | Description                                                                                  |
+=============================+==============================================================================================+
| IO data CR                  | Real-time cyclic data. Unacknowledged.                                                       |
+-----------------------------+----------------------------------------------------------------------------------------------+
| Record data CR              | Non-real time configuration data, for example parameter assignment and device identification |
+-----------------------------+----------------------------------------------------------------------------------------------+
| Alarm CR                    | Real-time alarms                                                                             |
+-----------------------------+----------------------------------------------------------------------------------------------+

Messages from controller to device:

* Connect request
* Parameter end request ?
* Application ready response
* Read IM0 request
* Release request
* DControl request
* CControl confirmation
* IODRead request
* IODWrite request

Where:

* DControl: Request to IO-device
* CControl: Request to IO-controller


Real Time Class:

* Real Time Class UDP
* Real Time Class 1
* Real Time Class 2
* Real Time Class 3


Net load class:

* I
* II
* III




DCP protocol, message "Ident OK" and message "Set Req".

+--------+-----------+-----------------+-------------------------------+
| Option | Suboption | Description     | Contains                      |
+========+===========+=================+===============================+
| 1      | 2         | IP parameter    | IP address, netmask, gateway  |
+--------+-----------+-----------------+-------------------------------+
| 2      | 1         | Type of station | Device vendor                 |
+--------+-----------+-----------------+-------------------------------+
| 2      | 2         | Name of station |                               |
+--------+-----------+-----------------+-------------------------------+
| 2      | 3         | Device ID       | VendorID, DeviceID            |
+--------+-----------+-----------------+-------------------------------+
| 2      | 4         | Device role     | ?                             |
+--------+-----------+-----------------+-------------------------------+
| 2      | 5         | Device options  | Which options are available   |
+--------+-----------+-----------------+-------------------------------+


Slots and modules
-----------------
A Profinet IO-device has typically a number of slots where (hardware) modules
can be placed. A module can have subslots where submodules are placed.
Each submodule have a number of channels (for example digital inputs).

TODO Example

* Module
* Submodule
* Channels

Channels are always connected to submodules (rather than to modules).


Addressing a channel

* Slot
* Subslot
* Index

First usable slot is slot number 1. Slot 0 is used for the IO-device itself,
and does not have any input/output data. Instead it has diagnostic information
for the IO-device.

The GSD file pretty much describes the hardware: slots (and subslots), and
the modules (and submodules) that can be placed in the slots. The file does
not describe which modules that actually have been placed in each slot for
each device. That is done during the setup (configuration) in the engineering
tool during PLC programming.

Also in the GSD file is description on the data exchange?

+----------------------+-------------------------------------+
| Type                 | Description                         |
+======================+=====================================+
| Compact field device | Not possible to change modules etc? |
+----------------------+-------------------------------------+
| Modular field device | Change modules at configuration?    |
+----------------------+-------------------------------------+

Subslots in DAP module

* 0x8000 (32768) First interface
* 0x8001 (32762) First port of first interface
* 0x8002 (32770) Second port of first interface
* 0x8100 (33024) Second interface
* 0x8101 (33025) First port of second interface
* 0x8102 (33026) Second port of second interface


I&M data records
----------------
This is Identification & Maintenance records.

+-------------+-----------+-----------------------+--------------------------------------------------------------+
| Data record | Mandatory | Controller can write? | Description                                                  |
+=============+===========+=======================+==============================================================+
| I&M0        | Yes       | No                    | Vendor ID, serial number. Hardware and software version etc. |
+-------------+-----------+-----------------------+--------------------------------------------------------------+
| I&M1        | No        | Yes                   | Tag function and location                                    |
+-------------+-----------+-----------------------+--------------------------------------------------------------+
| I&M2        | No        | Yes                   | Date                                                         |
+-------------+-----------+-----------------------+--------------------------------------------------------------+
| I&M3        | No        | Yes                   | Descriptor                                                   |
+-------------+-----------+-----------------------+--------------------------------------------------------------+
| I&M4        | No        | Yes                   | Signature                                                    |
+-------------+-----------+-----------------------+--------------------------------------------------------------+

Up to I&M 15?



Relevant standards
------------------

* IEEE 802        LANs
* IEEE 802.1      Higher Layer LAN Protocols
* IEEE 802.1AB    LLDP (A topology detection protocol)
* IEEE 802.1AS    Time synchronization
* IEEE 802.1Q     Virtual LANs
* IEEE 802.3      Ethernet
* IEEE 802.11     WiFi
* IEC 61158       Profinet????
* IEC 61784       Describes several fieldbuses, for example Foundation Fieldbus, Profibus and Profinet.
* ISO 15745       ?
* ISO 8859-1      ?
