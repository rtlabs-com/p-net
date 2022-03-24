Profinet details
================

Technical details on some of the protocols used in a Profinet Application
--------------------------------------------------------------------------

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
* Temporary: After power cycling the station name should be ``""`` and the IP address ``0.0.0.0``


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
* 0x0010 DiagnosisData
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


DCE/RPC UUID entities
---------------------

+-----------------------+-------------+-------------------------------------------------------------------------------+
| UUID variant          | Section     | Description                                                                   |
+=======================+=============+===============================================================================+
| Object UUID           | 4.10.3.2.8  | ``dea00000-6c97-11d1-8271-`` then vendorID, device ID etc                     |
+-----------------------+-------------+-------------------------------------------------------------------------------+
| PNIO Interface UUID   | 4.10.3.2.9  | ``dea00001-6c97-11d1-8271-00a02442df7d`` for IO devices                       |
+-----------------------+-------------+-------------------------------------------------------------------------------+
| Activity UUID         | 4.10.3.2.10 | Same in request and response for one group of Connect-Write-DControl-Ccontrol |
+-----------------------+-------------+-------------------------------------------------------------------------------+
| AR UUID               | 5.2.5.47    | Can be the same between several connect attempts.                             |
+-----------------------+-------------+-------------------------------------------------------------------------------+
| CMInitiatorObjectUUID | 5.1.2       | ``dea00000-6c97-11d1-8271-`` then InstanceID and deviceIdentNumber.           |
|                       |             | Only available in Connect requests.                                           |
+-----------------------+-------------+-------------------------------------------------------------------------------+

The section column refers to the Profinet 2.4 Protocol document.


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


Details on slots and modules
----------------------------
Subslots 0x8000-0xFFFF are reserved by the Profinet standard.

Subslots in the DAP module:

* 0x1   The device itself
* 0x8000 (32768) Interface 1 (typically named X1)
* 0x8001 (32769) Port 1 of interface 1 (typically named X1 P1)
* 0x8002 (32770) Port 2 of interface 1 (typically named X1 P2)
* 0x8100 (33024) Interface 2
* 0x8101 (33025) Port 1 of interface 2
* 0x8102 (33026) Port 2 of interface 2


Read and write indexes
----------------------
User defined indexes are in the range 0x?? to 0x??

Examples of pre-defined indexes:

* 0x802B  PDPortDataCheck for one subslot
* 0xAFF0  I&M0
* 0xAFF1  I&M1
* 0xAFF2  I&M2
* 0xAFF3  I&M3
* 0xF830  LogBookData
* 0xF840  I&M0FilterData
* 0xF841  PDRealData

+-----------------------------+--------------------------------------------------------+
| Data record                 | Description                                            |
+=============================+========================================================+
| APIData                     |                                                        |
+-----------------------------+--------------------------------------------------------+
| ARData                      |                                                        |
+-----------------------------+--------------------------------------------------------+
| ARFSUDataAdjust             |                                                        |
+-----------------------------+--------------------------------------------------------+
| AutoConfiguration           |                                                        |
+-----------------------------+--------------------------------------------------------+
| CombinedObjectContainer     |                                                        |
+-----------------------------+--------------------------------------------------------+
| ExpectedIdentificationData  |                                                        |
+-----------------------------+--------------------------------------------------------+
| I&M0FilterData              |                                                        |
+-----------------------------+--------------------------------------------------------+
| LogBookData                 |                                                        |
+-----------------------------+--------------------------------------------------------+
| ModuleDiffBlock             |                                                        |
+-----------------------------+--------------------------------------------------------+
| PDExpectedData              |                                                        |
+-----------------------------+--------------------------------------------------------+
| PDInterfaceAdjust           | Use standard LLDP mode or legacy LLDP mode.            |
+-----------------------------+--------------------------------------------------------+
| PDInterfaceDataReal         | Actual station name (not ChassisID), interface MAC     |
|                             | address, IP address etc.                               |
+-----------------------------+--------------------------------------------------------+
| PDInterfaceFSUDataAdjust    |                                                        |
+-----------------------------+--------------------------------------------------------+
| PDInterfaceSecurityAdjust   |                                                        |
+-----------------------------+--------------------------------------------------------+
| PDIRData                    |                                                        |
+-----------------------------+--------------------------------------------------------+
| PDIRSubframeData            |                                                        |
+-----------------------------+--------------------------------------------------------+
| PDNCDataCheck               |                                                        |
+-----------------------------+--------------------------------------------------------+
| PDPortDataAdjust            | Turn on and off LLDP transmission.                     |
+-----------------------------+--------------------------------------------------------+
| PDPortDataCheck             | Wanted peer chassisID, port ID etc.                    |
+-----------------------------+--------------------------------------------------------+
| PDPortDataReal              | Actual PortID, peer PortID, peer ChassisID, peer MAC   |
|                             | address, MAU type, link state etc.                     |
+-----------------------------+--------------------------------------------------------+
| PDPortDataRealExtended      |                                                        |
+-----------------------------+--------------------------------------------------------+
| PDPortStatistic             | Sent and received bytes. Errors and discards.          |
+-----------------------------+--------------------------------------------------------+
| PDRealData                  | Actual values and statistics for interface and port.   |
+-----------------------------+--------------------------------------------------------+
| PDSyncData                  |                                                        |
+-----------------------------+--------------------------------------------------------+
| PDTimeData                  |                                                        |
+-----------------------------+--------------------------------------------------------+
| PE_EntityFilterData         |                                                        |
+-----------------------------+--------------------------------------------------------+
| PE_EntityStatusData         |                                                        |
+-----------------------------+--------------------------------------------------------+
| PE_EntityStatusData         |                                                        |
+-----------------------------+--------------------------------------------------------+
| RealIdentificationData      | Which modules (and submodules) are plugged into which  |
|                             | slots (and subslots).                                  |
+-----------------------------+--------------------------------------------------------+
| SubstituteValue             |                                                        |
+-----------------------------+--------------------------------------------------------+


Allowed station name
--------------------
The specification is found i Profinet 2.4 section 4.3.1.4.16


Diagnosis details
-----------------
There are several formats in which the diagnosis information can be sent to
the PLC. The formats are described by the USI value.

Channel diagnosis (USI 0x8000, one of the standard formats):

 * Channel number
 * Channel properties (direction)
 * Channel error type

Extended channel diagnosis (USI 0x8002, one of the standard formats):

 * Same as for "Channel diagnosis", and also
 * ExtChannelErrorType
 * ExtChannelErrorAddValue

Qualified channel diagnosis (USI 0x8003, one of the standard formats):

 * Same as for "Extended channel diagnosis", and also
 * Qualified sub severities

USI format (USI 0x0000 to 0x7FFF):

  * Manufacturer data

Storage of diagnosis info
^^^^^^^^^^^^^^^^^^^^^^^^^
There is at most one diagnosis
entry stored for each ChannelErrortype, extChannelErrorType combination.


LLDP
----
A protocol for neighbourhood detection. LLDP frames are not forwarded by managed
switches, so the frames are useful to detect which neighbour the device is
connected to.

An LLDP frame is sent by a Profinet device every 5 seconds, to indicate
the IP address etc.

A Profinet device also receives LLDP frames. It uses the chassis ID and the
frame ID of the frame from its neighbour, to set the alias name.

The LLDP frame is a layer 2 Ethernet frame with the payload consisting of a
number of Type-Length-Value (TLV) blocks. The first 16 bits of each block
contains info on the block type and block payload length.
It is followed by the block payload data.
Different TLV block types may have subtypes defined (within the payload).

The frame is broadcast to MAC address ``01:80:c2:00:00:0e`` and
has an Ethertype of ``0x88cc``.

TLV types:

* 0: (End of LLDP frame indicator)
* 1: Chassis ID. Subtypes 4: MAC address. 7: Locally assigned name
* 2: Port ID. Subtypes 3: MAC address. 7: Locally assigned name
* 3: Time to live in seconds
* 4: Port description
* 5: System name
* 7: Capabilities (Router, Bridge, Telephone etc)
* 8: Management address (optional for LLDP, mandatory in Profinet). Includes IP
  address and interface number. Address subtype 1: IPv4 2: IPv6
* 127: Organisation specific (optional for LLDP. See below.). Has an
  organisation unique code, and a subtype.

Organisation unique code ``00:0e:cf`` belongs to Profibus Nutzerorganisation,
and supports these subtypes:

* 1: Measured delay values
* 2: Port status. Contains RTClass2 and RTClass3 port status.
* 5: Chassis MAC address

Organisation unique code ``00:12:0f`` belongs to the IEEE 802.3 organisation,
and supports these subtypes:

* 1: MAC/PHY configuration status. Shows autonegotiation support, and which
  speeds are supported. Also MAU type.
* 4: Maximum frame size

Autonegotiation:

* Bit 0: Supported
* Bit 1: Enabled

Speed:

* Bit 0: 1000BASE-T Full duplex
* Bit 1: 1000BASE-T Half duplex
* Bit 10: 100BASE-TX Full duplex
* Bit 11: 100BASE-TX Half duplex
* Bit 13: 10BASE-T Full duplex
* Bit 14: 10BASE-T Half duplex
* Bit 15: Unknown speed

MAU types:

* 0x00 Radio
* 0x10 Copper 100BaseTX Full duplex
* 0x1E Copper 1000BaseT Full duplex


Standard and legacy LLDP name format
------------------------------------
The contents of the LLDP fields PortID and ChassisID were changed in
Profinet v 2.3. An example will show the differences:

+-------------------------------+------------+
| NameOfStation (for interface) | "dut"      |
+-------------------------------+------------+
| NameOfPort                    | "port-001" |
+-------------------------------+------------+
| NameOfStation of neighbour    | "b"        |
+-------------------------------+------------+
| NameOfPort of neighbour       | "port-003" |
+-------------------------------+------------+

We have an alias name for each of our ports, and it is constructed from
the information about the connected neighbour. In this example, our
alias name for our port would be "port-003.b".

Legacy LLDP mode (up to and including Profinet v 2.2):

+-----------------+------------+
| LLDP_PortID     | "port-001" |
+-----------------+------------+
| LLDP_ChassisID  | "dut"      |
+-----------------+------------+

Standard LLDP mode (Profinet v 2.3 and later):

+-----------------+----------------------------------------+
| LLDP_PortID     | "port-001.dut"                         |
+-----------------+----------------------------------------+
| LLDP_ChassisID  | devicetype+orderID+serialNumber etc    |
+-----------------+----------------------------------------+

If NameOfStation not yet is set, we instead use our device MAC address
(as a string) when constructing the LLDP strings.

The legacy LLDP mode is used by devices conformant to Profinet v2.2 and earlier.
Later Profinet versions uses the standard LLDP mode, but an IO-device can
be ordered by the PLC at startup to use the legacy LLDP mode.
By setting the ``MultipleInterfaceMode`` to ``1`` (instead of ``0``) the PLC
will ask the IO-device to use the standard LLDP mode. This is done by writing
to the PDInterfaceAdjust index.


Media redundancy
----------------
With media redundancy an Ethernet ring topology is used, so that Ethernet frames
can reach all devices in the case of a link failure. To prevent frames to
circulate in the ring forever, a manager will keep one of its links blocked
for normal packets.
If the manager detects a failure in the ring, it will open both its links.

The detection of the ring integrity is done using the Media Redundancy Protocol
(MRP). The manager sends MRP_Test frames in both direction, and verifies that
it self can receive them. In case of ring failure, it will send a
MRP_TopologyChange frame to all clients to inform them that they should clean
their routing tables. Clients can also tell the manager that a link has failed,
by sending a MRP_LinkChange frame.

The MRP implementation will open and close network ports, but will not
affect the Profinet class A and B operation.

Devices supporting media redundancy needs to have at least two Ethernet ports
and an integrated switch.

For Profinet class C there is also Media redundancy with duplicated frames
(MRPD), also known as seamless or bumpless redundancy. It sends two similar
frames with cyclic data in the different ring directions. The frames have
identical FrameID but use different VLAN IDs. In case of a link failure there
is cyclic data available from the other direction.

Profinet also has the concept of system redundancy, where two PLCs with
automatic switchover are used. There are different levels of system
redundancy, named R1, R2, S1 and S2.

====================== ================= ===============
Media redundancy class Conformance class Comments
====================== ================= ===============
RED_CLASS_1            A and B           Uses MRP
RED_CLASS_3            C                 Uses MRPD
RED_CLASS_STREAM       D
====================== ================= ===============

An "interconnect ring" connects two MRP rings together, and has four nodes (two
from each normal MRP ring). This requires 3 Ethernet ports on each device.
The interconnect functionality have two modes:

* LC: Link Check (used by Profinet). The status of each link is monitored by the
  Ethernet hardware
* RC: Ring Check. Test frames are sent in the interconnect ring.

In summary, these are the available MRP roles:

* MRP client (MRC). Mandatory if MRP is used for Profinet.
* MRP manager (MRM)
* MRP manager with auto negotiation (MRA)
* MRP interconnection client (MIC)
* MRP interconnection manager (MIM)

The nodes participating in the interconnection ring will also have a role in
the normal ring. Thus a node can be for example bot a MRC and a MIM
simultaneusly.

Profinet defines alarms for different events in the MRP implementation, for
example if multiple MRP managers exist in the same ring or if the neighbour
belongs to wrong MRP ring.

It is possible to to read out the MRP status via the SNMP protocol. This is
done using the MRP-MIB. It is optional for Profinet devices to implement this.

See IEC 62439-2 for MRP protocol details.


Relevant standards
------------------

* IEC IEEE 60802  TSN Profile for Industrial Automation
* IEC 61158-5-10  PROFINET IO: Application Layer services for decentralized periphery (Also known as PNO-2.712)
* IEC 61158-6-10  PROFINET IO: Application Layer protocol for decentralized periphery (Also known as PNO-2.722)
* IEC 61784       Describes several fieldbuses, for example Foundation Fieldbus, Profibus and Profinet.
* IEC 61784-2     Profiles for decentralized periphery (Also known as PNO-2.742)
* IEC 62439-2     Media Redundancy Protocol
* IEEE 802        LANs
* IEEE 802.1      Higher Layer LAN Protocols
* IEEE 802.1AB    LLDP (A topology detection protocol)
* IEEE 802.1AS    Time synchronization
* IEEE 802.1Q     Virtual LANs (VLAN)
* IEEE 802.3      Ethernet
* IEEE 802.11     WiFi
* IETF :rfc:`768`    UDP
* IETF :rfc:`791`    IP
* IETF :rfc:`792`    ICMP
* IETF :rfc:`826`    ARP
* IETF :rfc:`1034`   DNS
* IETF :rfc:`1157`   SNMP
* IETF :rfc:`1213`   Management Information Base v 2 (MIB-II)
* IETF :rfc:`2131`   DHCP
* IETF :rfc:`2132`   DHCP Options
* IETF :rfc:`2741`   AgentX protocol for SNMP subagents
* IETF :rfc:`2863`   The Interfaces Group MIB
* IETF :rfc:`3414`   SNMPv3
* IETF :rfc:`3418`   Management Information Base (MIB) for SNMP
* IETF :rfc:`3635`   Definitions of Managed Objects for the Ethernet-like Interface Types
* IETF :rfc:`5890`   Internationalized Domain Names for Applications (IDNA)
* ISO/IEC 7498-1  ?
* ISO 8859-1      ?
* ISO 15745       ?
* Open Group C706 Remote procedure calls (RPC) DCE 1.1 https://publications.opengroup.org/c706
