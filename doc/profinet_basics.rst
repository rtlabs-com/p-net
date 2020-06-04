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

Real Time Class:

* Real Time Class 1 Mandatory for conformance class A, B, C
* Real Time Class 2 (legacy)
* Real Time Class 3 = IRT (Isochronous Real Time) Mandatory for conformance class C
* Real Time Class UDP  Optional for conformance class A, B, C
* Real Time Class STREAM for conformance class D.

This software supprts Real Time Class 1.


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
|       |          |                               | PNIO-CM = 34964       | Profinet start-up, uses DCE/RPC |
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
|       |          |             |                 | PN-DCP = 0x8892       | Profinet start-up               |
|       |          |             |                 +-----------------------+---------------------------------+
|       |          |             |                 | Profinet RT = 0x8892  | PNIO_PS = Cyclic IO data        |
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
| Profinet acyclic realtime | 2            |                | FrameId 2                            |                                                    |
+---------------------------+--------------+----------------+--------------------------------------+----------------------------------------------------+
| Profinet DCP              | 10           |                | ServiceID 1, ServiceType 1, Xid, Len |                                                    |
+---------------------------+--------------+----------------+--------------------------------------+----------------------------------------------------+

Sizes are given in bytes.
Note that "Profinet cyclic realtime" and "Profinet acyclic realtime" run
directly on Ethernet layer 2 (they do not use IP or UDP).

Profinet DCP runs via "Profinet acyclic realtime".

Note that the length field in the UDP header includes the size of the header itself.


Profinet cyclic and acyclic realtime protocol via Ethernet layer 2
------------------------------------------------------------------

+----------+-----------------------+-------------------+
| Frame ID | Protocol              | Description       |
+==========+=======================+===================+
| 0x8000   | Profinet cyclic       | Output CR         |
+----------+-----------------------+-------------------+
| 0x8001   | Profinet cyclic       | Input CR          |
+----------+-----------------------+-------------------+
| 0xFC01   |                       | ALARM_HIGH        |
+----------+-----------------------+-------------------+
| 0xFE01   |                       | ALARM_LOW         |
+----------+-----------------------+-------------------+
| 0xFEFC   | Profinet acyclic, DCP | HELLO             |
+----------+-----------------------+-------------------+
| 0xFEFD   | Profinet acyclic, DCP | GET_SET           |
+----------+-----------------------+-------------------+
| 0xFEFE   | Profinet acyclic, DCP | Identify request  |
+----------+-----------------------+-------------------+
| 0xFEFE   | Profinet acyclic, DCP | Identify response |
+----------+-----------------------+-------------------+

PNIO status (4 bytes):

* Error code
* Error decode
* Error code 1
* Error code 2


DCP protocol via Ethernet layer 2
---------------------------------
Uses Profinet cyclic realtime protocol.
This is used for example for assigning station name and IP address to devices.

+--------------+------------------+
| Service Type | Description      |
+==============+==================+
| 0            | Request          |
+--------------+------------------+
| 1            | Response Success |
+--------------+------------------+

+------------+-------------+
| Service ID | Description |
+============+=============+
| 3          | Get         |
+------------+-------------+
| 4          | Set         |
+------------+-------------+
| 5          | Identify    |
+------------+-------------+
| 6          | Hello       |
+------------+-------------+


Option 1 suboption 2   Full IP suite


+-------------+--------+-----------+------------------------+------------------------------------+
| Service IDs | Option | Suboption | Description            | Contains                           |
+=============+========+===========+========================+====================================+
| 3           | 1      | 1         | MAC address            |                                    |
+-------------+--------+-----------+------------------------+------------------------------------+
| 3, 4, 5, 6  | 1      | 2         | IP parameter           | IP address, netmask, gateway       |
+-------------+--------+-----------+------------------------+------------------------------------+
| 3, 4, 5, 6  | 1      | 3         | Full IP suite          | IP address, netmask, gateway, DNS  |
+-------------+--------+-----------+------------------------+------------------------------------+
| 3, 5        | 2      | 1         | Type of station        | Device vendor                      |
+-------------+--------+-----------+------------------------+------------------------------------+
| 3, 4, 5, 6  | 2      | 2         | Name of station        | Also permanent/temporary           |
+-------------+--------+-----------+------------------------+------------------------------------+
| 3, 5, 6     | 2      | 3         | Device ID              | VendorID, DeviceID                 |
+-------------+--------+-----------+------------------------+------------------------------------+
| 3, 5        | 2      | 4         | Device role            | ?                                  |
+-------------+--------+-----------+------------------------+------------------------------------+
| 3, 5        | 2      | 5         | Device options         | Which options are available        |
+-------------+--------+-----------+------------------------+------------------------------------+
|             | 2      | 6         | Alias name             |                                    |
+-------------+--------+-----------+------------------------+------------------------------------+
| 6           | 2      | 8         | OEM device ID          |                                    |
+-------------+--------+-----------+------------------------+------------------------------------+
| 4           | 5      | 1         | Start transaction      |                                    |
+-------------+--------+-----------+------------------------+------------------------------------+
| 4           | 5      | 2         | End transaction        |                                    |
+-------------+--------+-----------+------------------------+------------------------------------+
| 4           | 5      | 3         | Signal (Flash LED)     | Flash once                         |
+-------------+--------+-----------+------------------------+------------------------------------+
| 4           | 5      | 4         | Response               |                                    |
+-------------+--------+-----------+------------------------+------------------------------------+
| 4           | 5      | 6         | Reset factory settings |                                    |
+-------------+--------+-----------+------------------------+------------------------------------+
| 4           | 5      | 6         | Reset to factory       | Type of reset                      |
+-------------+--------+-----------+------------------------+------------------------------------+
| 5           | 255    | 255       | All                    |                                    |
+-------------+--------+-----------+------------------------+------------------------------------+
| 6           | 6      | 1         | Device inititive       | Issues Hello at power on           |
+-------------+--------+-----------+------------------------+------------------------------------+


DCE/RPC protocol via UDP
------------------------

Message types:

* req  Request
* ind  Indication
* rsp  Response
* cnf  Confirmation

* + Positively acknowledge (ACK)
* - Negatively acknowledge (NACK)


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

Operations:

* 0: Connect
* 1: Release
* 2: Read
* 3: Write
* 4: Control
* 5: Read Implicit

UDP ports:

* 0x8892 = 34962
* 0x8894 = 34964
* 0xC001 = 49153

UDP port numbers are described in Profinet 2.4 section 4.13.3.1.2.4


NDR header in DCE/RPC payload
-----------------------------
The first part of the DCE/RPC payload is the NDR (Network Data Representation) header. For requests, it contains five uint32 values:

* Args Maximum: Buffer size available for the response
* Args Length: Number of bytes payload after the NDR header
* Maximum Count: In requests this it the same values as the Args Maximum. For responses this is the Args Maximum from the request.
* Offset: Always 0.
* Actual Count: Same as Args Length

The Maximum Count, Offset and Actual Count are known as the "Array" block.

In responses there is no Args Maximum field (Instead there is a status field).


UDP message fragmentation
-------------------------
Profinet has a mechanism (part of DCE/RPC via UDP) to split large frames
(for start-up messages) into smaller fragments. Operating systems, for example
Linux, have a competing mechanism to split frames into fragments.

If sending a large chunk of data via UDP in Linux, it is automatically splitted
into fragments. The maximum transfer unit (MTU) is often 1500 bytes,
including the IP header (but not the Ethernet header). An IP header is
typically 20 bytes, but some rarely used options would make it larger.
Without any IP header options, the largest IP payload would then be 1480 bytes
and the largest UDP payload would be 1472 bytes. It seems that for Linux, the
largest UDP payload is 1464 bytes before the kernel fragments the message.


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

Subslots in DAP module

* 0x8000 (32768) First interface
* 0x8001 (32762) First port of first interface
* 0x8002 (32770) Second port of first interface
* 0x8100 (33024) Second interface
* 0x8101 (33025) First port of second interface
* 0x8102 (33026) Second port of second interface


Allowed station name
--------------------
The specification is found i Profinet 2.4 section 4.3.1.4.16


I&M data records
----------------
This is Identification & Maintenance records. Up to I&M15 is described in the
standard. The p-net stack supports I&M0 - I&M4.

+-------------+-----------+-----------------------+--------------------------------------------------------------+
| Data record | Mandatory | Controller can write? | Description                                                  |
+=============+===========+=======================+==============================================================+
| I&M0        | Yes       | No                    | Vendor ID, serial number. Hardware and software version etc. |
+-------------+-----------+-----------------------+--------------------------------------------------------------+
| I&M1        | No        | Yes                   | Tag function and location                                    |
+-------------+-----------+-----------------------+--------------------------------------------------------------+
| I&M2        | No        | Yes                   | Date. Format "1995-02-04 16:23"                              |
+-------------+-----------+-----------------------+--------------------------------------------------------------+
| I&M3        | No        | Yes                   | Descriptor                                                   |
+-------------+-----------+-----------------------+--------------------------------------------------------------+
| I&M4        | No        | Yes                   | Signature. Only for functional safety.                       |
+-------------+-----------+-----------------------+--------------------------------------------------------------+
| I&M5        | No        | No                    | Not supported by p-net                                       |
+-------------+-----------+-----------------------+--------------------------------------------------------------+

There is also I&M0 Filterdata, which is read only.


Relevant standards
------------------

* IEC IEEE 60802  TSN Profile for Industrial Automation
* IEC 61158-5-10  PROFINET IO: Application Layer services for decentralized periphery
* IEC 61158-6-10  PROFINET IO: Application Layer protocol for decentralized periphery
* IEC 61784       Describes several fieldbuses, for example Foundation Fieldbus, Profibus and Profinet.
* IEEE 802        LANs
* IEEE 802.1      Higher Layer LAN Protocols
* IEEE 802.1AB    LLDP (A topology detection protocol)
* IEEE 802.1AS    Time synchronization
* IEEE 802.1Q     Virtual LANs (VLAN)
* IEEE 802.3      Ethernet
* IEEE 802.11     WiFi
* IETF RFC 768    UDP
* IETF RFC 791    IP
* IETF RFC 792    ICMP
* IETF RFC 826    ARP
* IETF RFC 1034   DNS
* IETF RFC 1157   SNMP
* IETF RFC 2131   DHCP
* IETF RFC 2132   DHCP Options
* IETF RFC 3418   Management Information Base (MIB) for SNMP
* IETF RFC 5890   Internationalized Domain Names for Applications (IDNA)
* ISO/IEC 7498-1  ?
* ISO 8859-1      ?
* ISO 15745       ?
* Open Group C706 Remote procedure calls (RPC)
