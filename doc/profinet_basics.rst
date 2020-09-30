Profinet basics
===============
Profinet is a field bus that communicates over Ethernet, typically at a speed
of 100 Mbit/s. For details on Profinet, see
https://en.wikipedia.org/wiki/PROFINET

A detailed introduction to Profinet is given in the book "Industrial
communication with PROFINET" by Manfred Popp.
It is available to Profinet members.


GSD files
---------
A GSD (General Station Description) file is an XML file describing a Profinet
IO-Device. The XML-based language is called GSDML (GSD Markup Language).

Note that the GSD file is not used by the p-net stack or application. It is
a machine readable file describing the capabilities, hardware- and software
versions etc, and is used by the engineering tool to adjust the PLC settings.

The GSD file describes for example which types of pluggable hardware modules
that can be used in the IO-device. The GSD file is loaded into the
engineering tool (typically running on a personal computer),
and a user can then in the tool
describe which modules that actually should be plugged into the IO-device.
This is called configuration. Later this information is downloaded to the PLC
(IO-controller), in a process called commissioning. At startup the PLC will
tell the IO-device what type of modules it expects to be plugged in.
If the correct modules not are plugged into the IO-device, the IO-device will
send an error message back to the PLC.


Startup description
-------------------
In the startup of an Profinet IO device (before the IP address has been set) the
DCP protocol is used. It is like DHCP, but without using a central server.

The PLC sends a DCP broadcast message, and all IO Devices on the subnet answer
with their MAC addresses. The PLC sends a DCP message to the IO Device with
a specific MAC address, containing which IP address and station name that the
IO Device should use. The IO Device sets its IP address accordingly.

Then the PLC starts the actual configuration of the IO device, using the
DCE/RPC protocol that runs on UDP over IP.


Nodes classes and device details
--------------------------------
Profinet defines three node classes:

.. table::
    :widths: 25 50 25

    +---------------+------------------------------------------------------------------+----------------------------+
    | Node class    | Description                                                      | |  Supported by            |
    |               |                                                                  | |  this software           |
    +===============+==================================================================+============================+
    | IO-Device     | "Slave"                                                          | Yes                        |
    +---------------+------------------------------------------------------------------+----------------------------+
    | IO-Controller | "Master", often a PLC.                                           | No                         |
    +---------------+------------------------------------------------------------------+----------------------------+
    | IO-Supervisor | For commissioning and diagnostics, typically a personal computer | No                         |
    +---------------+------------------------------------------------------------------+----------------------------+


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

Communication profiles CP 3/1 and CP 3/2 are for Profibus. The CP 3/3 was used for the legacy Profinet CBA.

Real Time Class:

* Real Time Class 1 Mandatory for conformance class A, B, C
* Real Time Class 2 (legacy) Only for legacy startup mode for conformance class C.
* Real Time Class 3 = IRT (Isochronous Real Time) Mandatory for conformance class C
* Real Time Class UDP  For cross-network real time communication. Optional for conformance class A, B, C
* Real Time Class STREAM for conformance class D.

This software supports Real Time Class 1.


MAC addresses
-------------
If your device has two Ethernet connectors (ports), then it typically uses an Ethernet
switch chip with three data connections::

   Port X1 P1     Port X1 P2
      |                 |
      |                 |
      |                 |
   +--+-----------------+--+
   |                       |
   |    Ethernet Switch    |
   |                       |
   +----------+------------+
              |
              |
              |
     Profinet interface X1

The device has one interface and two ports.
For Profinet conformance class B and higher, there are three assigned MAC addresses:

* Profinet interface X1 (main MAC address or device MAC address)
* Port X1 P1
* Port X1 P2

The MAC addresses for X1 P1 and X1 P2 are used by the LLDP protocol for neighbor detection.

There are examples of Profinet controllers with more than one interface (having more than one port each).


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

Profinet uses IPv4 only (not IPv6).


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
| Filter only | 2      | 6         | Alias name             |                                    |
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
| 4           | 5      | 6         | Reset to factory       | Type of reset                      |
+-------------+--------+-----------+------------------------+------------------------------------+
| 5           | 255    | 255       | All                    |                                    |
+-------------+--------+-----------+------------------------+------------------------------------+
| 6           | 6      | 1         | Device initiative      | Issues Hello at power on           |
+-------------+--------+-----------+------------------------+------------------------------------+

Setting the station name and IP address etc:

* Permanent: The values should be used after power cycling
* Temporary: After power cycling the station name should be "" and the IP address 0.0.0.0


DCE/RPC protocol via UDP
------------------------
In the connect request, the IO-controller (PLC) tells the IO-device how it
believes that the IO-device hardware is set up. If that not is correct, the
IO-device will complain.

Message types:

* "Request" sent from system A
* "Indication" when it is received in system B
* "Response" sent back from system B
* "Confirmation" when received in system A

The "Response" and "Confirmation" can contain a positive value (+, ACK) or negative
value (-, NACK, indicating an error).

Most often (DCE/RPC) requests are sent from the IO-controller, but CControl
request and a few alarm requests are sent from the IO-device.

The section 5.2.40 "PDU checking rules" in the standard describes what to check in
incoming DCE/RPC messages via UDP.

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

* DControl: Request to IO-device (End of parameterization)
* CControl: Request to IO-controller (Application ready)

Operations:

* 0: Connect
* 1: Release
* 2: Read
* 3: Write
* 4: Control
* 5: Read Implicit
* 6: Reject
* 9: Fragment acknowledge

UDP ports:

* 0x8892 = 34962          Port for RT_CLASS_UDP
* 0x8894 = 34964          Listening port for incoming requests, both on IO-device and IO-controller.
* 0xC000 = 49152 and up   Ephemeral port range
* 0xC001 = 49153          Ephemeral port  for CControl sending???

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

In responses there is no Args Maximum field. Instead there is a status field, with these subfields:

* code
* decode
* code1
* code2


DCE/RPC payload
---------------
Examples of block identifiers:

* 0x0001 AlarmNotificationHigh
* 0x0002 AlarmNotificationLow
* 0x0008 IODWriteReqHeader
* 0x0009 IODReadReqHeader
* 0x0019 LogBookData
* 0x0020 I&M0
* 0x0021 I&M1
* 0x0101 ARBlockReq
* 0x0102 IOCRBlockReq
* 0x0103 AlarmCRBlockReq
* 0x0104 ExpectedSubmoduleBlockReq
* 0x0110 IODControlReq
* 0x8001 AlarmAckHigh
* 0x8002 AlarmAckLow
* 0x8008 IODWriteResHeader
* 0x8009 IODReadResHeader


UDP message fragmentation
-------------------------
Profinet has a mechanism (part of DCE/RPC via UDP) to split large frames
(for start-up messages) into smaller fragments. Operating systems, for example
Linux, have a competing mechanism to split frames into fragments.

If sending a large chunk of data via UDP in Linux, it is automatically split
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

Each API has its own collection of slots.

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

Subslots 0x8000-0xFFFF are reserved by the Profinet standard.

Subslots in the DAP module:

* 0x8000 (32768) First interface (typically named X1)
* 0x8001 (32762) First port of first interface (typically named X1 P1)
* 0x8002 (32770) Second port of first interface (typically named X1 P2)
* 0x8100 (33024) Second interface
* 0x8101 (33025) First port of second interface
* 0x8102 (33026) Second port of second interface


User defined indexes are in the range 0x?? to 0x??

Examples of pre-defined indexes:

* 0xAFF0  I&M0
* 0xAFF1  I&M1
* 0xAFF2  I&M2
* 0xAFF3  I&M3
* 0xF830  LogBookData
* 0xF840  I&M0FilterData
* 0xF841  PRRealData

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

There is also I&M0 Filterdata, which is read only.


Startup modes
-------------
The startup mode was changed in Profinet 2.3, to "Advanced". The previous
startup mode is now called "Legacy".

Alarm types
-----------
A process alarm describes conditions in the monitored process, for example
too high temperature.
A diagnostic alarm describes conditions in the IO Device itself, for example
a faulty channel or short circuit. Diagnostic alarms are also stored in the IO-Device.

* Diagnosis alarm (0x0001): There is something wrong with the IO device itself.
* Process alarm (0x0002): There is something wrong with the process, for example too high temperature.
* Pull alarm (0x0003): Module/submodule pulled from slot/subslot.
* Plug alarm (0x0004): Module/submodule plugged into slot/subslot.
* Plug wrong alarm (0x000a): Wrong module/submodule plugged into slot/subslot.
* etc


Relevant standards
------------------

* IEC IEEE 60802  TSN Profile for Industrial Automation
* IEC 61158-5-10  PROFINET IO: Application Layer services for decentralized periphery (Also known as PNO-2.712)
* IEC 61158-6-10  PROFINET IO: Application Layer protocol for decentralized periphery (Also known as PNO-2.722)
* IEC 61784       Describes several fieldbuses, for example Foundation Fieldbus, Profibus and Profinet.
* IEC 61784-2     Profiles for decentralized periphery (Also known as PNO-2.742)
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
* IETF RFC 1213   Management Information Base v 2 (MIB-II)
* IETF RFC 2131   DHCP
* IETF RFC 2132   DHCP Options
* IETF RFC 3418   Management Information Base (MIB) for SNMP
* IETF RFC 3635   Definitions of Managed Objects for the Ethernet-like Interface Types
* IETF RFC 5890   Internationalized Domain Names for Applications (IDNA)
* ISO/IEC 7498-1  ?
* ISO 8859-1      ?
* ISO 15745       ?
* Open Group C706 Remote procedure calls (RPC)
