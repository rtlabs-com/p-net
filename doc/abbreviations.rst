
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

AMR
    Asset Management Record
API
    Application Programming Identifier (uint32). Used to differentiate between for example user profiles. Sometimes named "Profile ID".
API
    Application Programming Interface. Application implementers use the API of the p-net Profinet stack.
APMR
    Acyclic Protocol Machine Receiver
APMS
    Acyclic Protocol Machine Sender
APMX
    General term for APMR and APMS
AR
    Application Relation
ARP
    Address Resolution Protocol
AREP
    Application Relationship End Point (uint32), pretty much an index into an array of AR.
ASE
    Application Service Element
CC
    Conformance Class (Performance level A to D)
CiR
    Configuration in Run
CM
    Context Management
CMDEV
    Context Management Protocol Machine Device
CMI
    CM Initiator
CMRDR
    Context Management Read Record Responder protocol machine device, responds to parameter read from the IO-controller
CMRPC
    Context Management RPC device protocol machine
CMSM
   Context Management Surveillance protocol Machine device, monitors the establishment of a connection. Also written as CM Server Protocol machine.
CP
    Communication profile. For example Profinet conformance class B is also known as CP 3/5.
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
DAP
    Device Access Point
DCE
    Distributed Computing Environment. Used with RPC.
DCP
    Discovery and basic Configuration Protocol. Runs over UDP?
DFP
    Dynamic Frame Packing. Used with IRT protocol.
DHCP
    Dynamic Host Configuration Protocol
DHT
    Data Hold Timer
DNS
    Domain Name System
DT
    Device Tool
DUT
    Device Under Test
EMC
    Electromagnetic Compatibility
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
IGMP
    Internet Group Management Protocol. For multicast groups. Used in IPv4.
IO
    Input Output
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
MAC
    Media Access Control
MAU
    Medium Attachment Unit. Ethernet transceiver type. 0x0 = radio, 0x10 = Media type copper 100BaseTXFD
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
OS
    Operating System
OUI
    Organizationally Unique Identifier. This is the three first bytes of the MAC address.
PA
    Process Automation (as opposed to production automation)
PCA
    Provider, Consumer or Alarm?
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
    Remote Procedure Call
RTA
    RealTime Acyclic protocol
RTC
    RealTime Cyclic protocol
RTE
    Real Time Ethernet
RTOS
    Real Time Operating System
SCL
    Structured Control Language. Siemens name for the structured text (ST) programming language for PLCs.
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
    Transmission Control Protocol
UDP
    User Datagram Protocol
USI
    User Structure Identifier (unit16)
UUID
    Universally Unique Identifier
XML
    eXtended Markup Language
