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
It uses three protocol levels:

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

Also these protocols are used:

* LLDP for link layer discovery. Ethertype 0x88CC
* ARP for IP address lookup. Ethertype 0x0806
* DHCP for IP address assignment. Runs over UDP on IP
* SNMP for network topology questions. Runs over UDP on IP.


Communication relations:

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

* CControl: Request from IO-device
* DControl: Request from IO-controller


Real Time Class:

* Real Time Class UDP
* Real Time Class 1
* Real Time Class 2
* Real Time Class 3


Net load class:

* I
* II
* III


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



Misc
----

Subslots in DAP module

* 0x8000 (32768) First interface
* 0x8001 (32762) First port of first interface
* 0x8002 (32770) Second port of first interface
* 0x8100 (33024) Second interface
* 0x8101 (33025) First port of second interface
* 0x8102 (33026) Second port of second interface