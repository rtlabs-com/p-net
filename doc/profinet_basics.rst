Profinet basics
===============
Profinet is a field bus that communicates over Ethernet, typically at a speed
of 100 Mbit/s. For details on Profinet, see
https://en.wikipedia.org/wiki/PROFINET

A detailed introduction to Profinet is given in the book "Industrial
communication with PROFINET" by Manfred Popp.
It is available to Profinet members.

Profinet devices (IO-devices) are controlled by an PLC (Programmable Logic
Controller), also known as an IO-controller.


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
IO Device should use. The IO Device sets its IP address and station name
accordingly.

Then the PLC starts the actual configuration of the IO device, using the
DCE/RPC protocol that runs on UDP over IP.

After the configuration is done, cyclic data is continuously exchanged between
the IO-device and the PLC. This communication runs on Ethernet layer 2, i.e.
the MAC addresses are used for routing frames (the IP protocol is not used in
those frames).


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
| B                      | Class A + SNMP (network topology detection) | Yes                        | CP 3/5                     |
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
Each interface has its own IP address. Each port has its own MAC address.


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
The p-net stack implements some of them, while others are optional.
For example HTTP is used by some IO-devices for additional configuration via
a web page, but should be implemented outside the Profinet stack itself.

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

Profinet Profiles
-----------------
There are a number of Profinet application profiles defined, for example
PROFIenergy and PROFIdrive. These defines for example the cyclic data should
be interpreted.

Profiles use the API (Application Program Identifier) concept for telling
that profile-specific data is transferred.


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

If compile time setting PNET_MAX_SLOTS is 5, then the slots are numbered 0-4.
The setting PNET_MAX_SUBSLOTS controls the number of subslots per slot,
but there is no fixed relation to which subslot numbers will be used.
Subslot numbers in the range 0-0x9FFF might be used.

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


Payload
-------
The Profinet payload is sent as big endian (network endian) on the wire.

The maximum data (and status) size in total in each direction for a device is 1440 bytes.
If this is consumed by a single subslot the maximum data size is 1438 byte (plus one or two bytes for IOPS and IOCS).
See the IOData entry in the GSDML specification.

If the IOPS (producer status) for a subslot goes to BAD, the PLC will indicate an error
"User data failure of hardware component".
It will persist until the communication is restarted to the IO-device.


I&M data records
----------------
This is Identification & Maintenance records, which are human-readable
information about the model type, software version etc.
Some fields are writable, for example where the device is located.

Up to I&M15 is described in the standard. The p-net stack supports I&M0 - I&M4.
You must support writing to I&M1-3 for at least one of the DAP submodules.

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


Net load class
--------------

* I
* II
* III


Alarm types
-----------
A process alarm describes conditions in the monitored process, for example
too high temperature.

A diagnostic alarm describes conditions in the IO Device itself, for example
a faulty channel or short circuit. Diagnostic alarms are also stored in the IO-Device.

Description of supported alarm types:

+------------------------------------------+--------+-----------------------------------------------------------+
| Name                                     | Hex    | Description                                               |
+==========================================+========+===========================================================+
| Diagnosis                                | 0x0001 | There is something wrong with the IO device itself        |
+------------------------------------------+--------+-----------------------------------------------------------+
| Process                                  | 0x0002 | There is something wrong with the process, for            |
|                                          |        | example too high temperature. High priority.              |
+------------------------------------------+--------+-----------------------------------------------------------+
| Pull                                     | 0x0003 | Submodule pulled from subslot.                            |
+------------------------------------------+--------+-----------------------------------------------------------+
| Plug                                     | 0x0004 | Module/submodule plugged into slot/subslot.               |
+------------------------------------------+--------+-----------------------------------------------------------+
| Controlled by supervisor                 | 0x0008 |                                                           |
+------------------------------------------+--------+-----------------------------------------------------------+
| Released                                 | 0x0009 |                                                           |
+------------------------------------------+--------+-----------------------------------------------------------+
| Plug wrong submodule                     | 0x000A | Wrong module/submodule plugged into slot/subslot.         |
+------------------------------------------+--------+-----------------------------------------------------------+
| Return of submodule                      | 0x000B |                                                           |
+------------------------------------------+--------+-----------------------------------------------------------+
| Diagnosis disappears                     | 0x000C | A kind of diagnosis alarm                                 |
+------------------------------------------+--------+-----------------------------------------------------------+
| Port data change notification            | 0x000E | Port up, or peer changes name. A kind of diagnosis alarm. |
+------------------------------------------+--------+-----------------------------------------------------------+
| Pull module                              | 0x001F | Module pulled from slot.                                  |
+------------------------------------------+--------+-----------------------------------------------------------+

Only process alarms are sent with high prio, all other alarms use low prio.


Diagnosis
---------
Diagnosis alarms are sent to indicate for example short-circuit on an output.

To localize the diagnosis source, these values are required:

* API
* Slot
* Subslot
* Channel number (Use 0x8000 for "whole submodule")
* Accumulative (true when describing a channel group)
* Direction (in or out. Use "manufacturer specific" for "whole submodule")

Do not update diagnosis information at a higher frequency than 1 Hz.


Logbook
-------
A logbook is a circular buffer with at least 16 entries. The IO-controller can
read out entire logbook. Each entry contains:

* A timestamp
* Error codes (4 bytes)
* A manufacturer specific entry detail (uint32_t)
