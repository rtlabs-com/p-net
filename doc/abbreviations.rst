
Terminology and abbreviations
=============================

Configuration
    IO-device and IO-controller definition in an engineering tool.
Commissioning
    Device initialization (Configuration is downloaded to IO-controller).
Engineering tool
    A desktop program for configuring PLC. For example Siemens TIA portal.


Abbreviations
-------------

ACK
    Positively acknowledge
AKA
    Also Known As
ALPMI
    Alarm Protocol Machine Initiator
ALPMR
    Alarm Protocol Machine Responder
AMR
    Asset Management Record
AP
    ?
APDU
    Application Protocol Data Unit
API
    Application Programming Identifier (uint32). Used to differentiate between for example user profiles. Sometimes named "Profile ID".
API
    Application Programming Interface. Application implementers use the API of the p-net Profinet stack.
APMR
    Acyclic Protocol Machine Receiver. A state machine in the IO device implementation.
APMS
    Acyclic Protocol Machine Sender. A state machine in the IO device implementation.
APMX
    General term for APMR and APMS
APO
    Application Process Object
AR
    Application Relation
ARP
    Address Resolution Protocol, used to translate from an IP address to a MAC address.
AREP
    Application Relationship End Point (uint32), pretty much an index into an array of AR.
ASDU
    Application Service Data Unit
ASE
    Application Service Element. For example logbook, time and diagnosis.
CC
    Conformance Class (Performance level A to D)
CIDR
    Classless Inter-Domain Routing. The CIDR notation ``/24`` tells how many bits of the netmask that should be enabled.
CiR
    Configuration in Run
CM
    Context Management
CMDEV
    Context Management Protocol Machine Device
CMI
    CM Initiator
CMINA
    Context Management Ip and Name Assignment protocol machine
CMIO
    Context Management Input Output protocol machine
CMPBE
    Context Management Parameter Begin End protocol machine
CMRDR
    Context Management Read Record Responder protocol machine, responds to parameter read from the IO-controller
CMRPC
    Context Management RPC protocol machine
CMWRR
    Context Management Write Record Responder protocol machine
CMSM
    Context Management Surveillance protocol Machine, monitors the establishment of a connection. Also written as CM Server Protocol machine.
CP
    Communication Profile. For example Profinet conformance class B is also known as CP 3/5.
CPF
    Communication Profile Family. Profinet and Profibus is CPF = 3, Ethercat is CPF = 12.
CPM
    Consumer Protocol Machine, for receiving cyclic data
CPU
    Central Processing Unit
CR
    Communication Relation (Part of AR).
CREP
    Communication Relationship EndPoint (uint32), pretty much an index into an array of input and output CR.
DA
    Destination Address. It is the MAC address of the receiver.
DAP
    Device Access Point
DCE
    Distributed Computing Environment. Used with RPC.
DCP
    Discovery and basic Configuration Protocol. Runs over Ethernet layer 2 (not IP or UDP).
DFP
    Dynamic Frame Packing. Used with IRT protocol.
DHCP
    Dynamic Host Configuration Protocol, for allocating IP addresses to devices.
DHT
    Data Hold Timer
DLL
    Data Link Layer
DNS
    Domain Name System, for converting from host name to IP address.
DT
    Device Tool
DUT
    Device Under Test
EMC
    ElectroMagnetic Compatibility
EPM
    EndPoint Mapper
ES
    Engineering System
FAL
    Fieldbus Application Layer
FSPM
    FAL Service Protocol Machines
FSU
    Fast Start Up (Store communication parameters in IO devices)
GAP
    ?
GSD
    General Station Description. An XML file describing an IO-Device.
GSDML
    GSD Markup Language
GUI
    Graphical User Interface
HTTP
    Hypertext Transfer Protocol
I&M
    Identification & Maintenance. Has different blocks; IM0 to IM??.
ICMP
    Internet Control Message Protocol. (Sent in an IP packet)
IDNA
    Internationalized Domain Names for Applications
IGMP
    Internet Group Management Protocol. For multicast groups. Used in IPv4.
IO
    Input Output
IOC
    IO Controller (Typically a PLC)
IOD
    IO Device. An input-output device controlled by a PLC via Profinet communication.
IOCS
    IO Consumer Status. Reported by IO-device (for output data) and IO-controller (for input data), per subslot. (uint8)
IOCR
    IO Communication Relation
IOPS
    IO Provider Status. Describes validity of IO data per subslot. Sent by IO-device (for input data) or IO-controller (for output data) together with data. (uint8)
IOxS
    General term for IOCS and IOPS.
IP
    Internet Protocol
IRT
    Isochronous Real-Time
LAN
    Local Area Network
LLDP
    Link Layer Discovery Protocol, for neighborhood detection.
LT
    Length and Type field in Ethernet frame. Also known as EtherType.
MAC
    Media Access Control
MAU
    Medium Attachment Unit. Ethernet transceiver type. 0x0 = radio, 0x10 = Media type copper 100BaseTXFD
MC
    Multicast (as opposed to unicast)
MC
    Multicore (Codesys runtime variant for Raspberry Pi)
MDNS
    Multicast DNS. A UDS based protocol for resolving hostname to IP address. Implemented by Bonjour and Avahi.
MIB
    Management Information Base. File format for SNMP?
MRP
    Media Redundancy Protocol
MRPD
    Media Redundancy for Planned Duplication
MTU
    Maximum Transfer Unit. The largest packet a network interface can handle. Typically 1500 bytes. This includes the IP header, but not the Ethernet header.
NACK
    Negatively acknowledge
NDR
    Network Data Representation. A header as first part of the DCE/RPC payload (sent via UDP). Contains info on how large the payload is, and how large responses that can be accepted.
NME
    Network Management Engine
OID
    Object IDentifier
OS
    Operating System
OUI
    Organizationally Unique Identifier. This is the three first bytes of the MAC address. The value for Profinet Multicast is 01:0E:CF.
PA
    Process Automation (as opposed to production automation)
PCA
    Provider, Consumer or Alarm.
PCP
    Priority Code Point, for VLAN
PDEV
    Physical Device management. Physical interface and switch ports of a Profinet field device.
PDU
    Protocol Data Unit
PI
    PROFIBUS & PROFINET International. The Profinet interest group.
PICO
    PI Certification Office
PITL
    PI Test Laboratories. Performs certification testing.
PLC
    Programmable Logic Controller. Often used as a Profinet IO-controller.
PN
    See PROFINET
PNIO
    Profinet IO protocol
PNO
    PROFIBUS Nutzerorganisation e.V, located in Germany.
POF
    ?
PPM
    Cyclic Provider Protocol Machine
PROFINET
    Process Field Net
PS
    ?
PTCP
    Precision Transparent Clock Protocol
RPC
    Remote Procedure Call. The protocol DCE/RPC runs on UDP.
RS
    Reporting system
RSI
    ?
RTA
    RealTime Acyclic protocol
RTC
    RealTime Cyclic protocol
RTE
    Real Time Ethernet
RTOS
    Real Time Operating System
SA
    Source Address. It is the MAC address of the sender.
SAM
    Source Address of ? Uses to restrict incoming DCP communication to a single remote MAC address (for 3 seconds).
SCL
    Structured Control Language. Siemens name for the structured text (ST) programming language for PLCs.
SDU
    ?
SNMP
    Simple Network Management Protocol. For network topology detection.
ST
    Structured Text. A programming language for PLCs.
STX
    See ST.
TIA
    Totally Integrated Automation. An automation portal (engineering tool) by Siemens.
TCI
    Tool Calling Interface (The engineering tool can call specialized device-related tools)
TCP
    Transmission Control Protocol, used on top of IP.
TLV
    Type-Length-Value. A data structure in an LLDP Ethernet frame.
TPID
    Tag protocol identifier, for VLAN.
TSN
    Time-Sensitive Networking
TTL
   Time to live. A field in an LLDP Ethernet frames.
UC
    Unicast (as opposed to multicast)
UDP
    User Datagram Protocol
USI
    User Structure Identifier (unit16)
UUID
    Universally Unique Identifier
VLAN
    Virtual LAN
VID
    VLAN identifier
WLAN
    Wireless LAN
XML
    eXtended Markup Language
