.. _network-topology-detection:

Network topology detection
==========================

A Profinet device of class B has the same features as a device class A, plus
the support for network topology detection. That allows tools to ask the
devices how they are connected, so it is possible to automatically draw a
network map. See for example the illustration in the section on the PRONETA
tool below.

LLDP
----
Link Layer Discovery Protocol (LLDP) is used to find information on the closest
neighbours on an Ethernet network. Each IO-device, IO-controller and (managed)
switch send out frames containing its name and the port name, for each of their
Ethernet ports.

LLDP packets should be sent every 5 seconds, and have a time-to-live value of 20 seconds.

Managed switches filter LLDP frames, and send its own LLDP frames.
That way everyone can know what is their closest neighbour. Only LLDP frames from
the closest neighbour is received on each port, thanks to the filtering done by
the switch.


SNMP
----
It is possible to ask an IO-Controller or IO-device (conformance class B or
higher) about its neighbours, using the Simple Network Management Protocol (SNMP).

SNMP agents collect information on individual devices, and the SNMP manager
retrieves information from them.

Some of the information can also be queried via the Profinet DCE/RPC protocol.

SNMP versions:

* v1 Mandatory for Profinet IO-devices and IO-Controllers
* v2c Supports 64-bit statistical counters
* v3 Encrypted


Monitor incoming LLDP frames on Linux
-------------------------------------
This is useful for experimenting with LLDP, but should not be running on the
same device as p-net. This is because p-net implements LLDP transmission and
reception itself.

Install the ``lldpd`` daemon::

   $ sudo apt install lldpd

It will start to collect information automatically, but will also start sending
LLDP packets on each interface every 30 seconds.

Show neighbour information::

   $ lldpcli show neighbors
   -------------------------------------------------------------------------------
   LLDP neighbors:
   -------------------------------------------------------------------------------
   Interface:    enp0s31f6, via: LLDP, RID: 1, Time: 0 day, 00:00:26
   Chassis:
      ChassisID:    local b
      SysName:      sysName Not Set
      SysDescr:     Siemens, SIMATIC NET, SCALANCE X204IRT, 6GK5 204-0BA00-2BA3, HW: Version 9, FW: Version V05.04.02, SVPL6147920
      MgmtIP:       192.168.0.99
      Capability:   Station, on
   Port:
      PortID:       local port-003
      PortDescr:    Siemens, SIMATIC NET, Ethernet Port, X1 P3
      TTL:          20
   Unknown TLVs:
      TLV:          OUI: 00,0E,CF, SubType: 1, Len: 20 00,00,01,BC,00,00,00,00,00,00,04,63,00,00,00,00,00,00,00,00
      TLV:          OUI: 00,0E,CF, SubType: 2, Len: 4 00,00,00,00
      TLV:          OUI: 00,0E,CF, SubType: 5, Len: 6 20,87,56,FF,AA,83
   -------------------------------------------------------------------------------

To stop the daemon (to avoid sending additional LLDP packets)::

   sudo systemctl stop lldpd.service
   sudo systemctl disable lldpd.service


SNMPwalk tool for querying SNMP agents
--------------------------------------
To install ``snmpwalk`` and standard description files on Ubuntu::

   sudo apt install snmp snmp-mibs-downloader

Use ``snmpwalk`` to read all info from a device. This is the result when
querying a Siemens Profinet-enabled switch::

   $ snmpwalk -v2c -c public 192.168.0.99
   iso.3.6.1.2.1.1.1.0 = STRING: "Siemens, SIMATIC NET, SCALANCE X204IRT, 6GK5 204-0BA00-2BA3, HW: Version 9, FW: Version V05.04.02, SVPL6147920"
   iso.3.6.1.2.1.1.2.0 = OID: iso.3.6.1.4.1.4196.1.1.5.2.5
   iso.3.6.1.2.1.1.3.0 = Timeticks: (602966) 1:40:29.66
   iso.3.6.1.2.1.1.4.0 = STRING: "sysContact Not Set"
   iso.3.6.1.2.1.1.5.0 = STRING: "sysName Not Set"
   iso.3.6.1.2.1.1.6.0 = STRING: "sysLocation Not Set"
   iso.3.6.1.2.1.1.7.0 = INTEGER: 67
   iso.3.6.1.2.1.2.1.0 = INTEGER: 5
   iso.3.6.1.2.1.2.2.1.1.1 = INTEGER: 1
   iso.3.6.1.2.1.2.2.1.1.2 = INTEGER: 2
   iso.3.6.1.2.1.2.2.1.1.3 = INTEGER: 3
   iso.3.6.1.2.1.2.2.1.1.4 = INTEGER: 4
   iso.3.6.1.2.1.2.2.1.1.1000 = INTEGER: 1000
   iso.3.6.1.2.1.2.2.1.2.1 = STRING: "Siemens, SIMATIC NET, Ethernet Port, X1 P1"
   iso.3.6.1.2.1.2.2.1.2.2 = STRING: "Siemens, SIMATIC NET, Ethernet Port, X1 P2"
   iso.3.6.1.2.1.2.2.1.2.3 = STRING: "Siemens, SIMATIC NET, Ethernet Port, X1 P3"
   iso.3.6.1.2.1.2.2.1.2.4 = STRING: "Siemens, SIMATIC NET, Ethernet Port, X1 P4"
   iso.3.6.1.2.1.2.2.1.2.1000 = STRING: "Siemens, SIMATIC NET, internal, X1"
   iso.3.6.1.2.1.2.2.1.3.1 = INTEGER: 6
   iso.3.6.1.2.1.2.2.1.3.2 = INTEGER: 6
   iso.3.6.1.2.1.2.2.1.3.3 = INTEGER: 6
   iso.3.6.1.2.1.2.2.1.3.4 = INTEGER: 6
   iso.3.6.1.2.1.2.2.1.3.1000 = INTEGER: 6
   iso.3.6.1.2.1.2.2.1.4.1 = INTEGER: 1500
   iso.3.6.1.2.1.2.2.1.4.2 = INTEGER: 1500
   iso.3.6.1.2.1.2.2.1.4.3 = INTEGER: 1500
   iso.3.6.1.2.1.2.2.1.4.4 = INTEGER: 1500
   iso.3.6.1.2.1.2.2.1.4.1000 = INTEGER: 1500
   iso.3.6.1.2.1.2.2.1.5.1 = Gauge32: 100000000
   iso.3.6.1.2.1.2.2.1.5.2 = Gauge32: 100000000
   iso.3.6.1.2.1.2.2.1.5.3 = Gauge32: 100000000
   iso.3.6.1.2.1.2.2.1.5.4 = Gauge32: 10000000
   iso.3.6.1.2.1.2.2.1.5.1000 = Gauge32: 0
   iso.3.6.1.2.1.2.2.1.6.1 = Hex-STRING: 20 87 56 FF AA 84
   iso.3.6.1.2.1.2.2.1.6.2 = Hex-STRING: 20 87 56 FF AA 85
   iso.3.6.1.2.1.2.2.1.6.3 = Hex-STRING: 20 87 56 FF AA 86
   iso.3.6.1.2.1.2.2.1.6.4 = Hex-STRING: 20 87 56 FF AA 87
   iso.3.6.1.2.1.2.2.1.6.1000 = Hex-STRING: 20 87 56 FF AA 83
   iso.3.6.1.2.1.2.2.1.7.1 = INTEGER: 1
   iso.3.6.1.2.1.2.2.1.7.2 = INTEGER: 1
   (etc)

Use the command line argument ``-v`` for the SNMP version to use, and ``-c``
for the "community string" which is a password.
The default community string for devices is often "public".

The values in the left column are the OID (Object Identifier) values.
For example ``1.3.6.1.2.1.2.2.1.6.3`` is used for the MAC address of the third
interface of the device. It can be interpreted as:

* 1 = iso
* 3 = identified-organization
* 6 = dod (US department of defence)
* 1 = internet
* 2 = mgmt
* 1 = mib-2
* 2 = interfaces
* 2 = ifTable
* 1 = ifEntry
* 6 = ifPhysAddress
* 3 = Third interface

To convert the digits to human readable text, a MIB (Management Information
Base) text file is used.

The example OID is defined in the ``IF-MIB``, which describes interface
information. The text is from :rfc:`2863`,
and typically installed in ``/var/lib/snmp/mibs/ietf/IF-MIB`` on Linux::

   ifPhysAddress OBJECT-TYPE
      SYNTAX      PhysAddress
      MAX-ACCESS  read-only
      STATUS      current
      DESCRIPTION
               "The interface's address at its protocol sub-layer.  For
               example, for an 802.x interface, this object normally
               contains a MAC address.  The interface's media-specific MIB
               must define the bit and byte ordering and the format of the
               value of this object.  For interfaces which do not have such
               an address (e.g., a serial line), this object should contain
               an octet string of zero length."
      ::= { ifEntry 6 }

Add the ``-m ALL`` flag to ``snmpwalk`` to use the installed MIB files. The
example line will then display as::

   IF-MIB::ifPhysAddress.3 = STRING: 20:87:56:ff:aa:86

instead of the previous::

   iso.3.6.1.2.1.2.2.1.6.3 = Hex-STRING: 20 87 56 FF AA 86

To always load specific MIB files, you can add them to a ``.snmp/snmp.conf``
configuration file in your home directory. For example::

    mibs +LLDP-MIB


Read and write a single OID
---------------------------
Read only the ``SNMPv2-MIB::sysLocation.0`` OID::

   $ snmpget -v1 -c public 192.168.0.99 1.3.6.1.2.1.1.6.0
   iso.3.6.1.2.1.1.6.0 = STRING: "sysLocation Not Set"

Write a new string describing the device location::

   $ snmpset -v1 -c private 192.168.0.99 1.3.6.1.2.1.1.6.0 s "My new location"


Using more MIB files for snmpwalk
---------------------------------
Add any additional MIB files in the directory ``.snmp/mibs/`` in your home
directory. For Profinet you need at least these files:

* IEC-62439-2-MGMT-MIB-20140509.mib
* IEC-62439-2-MON-MIB-20140509.mib
* LLDP-EXT-IEC61158-TYPE10-MIB.mib
* LLDP-MIB.my

The first three files are available in the test bundle (in subdirectory
``PN-Spec``) from the Profinet organization.

The last file is available for download on several locations.


Important SNMP subtrees and MIB files
-------------------------------------
The OID values are hierarchical (arranged in a tree).

+------------------------------+-------------------------------------------------------------+
| OID subtree                  | Name                                                        |
+==============================+=============================================================+
| 1.0.8802.1.1.2               | LLDP-MIB::lldpMIB                                           |
+------------------------------+-------------------------------------------------------------+
| 1.0.62439.1                  | IEC-62439-2-MIB::mrp                                        |
+------------------------------+-------------------------------------------------------------+
| 1.3.6.1.2.1                  | SNMPv2-SMI::mib-2 (default for snmpwalk)                    |
+------------------------------+-------------------------------------------------------------+
| 1.3.6.1.4.1.4196.1.1.5.2.100 | SNMPv2-SMI::enterprises.4196.1.1.5.2.100 (Siemens Scalance) |
+------------------------------+-------------------------------------------------------------+
| 1.3.6.1.4.1.4329.6           | SNMPv2-SMI::enterprises.4329.6                              |
+------------------------------+-------------------------------------------------------------+
| 1.3.6.1.6                    | SNMPv2-SMI::snmpV2                                          |
+------------------------------+-------------------------------------------------------------+

To convert a numerical OID to a human readable string::

   $ snmptranslate -m ALL 1.0.8802.1.1.2.1
   LLDP-MIB::lldpObjects

Note that a MIB file can insert entries inside existing subtrees.

Important Profinet-related MIB files:

+---------------------------+-----------------------------------------------------+
| MIB file                  | Description                                         |
+===========================+=====================================================+
| BRIDGE-MIB                | For MAC-layer bridges                               |
+---------------------------+-----------------------------------------------------+
| DISMAN-EXPRESSION-MIB     | Distributed Management                              |
+---------------------------+-----------------------------------------------------+
| IEC-62439-2-MIB           | Media Redundancy Protocol                           |
+---------------------------+-----------------------------------------------------+
| IF-MIB                    | Interfaces MIB (rfc 2863)                           |
+---------------------------+-----------------------------------------------------+
| LLDP-EXT-PNO-MIB          | Profinet extension of LLDP-MIB                      |
+---------------------------+-----------------------------------------------------+
| LLDP-MIB                  | LLDP MIB                                            |
+---------------------------+-----------------------------------------------------+
| RFC1213-MIB               | MIB-2                                               |
+---------------------------+-----------------------------------------------------+
| SNMP-USER-BASED-SM-MIB    | SNMP User-based Security Model (RFC 3414)           |
+---------------------------+-----------------------------------------------------+
| SNMP-VIEW-BASED-ACM-MIB   | View-based Access Control Model for SNMP            |
+---------------------------+-----------------------------------------------------+
| SNMP-COMMUNITY-MIB        | Coexistence between SNMP v1, v2c, v3 (RFC 2576)     |
+---------------------------+-----------------------------------------------------+
| SNMP-PROXY-MIB            | Parameter config of proxy forwarding (RFC 3413)     |
+---------------------------+-----------------------------------------------------+
| SNMP-NOTIFICATION-MIB     | Logging SNMP Notifications (RFC 3014)               |
+---------------------------+-----------------------------------------------------+
| SNMP-TARGET-MIB           | ? (RFC 3413)                                        |
+---------------------------+-----------------------------------------------------+
| SNMP-FRAMEWORK-MIB        | ? (RFC 3411)                                        |
+---------------------------+-----------------------------------------------------+
| SNMPv2-MIB                |                                                     |
+---------------------------+-----------------------------------------------------+
| SNMPv2-SMI                |                                                     |
+---------------------------+-----------------------------------------------------+

Other Profinet-relevant MIB files on Linux:

* HOST-RESOURCES-MIB
* IP-FORWARD-MIB
* IP-MIB
* NET-SNMP-AGENT-MIB
* UDP-MIB


Walking a subtree using snmpwalk
--------------------------------
By default the ``snmpwalk`` searches the ``SNMPv2-SMI::mib-2`` subtree. You can
search a smaller or larger subtree by giving the OID (or the corresponding
name) to ``snmpwalk`` as the argument after the host argument.

As an example, here are the number of found variables for different
subtrees for a Siemens Profinet switch:

+---------------------+------------------------------------------+-----------------+
| OID Subtree         | Subtree name                             | Found variables |
+=====================+==========================================+=================+
| 1.3.6.1.2.1.2.2.1.6 | IF-MIB::ifPhysAddress                    |               5 |
+---------------------+------------------------------------------+-----------------+
| 1.3.6.1.2.1.2.2.1   | IF-MIB::ifEntry                          |             110 |
+---------------------+------------------------------------------+-----------------+
| 1.3.6.1.2.1.2.2     | IF-MIB::ifTable                          |             110 |
+---------------------+------------------------------------------+-----------------+
| 1.3.6.1.2.1.2       | IF-MIB::interfaces                       |             110 |
+---------------------+------------------------------------------+-----------------+
| 1.3.6.1.2.1         | SNMPv2-SMI::mib-2 (default for snmpwalk) |             408 |
+---------------------+------------------------------------------+-----------------+
| 1.3.6.1.2           | SNMPv2-SMI::mgmt                         |             408 |
+---------------------+------------------------------------------+-----------------+
| 1.3.6.1             | SNMPv2-SMI::internet                     |            2397 |
+---------------------+------------------------------------------+-----------------+
| 1.3.6               | SNMPv2-SMI::dod                          |            2397 |
+---------------------+------------------------------------------+-----------------+
| 1.3                 | SNMPv2-SMI::org                          |            2397 |
+---------------------+------------------------------------------+-----------------+
| 1                   | iso                                      |            2669 |
+---------------------+------------------------------------------+-----------------+

In order to show all available variables when running ``snmpwalk``, use the
OID ``1``. However, in this example this would result in displaying over 2600
variables.

To show relevant data only, you typically need to specify a subtree.
This is done by giving the OID value or corresponding name, for example
``1.0.8802.1.1.2`` or ``LLDP-MIB::lldpMIB``.


Showing SNMP info in tabular format
-----------------------------------
Some of the OIDs are part of tables, for example the OID ``1.3.6.1.2.1.2.2.1.2``
which is the ``ifDescr`` (descriptive text for Ethernet interface).
It is part of the table ``IF-MIB::ifTable`` which uses the ``ifIndex``
(interface index) as the table index, and the ``ifIndex`` is appended to the
end of the OID. Thus:

* 1.3.6.1.2.1.2.2.1.2.1 ``ifDescr`` for first interface (actually a port in Profinet naming)
* 1.3.6.1.2.1.2.2.1.2.2 ``ifDescr`` for second interface
* 1.3.6.1.2.1.2.2.1.2.3 ``ifDescr`` for third interface

This SNMP table can also be displayed in a tabular format on the command
line (the symbolic name must be given)::

   snmptable -v1 -c public 192.168.0.50 IF-MIB::ifTable

Add ``-Cw 120`` to the command line to limit the ``snmptable`` output line length.

Relevant tables:

+-----------------------------------------+--------------------------------+
| Table name                              | Top OID                        |
+=========================================+================================+
| IF-MIB::ifTable                         | 1.3.6.1.2.1.2.2                |
+-----------------------------------------+--------------------------------+
| LLDP-MIB::lldpConfigManAddrTable        | 1.0.8802.1.1.2.1.1.7           |
+-----------------------------------------+--------------------------------+
| LLDP-MIB::lldpLocPortTable              | 1.0.8802.1.1.2.1.3.7           |
+-----------------------------------------+--------------------------------+
| LLDP-MIB::lldpLocManAddrTable           | 1.0.8802.1.1.2.1.3.8           |
+-----------------------------------------+--------------------------------+
| LLDP-MIB::lldpRemTable                  | 1.0.8802.1.1.2.1.4.1           |
+-----------------------------------------+--------------------------------+
| LLDP-MIB::lldpRemManAddrTable           | 1.0.8802.1.1.2.1.4.2           |
+-----------------------------------------+--------------------------------+
| LLDP-EXT-PNO-MIB::lldpXPnoLocTable      | 1.0.8802.1.1.2.1.5.3791.1.2.1  |
+-----------------------------------------+--------------------------------+
| LLDP-EXT-PNO-MIB::lldpXPnoRemTable      | 1.0.8802.1.1.2.1.5.3791.1.3.1  |
+-----------------------------------------+--------------------------------+


Supported SNMP variables for Profinet
-------------------------------------

Device details:

+--------------------------------+-----------------------------------------------------------------+
| Field name                     | Description                                                     |
+================================+=================================================================+
| sysDescr                       | String, max 255 char. Consists of product name, serial number,  |
|                                | hardware and software versions etc.                             |
|                                | Also used as SystemIdentification = ChassisID ?                 |
+--------------------------------+-----------------------------------------------------------------+
| sysObjectId                    | An OID with enterprise info. Use 1.3.6.1.4.1.24686 for Profinet |
+--------------------------------+-----------------------------------------------------------------+
| sysUpTime                      | Uptime in 1/100 seconds                                         |
+--------------------------------+-----------------------------------------------------------------+
| sysContact                     | String, max 255 char. Writable. Contact info to a person.       |
+--------------------------------+-----------------------------------------------------------------+
| sysName                        | String, writable. Fully qualified domain name?                  |
|                                | Here limited to PNAL_HOSTNAME_MAX_SIZE (typically 64 char).     |
+--------------------------------+-----------------------------------------------------------------+
| sysLocation                    | String, writable. Here same as I&M1 location (max 22 char).     |
+--------------------------------+-----------------------------------------------------------------+
| sysServices                    | 78 (dec) for Profinet devices                                   |
+--------------------------------+-----------------------------------------------------------------+
| lldpConfigManAddrPortsTxEnable | On/off for ports. Writable?                                     |
+--------------------------------+-----------------------------------------------------------------+

Interface statistics, for each port:

+----------------+-----------------------------------------------------------------+
| Field name     | Description                                                     |
+================+=================================================================+
| ifIndex        |                                                                 |
+----------------+-----------------------------------------------------------------+
| ifDescr        | String, max 255 char. Unique within device. See lldpLocPortDesc |
+----------------+-----------------------------------------------------------------+
| ifType         | Typically 6 = Ethernet                                          |
+----------------+-----------------------------------------------------------------+
| ifMtu          | Max bytes per packet. Often 1500.                               |
+----------------+-----------------------------------------------------------------+
| ifSpeed        | Bits/s. uint32. Often 100000000.                                |
+----------------+-----------------------------------------------------------------+
| ifPhysAddress  | MAC address                                                     |
+----------------+-----------------------------------------------------------------+
| ifAdminStatus  | Up, down etc. Writable?                                         |
+----------------+-----------------------------------------------------------------+
| ifOperStatus   | Up, down etc.                                                   |
+----------------+-----------------------------------------------------------------+
| ifInOctets     | Input bytes. uint32                                             |
+----------------+-----------------------------------------------------------------+
| ifInDiscards   | uint32                                                          |
+----------------+-----------------------------------------------------------------+
| ifInErrors     | uint32                                                          |
+----------------+-----------------------------------------------------------------+
| ifOutOctets    | Output bytes. uint32                                            |
+----------------+-----------------------------------------------------------------+
| ifOutDiscards  | uint32                                                          |
+----------------+-----------------------------------------------------------------+
| ifOutErrors    | uint32                                                          |
+----------------+-----------------------------------------------------------------+

Readable fields related to ports and interfaces:

+----------------------+--------------------------------------+-----------------+------------+-------------+
| Field                | Description                          | Local interface | Local port | Remote port |
+======================+======================================+=================+============+=============+
| ChassisId            | Chassis ID (same for all interfaces) | x               |            | x           |
+----------------------+--------------------------------------+-----------------+------------+-------------+
| TimeMark             | Timestamp for latest LLDP frame      |                 |            | x           |
+----------------------+--------------------------------------+-----------------+------------+-------------+
| LocalPortNum         | Port number                          |                 | x          | x           |
+----------------------+--------------------------------------+-----------------+------------+-------------+
| PortId               | String, max 14 char.                 |                 | x          | x           |
+----------------------+--------------------------------------+-----------------+------------+-------------+
| PortIdSubtype        | Typically 7 = Locally assigned       |                 | x          | o           |
+----------------------+--------------------------------------+-----------------+------------+-------------+
| PortDesc             | String, max 255 char. See ifDescr.   |                 | x          |             |
+----------------------+--------------------------------------+-----------------+------------+-------------+
| Index                |                                      |                 |            | x           |
+----------------------+--------------------------------------+-----------------+------------+-------------+
| ManAddrSubtype       | Typically 1=IP                       | x               |            | x           |
+----------------------+--------------------------------------+-----------------+------------+-------------+
| ManAddr              | Management (IP) address              | x               |            | x           |
+----------------------+--------------------------------------+-----------------+------------+-------------+
| ManAddrIfId          | See ifIndex                          | x               |            | x           |
+----------------------+--------------------------------------+-----------------+------------+-------------+
| LPDValue             | Propagation delay in ns. uint32      |                 | x          | x           |
+----------------------+--------------------------------------+-----------------+------------+-------------+
| PortTxDValue         | Transmission delay in ns. uint32     |                 | x          | x           |
+----------------------+--------------------------------------+-----------------+------------+-------------+
| PortRxDValue         | Reception delay in ns. uint32        |                 | x          | x           |
+----------------------+--------------------------------------+-----------------+------------+-------------+
| NoS                  | Station name (or MAC address as      | x               |            | x           |
|                      | string) String                       |                 |            |             |
+----------------------+--------------------------------------+-----------------+------------+-------------+
| AutoNegSupported     | Autonegotiation supported. Bool.     |                 | x          | o           |
+----------------------+--------------------------------------+-----------------+------------+-------------+
| AutoNegEnabled       | Autonegotiation enabled. Bool.       |                 | x          | x           |
+----------------------+--------------------------------------+-----------------+------------+-------------+
| AutoNegAdvertisedCap |                                      |                 | o          | o           |
+----------------------+--------------------------------------+-----------------+------------+-------------+
| OperMauType          | MAU type                             |                 | x          | x           |
+----------------------+--------------------------------------+-----------------+------------+-------------+

Note that some objects are listed "Not accessible" in the standard. These
are read indirectly via other objects (used as table index, and appended
to the OID of other objects), so the information must thus be available.

See the Profinet standard for the corresponding numerical OID values.

Enable SNMP when running p-net on Linux
---------------------------------------
See the page "Additional Linux details" in this documentation.


Verification of SNMP communication to p-net
-------------------------------------------
To verify the SNMP capabilities of the p-net stack, first use ``ping`` to
make sure you have a connection to the device, and then use ``snmpwalk``::

   ping 192.168.0.50
   snmpwalk -v1 -c public 192.168.0.50 1
   snmpget -v1 -c public 192.168.0.50 1.3.6.1.2.1.1.4.0
   snmpset -v1 -c private 192.168.0.50 1.3.6.1.2.1.1.4.0 s "My new sys contact"

If you enable debug logging in the p-net stack, the two last commands will
cause entries in the p-net log.

This will only be answered if the agent is Profinet-enabled::

   snmpget -v1 -c public 192.168.0.50 1.0.8802.1.1.2.1.5.3791.1.2.1.1.1.1

The ``sysObjectId`` should be reported as ``iso.3.6.1.4.1.24686`` if the
agent is Profinet-enabled::

   snmpget -v1 -c public 192.168.0.50 1.3.6.1.2.1.1.2.0


Verification of multiport SNMP functionality for p-net
------------------------------------------------------
Ask the device about its neighbours on the different ports by running::

   snmpwalk -v1 -c public 192.168.0.50  1.0.8802.1.1.2.1.4.1

This will use the LldpRemTable to show peer ChassisID and peer PortId for the
different local ports.
Connect some other LLDP-enabled device (Profinet or not) to one of the ports,
and verify that it will show up in the results.

A while after disconnecting the device (controlled by the TTL value)
the entry will be removed from the list.
Verify this behavior for all ports on your device.


Siemens PRONETA - Profinet Network Analysis tool
------------------------------------------------
The Proneta tool can scan the network to discover the topology of connected
Profinet equipment.

To install Proneta on Windows, use the "Download Proneta Basic free" link
on the Siemens web page:
https://new.siemens.com/global/en/products/automation/industrial-communication/profinet/proneta.html
The file is named ``proneta_VERSION.zip``. Registration is required.
Unzip the downloaded file, and double-click the ``PRONETA.exe`` file to start the program.

Turn off the ART tester tool before starting Proneta, otherwise Proneta can
not scan the network.

On the settings page, select which Ethernet network adapter to use
on your computer.
On the home screen, select "Network analysis" and use the "Online" tab.
Click the "Refresh" icon to scan the network topology.
A graphical view with all Profinet equipment will be shown, including
the connections between all ports. An example screenshot is shown below.

.. image:: illustrations/proneta_example.png

For the Proneta tool to be able to fully map the network, most of the nodes
must have valid IP addresses (and have SNMP enabled).

The program will show details about interfaces and ports, and which of the slots
that are populated by modules.
It will also show the list of current diagnosis for IO-devices.
Proneta will read out the diagnostics buffer every 5 seconds, as long as the
device is selected in the graphical view.
In order to display the diagnostics buffer the IP address of the device must be
set. Otherwise the message "Diagnostics buffer not supported" will be shown.

By right-clicking on a device in the graphical view, you can set the
station name and IP address (temporarily or permanently).
It is also possible to edit the I&M information, to flash the signal LED and
to do a factory reset.

For PRONETA to be able to map the connection to the closest device, you need to
enable LLDP transmission on the Ethernet interface of your laptop. Note that
this interferes with the ART tester software. See the section on installing
ART tester in this documentation.

Except from sending the "DCP identify", Proneta will scan the subnet with
PING and ARP messages if enabled in the settings.
It is possible to customise which address range to scan.
Found DCP-enabled hosts will be probed using EPM lookup and SNMP queries.

Proneta will show details about modules in found devices, if it has access to the
corresponding GSDML file. Use the page "Settings" and the tab "GSDML Manager" to
load GSDML files.


Full SNMP readout example
-------------------------
Here is more data from the Siemens switch, when using the MIBs::

   $ snmpwalk -v2c -c public -m ALL 192.168.0.99
   SNMPv2-MIB::sysDescr.0 = STRING: Siemens, SIMATIC NET, SCALANCE X204IRT, 6GK5 204-0BA00-2BA3, HW: Version 9, FW: Version V05.04.02, SVPL6147920
   SNMPv2-MIB::sysObjectID.0 = OID: SNMPv2-SMI::enterprises.4196.1.1.5.2.5
   DISMAN-EXPRESSION-MIB::sysUpTimeInstance = Timeticks: (756575) 2:06:05.75
   SNMPv2-MIB::sysContact.0 = STRING: sysContact Not Set
   SNMPv2-MIB::sysName.0 = STRING: sysName Not Set
   SNMPv2-MIB::sysLocation.0 = STRING: sysLocation Not Set
   SNMPv2-MIB::sysServices.0 = INTEGER: 67
   IF-MIB::ifNumber.0 = INTEGER: 5
   IF-MIB::ifIndex.1 = INTEGER: 1
   IF-MIB::ifIndex.2 = INTEGER: 2
   IF-MIB::ifIndex.3 = INTEGER: 3
   IF-MIB::ifIndex.4 = INTEGER: 4
   IF-MIB::ifIndex.1000 = INTEGER: 1000
   IF-MIB::ifDescr.1 = STRING: Siemens, SIMATIC NET, Ethernet Port, X1 P1
   IF-MIB::ifDescr.2 = STRING: Siemens, SIMATIC NET, Ethernet Port, X1 P2
   IF-MIB::ifDescr.3 = STRING: Siemens, SIMATIC NET, Ethernet Port, X1 P3
   IF-MIB::ifDescr.4 = STRING: Siemens, SIMATIC NET, Ethernet Port, X1 P4
   IF-MIB::ifDescr.1000 = STRING: Siemens, SIMATIC NET, internal, X1
   IF-MIB::ifType.1 = INTEGER: ethernetCsmacd(6)
   IF-MIB::ifType.2 = INTEGER: ethernetCsmacd(6)
   IF-MIB::ifType.3 = INTEGER: ethernetCsmacd(6)
   IF-MIB::ifType.4 = INTEGER: ethernetCsmacd(6)
   IF-MIB::ifType.1000 = INTEGER: ethernetCsmacd(6)
   IF-MIB::ifMtu.1 = INTEGER: 1500
   IF-MIB::ifMtu.2 = INTEGER: 1500
   IF-MIB::ifMtu.3 = INTEGER: 1500
   IF-MIB::ifMtu.4 = INTEGER: 1500
   IF-MIB::ifMtu.1000 = INTEGER: 1500
   IF-MIB::ifSpeed.1 = Gauge32: 10000000
   IF-MIB::ifSpeed.2 = Gauge32: 100000000
   IF-MIB::ifSpeed.3 = Gauge32: 100000000
   IF-MIB::ifSpeed.4 = Gauge32: 10000000
   IF-MIB::ifSpeed.1000 = Gauge32: 0
   IF-MIB::ifPhysAddress.1 = STRING: 20:87:56:ff:aa:84
   IF-MIB::ifPhysAddress.2 = STRING: 20:87:56:ff:aa:85
   IF-MIB::ifPhysAddress.3 = STRING: 20:87:56:ff:aa:86
   IF-MIB::ifPhysAddress.4 = STRING: 20:87:56:ff:aa:87
   IF-MIB::ifPhysAddress.1000 = STRING: 20:87:56:ff:aa:83
   IF-MIB::ifAdminStatus.1 = INTEGER: up(1)
   IF-MIB::ifAdminStatus.2 = INTEGER: up(1)
   IF-MIB::ifAdminStatus.3 = INTEGER: up(1)
   IF-MIB::ifAdminStatus.4 = INTEGER: up(1)
   IF-MIB::ifAdminStatus.1000 = INTEGER: up(1)
   IF-MIB::ifOperStatus.1 = INTEGER: up(1)
   IF-MIB::ifOperStatus.2 = INTEGER: up(1)
   IF-MIB::ifOperStatus.3 = INTEGER: up(1)
   IF-MIB::ifOperStatus.4 = INTEGER: down(2)
   IF-MIB::ifOperStatus.1000 = INTEGER: up(1)
   IF-MIB::ifLastChange.1 = Timeticks: (616788) 1:42:47.88
   IF-MIB::ifLastChange.2 = Timeticks: (436359) 1:12:43.59
   IF-MIB::ifLastChange.3 = Timeticks: (1094) 0:00:10.94
   IF-MIB::ifLastChange.4 = Timeticks: (0) 0:00:00.00
   IF-MIB::ifLastChange.1000 = Timeticks: (0) 0:00:00.00
   IF-MIB::ifInOctets.1 = Counter32: 908950
   IF-MIB::ifInOctets.2 = Counter32: 214915
   IF-MIB::ifInOctets.3 = Counter32: 576327
   IF-MIB::ifInOctets.4 = Counter32: 0
   IF-MIB::ifInOctets.1000 = Counter32: 0
   IF-MIB::ifInUcastPkts.1 = Counter32: 911
   IF-MIB::ifInUcastPkts.2 = Counter32: 308
   IF-MIB::ifInUcastPkts.3 = Counter32: 6185
   IF-MIB::ifInUcastPkts.4 = Counter32: 0
   IF-MIB::ifInUcastPkts.1000 = Counter32: 0
   IF-MIB::ifInNUcastPkts.1 = Counter32: 7840
   IF-MIB::ifInNUcastPkts.2 = Counter32: 1521
   IF-MIB::ifInNUcastPkts.3 = Counter32: 99
   IF-MIB::ifInNUcastPkts.4 = Counter32: 0
   IF-MIB::ifInNUcastPkts.1000 = Counter32: 0
   IF-MIB::ifInDiscards.1 = Counter32: 0
   IF-MIB::ifInDiscards.2 = Counter32: 0
   IF-MIB::ifInDiscards.3 = Counter32: 0
   IF-MIB::ifInDiscards.4 = Counter32: 0
   IF-MIB::ifInDiscards.1000 = Counter32: 0
   IF-MIB::ifInErrors.1 = Counter32: 0
   IF-MIB::ifInErrors.2 = Counter32: 0
   IF-MIB::ifInErrors.3 = Counter32: 0
   IF-MIB::ifInErrors.4 = Counter32: 0
   IF-MIB::ifInErrors.1000 = Counter32: 0
   IF-MIB::ifInUnknownProtos.1 = Counter32: 492
   IF-MIB::ifInUnknownProtos.2 = Counter32: 19
   IF-MIB::ifInUnknownProtos.3 = Counter32: 21
   IF-MIB::ifInUnknownProtos.4 = Counter32: 0
   IF-MIB::ifInUnknownProtos.1000 = Counter32: 0
   IF-MIB::ifOutOctets.1 = Counter32: 1516306
   IF-MIB::ifOutOctets.2 = Counter32: 596825
   IF-MIB::ifOutOctets.3 = Counter32: 2244836
   IF-MIB::ifOutOctets.4 = Counter32: 0
   IF-MIB::ifOutOctets.1000 = Counter32: 0
   IF-MIB::ifOutUcastPkts.1 = Counter32: 5835
   IF-MIB::ifOutUcastPkts.2 = Counter32: 385
   IF-MIB::ifOutUcastPkts.3 = Counter32: 6649
   IF-MIB::ifOutUcastPkts.4 = Counter32: 0
   IF-MIB::ifOutUcastPkts.1000 = Counter32: 0
   IF-MIB::ifOutNUcastPkts.1 = Counter32: 7470
   IF-MIB::ifOutNUcastPkts.2 = Counter32: 5352
   IF-MIB::ifOutNUcastPkts.3 = Counter32: 15626
   IF-MIB::ifOutNUcastPkts.4 = Counter32: 0
   IF-MIB::ifOutNUcastPkts.1000 = Counter32: 0
   IF-MIB::ifOutDiscards.1 = Counter32: 0
   IF-MIB::ifOutDiscards.2 = Counter32: 0
   IF-MIB::ifOutDiscards.3 = Counter32: 0
   IF-MIB::ifOutDiscards.4 = Counter32: 0
   IF-MIB::ifOutDiscards.1000 = Counter32: 0
   IF-MIB::ifOutErrors.1 = Counter32: 0
   IF-MIB::ifOutErrors.2 = Counter32: 0
   IF-MIB::ifOutErrors.3 = Counter32: 0
   IF-MIB::ifOutErrors.4 = Counter32: 0
   IF-MIB::ifOutErrors.1000 = Counter32: 0
   IF-MIB::ifOutQLen.1 = Gauge32: 0
   IF-MIB::ifOutQLen.2 = Gauge32: 0
   IF-MIB::ifOutQLen.3 = Gauge32: 0
   IF-MIB::ifOutQLen.4 = Gauge32: 0
   IF-MIB::ifOutQLen.1000 = Gauge32: 15
   IF-MIB::ifSpecific.1 = OID: SNMPv2-SMI::zeroDotZero
   IF-MIB::ifSpecific.2 = OID: SNMPv2-SMI::zeroDotZero
   IF-MIB::ifSpecific.3 = OID: SNMPv2-SMI::zeroDotZero
   IF-MIB::ifSpecific.4 = OID: SNMPv2-SMI::zeroDotZero
   IF-MIB::ifSpecific.1000 = OID: SNMPv2-SMI::zeroDotZero
   RFC1213-MIB::ipForwarding.0 = INTEGER: not-forwarding(2)
   RFC1213-MIB::ipDefaultTTL.0 = INTEGER: 64
   RFC1213-MIB::ipInReceives.0 = Counter32: 13017
   RFC1213-MIB::ipInHdrErrors.0 = Counter32: 0
   RFC1213-MIB::ipInAddrErrors.0 = Counter32: 0
   RFC1213-MIB::ipForwDatagrams.0 = Counter32: 0
   RFC1213-MIB::ipInUnknownProtos.0 = Counter32: 0
   RFC1213-MIB::ipInDiscards.0 = Counter32: 0
   RFC1213-MIB::ipInDelivers.0 = Counter32: 13023
   RFC1213-MIB::ipOutRequests.0 = Counter32: 13014
   RFC1213-MIB::ipOutDiscards.0 = Counter32: 0
   RFC1213-MIB::ipOutNoRoutes.0 = Counter32: 0
   RFC1213-MIB::ipReasmTimeout.0 = INTEGER: 60
   RFC1213-MIB::ipReasmReqds.0 = Counter32: 0
   RFC1213-MIB::ipReasmOKs.0 = Counter32: 0
   RFC1213-MIB::ipReasmFails.0 = Counter32: 0
   RFC1213-MIB::ipFragOKs.0 = Counter32: 0
   RFC1213-MIB::ipFragFails.0 = Counter32: 0
   RFC1213-MIB::ipFragCreates.0 = Counter32: 0
   RFC1213-MIB::ipAdEntAddr.192.168.0.99 = IpAddress: 192.168.0.99
   RFC1213-MIB::ipAdEntIfIndex.192.168.0.99 = INTEGER: 1000
   RFC1213-MIB::ipAdEntNetMask.192.168.0.99 = IpAddress: 255.255.255.0
   RFC1213-MIB::ipAdEntBcastAddr.192.168.0.99 = INTEGER: 1
   RFC1213-MIB::ipAdEntReasmMaxSize.192.168.0.99 = INTEGER: 65535
   RFC1213-MIB::ipRouteDest.127.0.0.1 = IpAddress: 127.0.0.1
   RFC1213-MIB::ipRouteDest.192.168.0.0 = IpAddress: 192.168.0.0
   RFC1213-MIB::ipRouteIfIndex.127.0.0.1 = INTEGER: 65535
   RFC1213-MIB::ipRouteIfIndex.192.168.0.0 = INTEGER: 65535
   RFC1213-MIB::ipRouteMetric1.127.0.0.1 = INTEGER: 0
   RFC1213-MIB::ipRouteMetric1.192.168.0.0 = INTEGER: 0
   RFC1213-MIB::ipRouteMetric2.127.0.0.1 = INTEGER: 0
   RFC1213-MIB::ipRouteMetric2.192.168.0.0 = INTEGER: 0
   RFC1213-MIB::ipRouteMetric3.127.0.0.1 = INTEGER: 0
   RFC1213-MIB::ipRouteMetric3.192.168.0.0 = INTEGER: 0
   RFC1213-MIB::ipRouteMetric4.127.0.0.1 = INTEGER: 0
   RFC1213-MIB::ipRouteMetric4.192.168.0.0 = INTEGER: 0
   RFC1213-MIB::ipRouteNextHop.127.0.0.1 = IpAddress: 127.0.0.1
   RFC1213-MIB::ipRouteNextHop.192.168.0.0 = IpAddress: 192.168.0.99
   RFC1213-MIB::ipRouteType.127.0.0.1 = INTEGER: direct(3)
   RFC1213-MIB::ipRouteType.192.168.0.0 = INTEGER: direct(3)
   RFC1213-MIB::ipRouteProto.127.0.0.1 = INTEGER: local(2)
   RFC1213-MIB::ipRouteProto.192.168.0.0 = INTEGER: local(2)
   RFC1213-MIB::ipRouteAge.127.0.0.1 = INTEGER: 7563
   RFC1213-MIB::ipRouteAge.192.168.0.0 = INTEGER: 7562
   RFC1213-MIB::ipRouteMask.127.0.0.1 = IpAddress: 255.255.255.255
   RFC1213-MIB::ipRouteMask.192.168.0.0 = IpAddress: 255.255.255.0
   RFC1213-MIB::ipRouteMetric5.127.0.0.1 = INTEGER: 0
   RFC1213-MIB::ipRouteMetric5.192.168.0.0 = INTEGER: 0
   RFC1213-MIB::ipRouteInfo.127.0.0.1 = OID: SNMPv2-SMI::zeroDotZero
   RFC1213-MIB::ipRouteInfo.192.168.0.0 = OID: SNMPv2-SMI::zeroDotZero
   RFC1213-MIB::ipNetToMediaIfIndex.8.192.168.0.25 = INTEGER: 8
   RFC1213-MIB::ipNetToMediaIfIndex.8.192.168.0.50 = INTEGER: 8
   RFC1213-MIB::ipNetToMediaPhysAddress.8.192.168.0.25 = Hex-STRING: 1C 39 47 CD D4 EB
   RFC1213-MIB::ipNetToMediaPhysAddress.8.192.168.0.50 = Hex-STRING: 54 EE 75 FF 95 A6
   RFC1213-MIB::ipNetToMediaNetAddress.8.192.168.0.25 = IpAddress: 192.168.0.25
   RFC1213-MIB::ipNetToMediaNetAddress.8.192.168.0.50 = IpAddress: 192.168.0.50
   RFC1213-MIB::ipNetToMediaType.8.192.168.0.25 = INTEGER: invalid(2)
   RFC1213-MIB::ipNetToMediaType.8.192.168.0.50 = INTEGER: dynamic(3)
   RFC1213-MIB::ipRoutingDiscards.0 = Counter32: 0
   RFC1213-MIB::icmpInMsgs.0 = Counter32: 0
   RFC1213-MIB::icmpInErrors.0 = Counter32: 0
   RFC1213-MIB::icmpInDestUnreachs.0 = Counter32: 0
   RFC1213-MIB::icmpInTimeExcds.0 = Counter32: 0
   RFC1213-MIB::icmpInParmProbs.0 = Counter32: 0
   RFC1213-MIB::icmpInSrcQuenchs.0 = Counter32: 0
   RFC1213-MIB::icmpInRedirects.0 = Counter32: 0
   RFC1213-MIB::icmpInEchos.0 = Counter32: 0
   RFC1213-MIB::icmpInEchoReps.0 = Counter32: 0
   RFC1213-MIB::icmpInTimestamps.0 = Counter32: 0
   RFC1213-MIB::icmpInTimestampReps.0 = Counter32: 0
   RFC1213-MIB::icmpInAddrMasks.0 = Counter32: 0
   RFC1213-MIB::icmpInAddrMaskReps.0 = Counter32: 0
   RFC1213-MIB::icmpOutMsgs.0 = Counter32: 0
   RFC1213-MIB::icmpOutErrors.0 = Counter32: 0
   RFC1213-MIB::icmpOutDestUnreachs.0 = Counter32: 0
   RFC1213-MIB::icmpOutTimeExcds.0 = Counter32: 0
   RFC1213-MIB::icmpOutParmProbs.0 = Counter32: 0
   RFC1213-MIB::icmpOutSrcQuenchs.0 = Counter32: 0
   RFC1213-MIB::icmpOutRedirects.0 = Counter32: 0
   RFC1213-MIB::icmpOutEchos.0 = Counter32: 0
   RFC1213-MIB::icmpOutEchoReps.0 = Counter32: 0
   RFC1213-MIB::icmpOutTimestamps.0 = Counter32: 0
   RFC1213-MIB::icmpOutTimestampReps.0 = Counter32: 0
   RFC1213-MIB::icmpOutAddrMasks.0 = Counter32: 0
   RFC1213-MIB::icmpOutAddrMaskReps.0 = Counter32: 0
   RFC1213-MIB::tcpRtoAlgorithm.0 = INTEGER: vanj(4)
   RFC1213-MIB::tcpRtoMin.0 = INTEGER: 50000
   RFC1213-MIB::tcpRtoMax.0 = INTEGER: 3200000
   RFC1213-MIB::tcpMaxConn.0 = INTEGER: -1
   RFC1213-MIB::tcpActiveOpens.0 = Counter32: 0
   RFC1213-MIB::tcpPassiveOpens.0 = Counter32: 0
   RFC1213-MIB::tcpAttemptFails.0 = Counter32: 0
   RFC1213-MIB::tcpEstabResets.0 = Counter32: 0
   RFC1213-MIB::tcpCurrEstab.0 = Gauge32: 0
   RFC1213-MIB::tcpInSegs.0 = Counter32: 0
   RFC1213-MIB::tcpOutSegs.0 = Counter32: 0
   RFC1213-MIB::tcpRetransSegs.0 = Counter32: 0
   RFC1213-MIB::tcpConnState.0.0.0.0.22.0.0.0.0.0 = INTEGER: listen(2)
   RFC1213-MIB::tcpConnState.0.0.0.0.23.0.0.0.0.0 = INTEGER: listen(2)
   RFC1213-MIB::tcpConnState.0.0.0.0.80.0.0.0.0.0 = INTEGER: listen(2)
   RFC1213-MIB::tcpConnState.0.0.0.0.84.0.0.0.0.0 = INTEGER: listen(2)
   RFC1213-MIB::tcpConnState.0.0.0.0.443.0.0.0.0.0 = INTEGER: listen(2)
   RFC1213-MIB::tcpConnLocalAddress.0.0.0.0.22.0.0.0.0.0 = IpAddress: 0.0.0.0
   RFC1213-MIB::tcpConnLocalAddress.0.0.0.0.23.0.0.0.0.0 = IpAddress: 0.0.0.0
   RFC1213-MIB::tcpConnLocalAddress.0.0.0.0.80.0.0.0.0.0 = IpAddress: 0.0.0.0
   RFC1213-MIB::tcpConnLocalAddress.0.0.0.0.84.0.0.0.0.0 = IpAddress: 0.0.0.0
   RFC1213-MIB::tcpConnLocalAddress.0.0.0.0.443.0.0.0.0.0 = IpAddress: 0.0.0.0
   RFC1213-MIB::tcpConnLocalPort.0.0.0.0.22.0.0.0.0.0 = INTEGER: 22
   RFC1213-MIB::tcpConnLocalPort.0.0.0.0.23.0.0.0.0.0 = INTEGER: 23
   RFC1213-MIB::tcpConnLocalPort.0.0.0.0.80.0.0.0.0.0 = INTEGER: 80
   RFC1213-MIB::tcpConnLocalPort.0.0.0.0.84.0.0.0.0.0 = INTEGER: 84
   RFC1213-MIB::tcpConnLocalPort.0.0.0.0.443.0.0.0.0.0 = INTEGER: 443
   RFC1213-MIB::tcpConnRemAddress.0.0.0.0.22.0.0.0.0.0 = IpAddress: 0.0.0.0
   RFC1213-MIB::tcpConnRemAddress.0.0.0.0.23.0.0.0.0.0 = IpAddress: 0.0.0.0
   RFC1213-MIB::tcpConnRemAddress.0.0.0.0.80.0.0.0.0.0 = IpAddress: 0.0.0.0
   RFC1213-MIB::tcpConnRemAddress.0.0.0.0.84.0.0.0.0.0 = IpAddress: 0.0.0.0
   RFC1213-MIB::tcpConnRemAddress.0.0.0.0.443.0.0.0.0.0 = IpAddress: 0.0.0.0
   RFC1213-MIB::tcpConnRemPort.0.0.0.0.22.0.0.0.0.0 = INTEGER: 0
   RFC1213-MIB::tcpConnRemPort.0.0.0.0.23.0.0.0.0.0 = INTEGER: 0
   RFC1213-MIB::tcpConnRemPort.0.0.0.0.80.0.0.0.0.0 = INTEGER: 0
   RFC1213-MIB::tcpConnRemPort.0.0.0.0.84.0.0.0.0.0 = INTEGER: 0
   RFC1213-MIB::tcpConnRemPort.0.0.0.0.443.0.0.0.0.0 = INTEGER: 0
   RFC1213-MIB::tcpInErrs.0 = Counter32: 0
   RFC1213-MIB::tcpOutRsts.0 = Counter32: 0
   RFC1213-MIB::udpInDatagrams.0 = Counter32: 13139
   RFC1213-MIB::udpNoPorts.0 = Counter32: 0
   RFC1213-MIB::udpInErrors.0 = Counter32: 0
   RFC1213-MIB::udpOutDatagrams.0 = Counter32: 13132
   RFC1213-MIB::udpLocalAddress.0.0.0.0.0 = IpAddress: 0.0.0.0
   RFC1213-MIB::udpLocalAddress.0.0.0.0.68 = IpAddress: 0.0.0.0
   RFC1213-MIB::udpLocalAddress.0.0.0.0.161 = IpAddress: 0.0.0.0
   RFC1213-MIB::udpLocalAddress.0.0.0.0.34964 = IpAddress: 0.0.0.0
   RFC1213-MIB::udpLocalAddress.0.0.0.0.49152 = IpAddress: 0.0.0.0
   RFC1213-MIB::udpLocalAddress.0.0.0.0.49153 = IpAddress: 0.0.0.0
   RFC1213-MIB::udpLocalAddress.127.0.0.1.12345 = IpAddress: 127.0.0.1
   RFC1213-MIB::udpLocalAddress.127.0.0.1.12346 = IpAddress: 127.0.0.1
   RFC1213-MIB::udpLocalPort.0.0.0.0.0 = INTEGER: 0
   RFC1213-MIB::udpLocalPort.0.0.0.0.68 = INTEGER: 68
   RFC1213-MIB::udpLocalPort.0.0.0.0.161 = INTEGER: 161
   RFC1213-MIB::udpLocalPort.0.0.0.0.34964 = INTEGER: 34964
   RFC1213-MIB::udpLocalPort.0.0.0.0.49152 = INTEGER: 49152
   RFC1213-MIB::udpLocalPort.0.0.0.0.49153 = INTEGER: 49153
   RFC1213-MIB::udpLocalPort.127.0.0.1.12345 = INTEGER: 12345
   RFC1213-MIB::udpLocalPort.127.0.0.1.12346 = INTEGER: 12346
   SNMPv2-MIB::snmpInPkts.0 = Counter32: 5616
   SNMPv2-MIB::snmpOutPkts.0 = Counter32: 5607
   SNMPv2-MIB::snmpInBadVersions.0 = Counter32: 0
   SNMPv2-MIB::snmpInBadCommunityNames.0 = Counter32: 9
   SNMPv2-MIB::snmpInBadCommunityUses.0 = Counter32: 0
   SNMPv2-MIB::snmpInASNParseErrs.0 = Counter32: 0
   SNMPv2-MIB::snmpInTooBigs.0 = Counter32: 0
   SNMPv2-MIB::snmpInNoSuchNames.0 = Counter32: 0
   SNMPv2-MIB::snmpInBadValues.0 = Counter32: 0
   SNMPv2-MIB::snmpInReadOnlys.0 = Counter32: 0
   SNMPv2-MIB::snmpInGenErrs.0 = Counter32: 0
   SNMPv2-MIB::snmpInTotalReqVars.0 = Counter32: 5616
   SNMPv2-MIB::snmpInTotalSetVars.0 = Counter32: 0
   SNMPv2-MIB::snmpInGetRequests.0 = Counter32: 5
   SNMPv2-MIB::snmpInGetNexts.0 = Counter32: 5615
   SNMPv2-MIB::snmpInSetRequests.0 = Counter32: 0
   SNMPv2-MIB::snmpInGetResponses.0 = Counter32: 0
   SNMPv2-MIB::snmpInTraps.0 = Counter32: 0
   SNMPv2-MIB::snmpOutTooBigs.0 = Counter32: 0
   SNMPv2-MIB::snmpOutNoSuchNames.0 = Counter32: 0
   SNMPv2-MIB::snmpOutBadValues.0 = Counter32: 0
   SNMPv2-MIB::snmpOutGenErrs.0 = Counter32: 0
   SNMPv2-MIB::snmpOutGetRequests.0 = Counter32: 0
   SNMPv2-MIB::snmpOutGetNexts.0 = Counter32: 0
   SNMPv2-MIB::snmpOutSetRequests.0 = Counter32: 0
   SNMPv2-MIB::snmpOutGetResponses.0 = Counter32: 5630
   SNMPv2-MIB::snmpOutTraps.0 = Counter32: 0
   SNMPv2-MIB::snmpEnableAuthenTraps.0 = INTEGER: disabled(2)
   SNMPv2-MIB::snmpSilentDrops.0 = Counter32: 0
   SNMPv2-MIB::snmpProxyDrops.0 = Counter32: 0
   RMON2-MIB::netDefaultGateway.0 = IpAddress: 0.0.0.0
   BRIDGE-MIB::dot1dBaseBridgeAddress.0 = STRING: 20:87:56:ff:aa:84
   BRIDGE-MIB::dot1dBaseNumPorts.0 = INTEGER: 4 ports
   BRIDGE-MIB::dot1dBaseType.0 = INTEGER: transparent-only(2)
   BRIDGE-MIB::dot1dBasePort.1 = INTEGER: 1
   BRIDGE-MIB::dot1dBasePort.2 = INTEGER: 2
   BRIDGE-MIB::dot1dBasePort.3 = INTEGER: 3
   BRIDGE-MIB::dot1dBasePort.4 = INTEGER: 4
   BRIDGE-MIB::dot1dBasePortIfIndex.1 = INTEGER: 1
   BRIDGE-MIB::dot1dBasePortIfIndex.2 = INTEGER: 2
   BRIDGE-MIB::dot1dBasePortIfIndex.3 = INTEGER: 3
   BRIDGE-MIB::dot1dBasePortIfIndex.4 = INTEGER: 4
   BRIDGE-MIB::dot1dBasePortCircuit.1 = OID: SNMPv2-SMI::zeroDotZero
   BRIDGE-MIB::dot1dBasePortCircuit.2 = OID: SNMPv2-SMI::zeroDotZero
   BRIDGE-MIB::dot1dBasePortCircuit.3 = OID: SNMPv2-SMI::zeroDotZero
   BRIDGE-MIB::dot1dBasePortCircuit.4 = OID: SNMPv2-SMI::zeroDotZero
   BRIDGE-MIB::dot1dBasePortDelayExceededDiscards.1 = Counter32: 0
   BRIDGE-MIB::dot1dBasePortDelayExceededDiscards.2 = Counter32: 0
   BRIDGE-MIB::dot1dBasePortDelayExceededDiscards.3 = Counter32: 0
   BRIDGE-MIB::dot1dBasePortDelayExceededDiscards.4 = Counter32: 0
   BRIDGE-MIB::dot1dBasePortMtuExceededDiscards.1 = Counter32: 0
   BRIDGE-MIB::dot1dBasePortMtuExceededDiscards.2 = Counter32: 0
   BRIDGE-MIB::dot1dBasePortMtuExceededDiscards.3 = Counter32: 0
   BRIDGE-MIB::dot1dBasePortMtuExceededDiscards.4 = Counter32: 0
   BRIDGE-MIB::dot1dTpLearnedEntryDiscards.0 = Counter32: 0
   BRIDGE-MIB::dot1dTpAgingTime.0 = INTEGER: 30 seconds
   BRIDGE-MIB::dot1dTpFdbAddress.'T.u...' = STRING: 54:ee:75:ff:95:a6
   BRIDGE-MIB::dot1dTpFdbAddress.'.'....' = STRING: b8:27:eb:a4:b5:ee
   BRIDGE-MIB::dot1dTpFdbPort.'T.u...' = INTEGER: 3
   BRIDGE-MIB::dot1dTpFdbPort.'.'....' = INTEGER: 2
   BRIDGE-MIB::dot1dTpFdbStatus.'T.u...' = INTEGER: learned(3)
   BRIDGE-MIB::dot1dTpFdbStatus.'.'....' = INTEGER: learned(3)
   BRIDGE-MIB::dot1dTpPort.1 = INTEGER: 1
   BRIDGE-MIB::dot1dTpPort.2 = INTEGER: 2
   BRIDGE-MIB::dot1dTpPort.3 = INTEGER: 3
   BRIDGE-MIB::dot1dTpPort.4 = INTEGER: 4
   BRIDGE-MIB::dot1dTpPortMaxInfo.1 = INTEGER: 1486 bytes
   BRIDGE-MIB::dot1dTpPortMaxInfo.2 = INTEGER: 1486 bytes
   BRIDGE-MIB::dot1dTpPortMaxInfo.3 = INTEGER: 1486 bytes
   BRIDGE-MIB::dot1dTpPortMaxInfo.4 = INTEGER: 1486 bytes
   BRIDGE-MIB::dot1dTpPortInFrames.1 = Counter32: 8751 frames
   BRIDGE-MIB::dot1dTpPortInFrames.2 = Counter32: 1830 frames
   BRIDGE-MIB::dot1dTpPortInFrames.3 = Counter32: 6546 frames
   BRIDGE-MIB::dot1dTpPortInFrames.4 = Counter32: 0 frames
   BRIDGE-MIB::dot1dTpPortOutFrames.1 = Counter32: 13306 frames
   BRIDGE-MIB::dot1dTpPortOutFrames.2 = Counter32: 5738 frames
   BRIDGE-MIB::dot1dTpPortOutFrames.3 = Counter32: 22510 frames
   BRIDGE-MIB::dot1dTpPortOutFrames.4 = Counter32: 0 frames
   BRIDGE-MIB::dot1dTpPortInDiscards.1 = Counter32: 0 frames
   BRIDGE-MIB::dot1dTpPortInDiscards.2 = Counter32: 0 frames
   BRIDGE-MIB::dot1dTpPortInDiscards.3 = Counter32: 0 frames
   BRIDGE-MIB::dot1dTpPortInDiscards.4 = Counter32: 0 frames
   IF-MIB::ifName.1 = STRING: Siemens, SIMATIC NET, Ethernet Port, X1 P1
   IF-MIB::ifName.2 = STRING: Siemens, SIMATIC NET, Ethernet Port, X1 P2
   IF-MIB::ifName.3 = STRING: Siemens, SIMATIC NET, Ethernet Port, X1 P3
   IF-MIB::ifName.4 = STRING: Siemens, SIMATIC NET, Ethernet Port, X1 P4
   IF-MIB::ifName.1000 = STRING: Siemens, SIMATIC NET, internal, X1
   IF-MIB::ifInMulticastPkts.1 = Counter32: 6744
   IF-MIB::ifInMulticastPkts.2 = Counter32: 1299
   IF-MIB::ifInMulticastPkts.3 = Counter32: 91
   IF-MIB::ifInMulticastPkts.4 = Counter32: 0
   IF-MIB::ifInMulticastPkts.1000 = Counter32: 0
   IF-MIB::ifInBroadcastPkts.1 = Counter32: 1096
   IF-MIB::ifInBroadcastPkts.2 = Counter32: 223
   IF-MIB::ifInBroadcastPkts.3 = Counter32: 8
   IF-MIB::ifInBroadcastPkts.4 = Counter32: 0
   IF-MIB::ifInBroadcastPkts.1000 = Counter32: 0
   IF-MIB::ifOutMulticastPkts.1 = Counter32: 7239
   IF-MIB::ifOutMulticastPkts.2 = Counter32: 5046
   IF-MIB::ifOutMulticastPkts.3 = Counter32: 14309
   IF-MIB::ifOutMulticastPkts.4 = Counter32: 0
   IF-MIB::ifOutMulticastPkts.1000 = Counter32: 0
   IF-MIB::ifOutBroadcastPkts.1 = Counter32: 232
   IF-MIB::ifOutBroadcastPkts.2 = Counter32: 307
   IF-MIB::ifOutBroadcastPkts.3 = Counter32: 1320
   IF-MIB::ifOutBroadcastPkts.4 = Counter32: 0
   IF-MIB::ifOutBroadcastPkts.1000 = Counter32: 0
   IF-MIB::ifHCInOctets.1 = Counter64: 908950
   IF-MIB::ifHCInOctets.2 = Counter64: 214979
   IF-MIB::ifHCInOctets.3 = Counter64: 605526
   IF-MIB::ifHCInOctets.4 = Counter64: 0
   IF-MIB::ifHCInOctets.1000 = Counter64: 0
   IF-MIB::ifHCInUcastPkts.1 = Counter64: 911
   IF-MIB::ifHCInUcastPkts.2 = Counter64: 308
   IF-MIB::ifHCInUcastPkts.3 = Counter64: 6501
   IF-MIB::ifHCInUcastPkts.4 = Counter64: 0
   IF-MIB::ifHCInUcastPkts.1000 = Counter64: 0
   IF-MIB::ifHCInMulticastPkts.1 = Counter64: 6744
   IF-MIB::ifHCInMulticastPkts.2 = Counter64: 1299
   IF-MIB::ifHCInMulticastPkts.3 = Counter64: 91
   IF-MIB::ifHCInMulticastPkts.4 = Counter64: 0
   IF-MIB::ifHCInMulticastPkts.1000 = Counter64: 0
   IF-MIB::ifHCInBroadcastPkts.1 = Counter64: 1096
   IF-MIB::ifHCInBroadcastPkts.2 = Counter64: 223
   IF-MIB::ifHCInBroadcastPkts.3 = Counter64: 8
   IF-MIB::ifHCInBroadcastPkts.4 = Counter64: 0
   IF-MIB::ifHCInBroadcastPkts.1000 = Counter64: 0
   IF-MIB::ifHCOutOctets.1 = Counter64: 1516677
   IF-MIB::ifHCOutOctets.2 = Counter64: 597260
   IF-MIB::ifHCOutOctets.3 = Counter64: 2274335
   IF-MIB::ifHCOutOctets.4 = Counter64: 0
   IF-MIB::ifHCOutOctets.1000 = Counter64: 0
   IF-MIB::ifHCOutUcastPkts.1 = Counter64: 5835
   IF-MIB::ifHCOutUcastPkts.2 = Counter64: 385
   IF-MIB::ifHCOutUcastPkts.3 = Counter64: 6955
   IF-MIB::ifHCOutUcastPkts.4 = Counter64: 0
   IF-MIB::ifHCOutUcastPkts.1000 = Counter64: 0
   IF-MIB::ifHCOutMulticastPkts.1 = Counter64: 7239
   IF-MIB::ifHCOutMulticastPkts.2 = Counter64: 5046
   IF-MIB::ifHCOutMulticastPkts.3 = Counter64: 14309
   IF-MIB::ifHCOutMulticastPkts.4 = Counter64: 0
   IF-MIB::ifHCOutMulticastPkts.1000 = Counter64: 0
   IF-MIB::ifHCOutBroadcastPkts.1 = Counter64: 232
   IF-MIB::ifHCOutBroadcastPkts.2 = Counter64: 307
   IF-MIB::ifHCOutBroadcastPkts.3 = Counter64: 1320
   IF-MIB::ifHCOutBroadcastPkts.4 = Counter64: 0
   IF-MIB::ifHCOutBroadcastPkts.1000 = Counter64: 0
   IF-MIB::ifLinkUpDownTrapEnable.1 = INTEGER: disabled(2)
   IF-MIB::ifLinkUpDownTrapEnable.2 = INTEGER: disabled(2)
   IF-MIB::ifLinkUpDownTrapEnable.3 = INTEGER: disabled(2)
   IF-MIB::ifLinkUpDownTrapEnable.4 = INTEGER: disabled(2)
   IF-MIB::ifLinkUpDownTrapEnable.1000 = INTEGER: disabled(2)
   IF-MIB::ifHighSpeed.1 = Gauge32: 10000000
   IF-MIB::ifHighSpeed.2 = Gauge32: 100000000
   IF-MIB::ifHighSpeed.3 = Gauge32: 100000000
   IF-MIB::ifHighSpeed.4 = Gauge32: 10000000
   IF-MIB::ifHighSpeed.1000 = Gauge32: 0
   IF-MIB::ifPromiscuousMode.1 = INTEGER: false(2)
   IF-MIB::ifPromiscuousMode.2 = INTEGER: false(2)
   IF-MIB::ifPromiscuousMode.3 = INTEGER: false(2)
   IF-MIB::ifPromiscuousMode.4 = INTEGER: false(2)
   IF-MIB::ifPromiscuousMode.1000 = INTEGER: false(2)
   IF-MIB::ifConnectorPresent.1 = INTEGER: true(1)
   IF-MIB::ifConnectorPresent.2 = INTEGER: true(1)
   IF-MIB::ifConnectorPresent.3 = INTEGER: true(1)
   IF-MIB::ifConnectorPresent.4 = INTEGER: true(1)
   IF-MIB::ifConnectorPresent.1000 = INTEGER: false(2)
   IF-MIB::ifAlias.1 = STRING:
   IF-MIB::ifAlias.2 = STRING:
   IF-MIB::ifAlias.3 = STRING:
   IF-MIB::ifAlias.4 = STRING:
   IF-MIB::ifAlias.1000 = STRING:
   IF-MIB::ifCounterDiscontinuityTime.1 = Timeticks: (0) 0:00:00.00
   IF-MIB::ifCounterDiscontinuityTime.2 = Timeticks: (0) 0:00:00.00
   IF-MIB::ifCounterDiscontinuityTime.3 = Timeticks: (0) 0:00:00.00
   IF-MIB::ifCounterDiscontinuityTime.4 = Timeticks: (0) 0:00:00.00
   IF-MIB::ifCounterDiscontinuityTime.1000 = Timeticks: (0) 0:00:00.00
   IF-MIB::ifStackStatus.0.1000 = INTEGER: active(1)
   IF-MIB::ifStackStatus.1.0 = INTEGER: active(1)
   IF-MIB::ifStackStatus.2.0 = INTEGER: active(1)
   IF-MIB::ifStackStatus.3.0 = INTEGER: active(1)
   IF-MIB::ifStackStatus.4.0 = INTEGER: active(1)
   IF-MIB::ifStackStatus.1000.1 = INTEGER: active(1)
   IF-MIB::ifStackStatus.1000.2 = INTEGER: active(1)
   IF-MIB::ifStackStatus.1000.3 = INTEGER: active(1)
   IF-MIB::ifStackStatus.1000.4 = INTEGER: active(1)
   IF-MIB::ifRcvAddressStatus.1000 = INTEGER: active(1)
   IF-MIB::ifRcvAddressType.1000 = INTEGER: nonVolatile(3)
   IF-MIB::ifTableLastChange.0 = Timeticks: (0) 0:00:00.00
   IF-MIB::ifStackLastChange.0 = Timeticks: (0) 0:00:00.00

and the LLDP subtree::

   $ snmpwalk -v2c -c public -m ALL 192.168.0.99 LLDP-MIB::lldpMIB
   LLDP-MIB::lldpMessageTxInterval.0 = INTEGER: 5 seconds
   LLDP-MIB::lldpMessageTxHoldMultiplier.0 = INTEGER: 4
   LLDP-MIB::lldpReinitDelay.0 = INTEGER: 1 seconds
   LLDP-MIB::lldpTxDelay.0 = INTEGER: 1 seconds
   LLDP-MIB::lldpPortConfigAdminStatus.1 = INTEGER: txAndRx(3)
   LLDP-MIB::lldpPortConfigAdminStatus.2 = INTEGER: txAndRx(3)
   LLDP-MIB::lldpPortConfigAdminStatus.3 = INTEGER: txAndRx(3)
   LLDP-MIB::lldpPortConfigAdminStatus.4 = INTEGER: txAndRx(3)
   LLDP-MIB::lldpPortConfigNotificationEnable.1 = INTEGER: false(2)
   LLDP-MIB::lldpPortConfigNotificationEnable.2 = INTEGER: false(2)
   LLDP-MIB::lldpPortConfigNotificationEnable.3 = INTEGER: false(2)
   LLDP-MIB::lldpPortConfigNotificationEnable.4 = INTEGER: false(2)
   LLDP-MIB::lldpPortConfigTLVsTxEnable.1 = BITS: F0 portDesc(0) sysName(1) sysDesc(2) sysCap(3)
   LLDP-MIB::lldpPortConfigTLVsTxEnable.2 = BITS: F0 portDesc(0) sysName(1) sysDesc(2) sysCap(3)
   LLDP-MIB::lldpPortConfigTLVsTxEnable.3 = BITS: F0 portDesc(0) sysName(1) sysDesc(2) sysCap(3)
   LLDP-MIB::lldpPortConfigTLVsTxEnable.4 = BITS: F0 portDesc(0) sysName(1) sysDesc(2) sysCap(3)
   LLDP-MIB::lldpConfigManAddrPortsTxEnable.ipV4."...c" = Hex-STRING: F0
   LLDP-MIB::lldpStatsRemTablesLastChangeTime.0 = Timeticks: (1200) 0:00:12.00
   LLDP-MIB::lldpStatsRemTablesInserts.0 = Gauge32: 2 table entries
   LLDP-MIB::lldpStatsRemTablesDeletes.0 = Gauge32: 0 table entries
   LLDP-MIB::lldpStatsRemTablesDrops.0 = Gauge32: 0 table entries
   LLDP-MIB::lldpStatsRemTablesAgeouts.0 = Gauge32: 0
   LLDP-MIB::lldpStatsTxPortFramesTotal.1 = Counter32: 1072
   LLDP-MIB::lldpStatsTxPortFramesTotal.2 = Counter32: 1072
   LLDP-MIB::lldpStatsTxPortFramesTotal.3 = Counter32: 1072
   LLDP-MIB::lldpStatsTxPortFramesTotal.4 = Counter32: 0
   LLDP-MIB::lldpStatsRxPortFramesDiscardedTotal.1 = Counter32: 0
   LLDP-MIB::lldpStatsRxPortFramesDiscardedTotal.2 = Counter32: 0
   LLDP-MIB::lldpStatsRxPortFramesDiscardedTotal.3 = Counter32: 0
   LLDP-MIB::lldpStatsRxPortFramesDiscardedTotal.4 = Counter32: 0
   LLDP-MIB::lldpStatsRxPortFramesErrors.1 = Counter32: 0
   LLDP-MIB::lldpStatsRxPortFramesErrors.2 = Counter32: 0
   LLDP-MIB::lldpStatsRxPortFramesErrors.3 = Counter32: 0
   LLDP-MIB::lldpStatsRxPortFramesErrors.4 = Counter32: 0
   LLDP-MIB::lldpStatsRxPortFramesTotal.1 = Counter32: 669
   LLDP-MIB::lldpStatsRxPortFramesTotal.2 = Counter32: 1073
   LLDP-MIB::lldpStatsRxPortFramesTotal.3 = Counter32: 0
   LLDP-MIB::lldpStatsRxPortFramesTotal.4 = Counter32: 0
   LLDP-MIB::lldpStatsRxPortTLVsDiscardedTotal.1 = Counter32: 0
   LLDP-MIB::lldpStatsRxPortTLVsDiscardedTotal.2 = Counter32: 0
   LLDP-MIB::lldpStatsRxPortTLVsDiscardedTotal.3 = Counter32: 0
   LLDP-MIB::lldpStatsRxPortTLVsDiscardedTotal.4 = Counter32: 0
   LLDP-MIB::lldpStatsRxPortTLVsUnrecognizedTotal.1 = Counter32: 0
   LLDP-MIB::lldpStatsRxPortTLVsUnrecognizedTotal.2 = Counter32: 0
   LLDP-MIB::lldpStatsRxPortTLVsUnrecognizedTotal.3 = Counter32: 0
   LLDP-MIB::lldpStatsRxPortTLVsUnrecognizedTotal.4 = Counter32: 0
   LLDP-MIB::lldpStatsRxPortAgeoutsTotal.1 = Gauge32: 0
   LLDP-MIB::lldpStatsRxPortAgeoutsTotal.2 = Gauge32: 0
   LLDP-MIB::lldpStatsRxPortAgeoutsTotal.3 = Gauge32: 0
   LLDP-MIB::lldpStatsRxPortAgeoutsTotal.4 = Gauge32: 0
   LLDP-MIB::lldpLocChassisIdSubtype.0 = INTEGER: local(7)
   LLDP-MIB::lldpLocChassisId.0 = STRING: "b"
   LLDP-MIB::lldpLocSysName.0 = STRING: sysName Not Set
   LLDP-MIB::lldpLocSysDesc.0 = STRING: Siemens, SIMATIC NET, SCALANCE X204IRT, 6GK5 204-0BA00-2BA3, HW: Version 9, FW: Version V05.04.02, SVPL6147920
   LLDP-MIB::lldpLocSysCapSupported.0 = BITS: 01 stationOnly(7)
   LLDP-MIB::lldpLocSysCapEnabled.0 = BITS: 01 stationOnly(7)
   LLDP-MIB::lldpLocPortIdSubtype.1 = INTEGER: local(7)
   LLDP-MIB::lldpLocPortIdSubtype.2 = INTEGER: local(7)
   LLDP-MIB::lldpLocPortIdSubtype.3 = INTEGER: local(7)
   LLDP-MIB::lldpLocPortIdSubtype.4 = INTEGER: local(7)
   LLDP-MIB::lldpLocPortId.1 = STRING: "port-001"
   LLDP-MIB::lldpLocPortId.2 = STRING: "port-002"
   LLDP-MIB::lldpLocPortId.3 = STRING: "port-003"
   LLDP-MIB::lldpLocPortId.4 = STRING: "port-004"
   LLDP-MIB::lldpLocPortDesc.1 = STRING: Siemens, SIMATIC NET, Ethernet Port, X1 P1
   LLDP-MIB::lldpLocPortDesc.2 = STRING: Siemens, SIMATIC NET, Ethernet Port, X1 P2
   LLDP-MIB::lldpLocPortDesc.3 = STRING: Siemens, SIMATIC NET, Ethernet Port, X1 P3
   LLDP-MIB::lldpLocPortDesc.4 = STRING: Siemens, SIMATIC NET, Ethernet Port, X1 P4
   LLDP-MIB::lldpLocManAddrLen.ipV4."...c" = INTEGER: 5
   LLDP-MIB::lldpLocManAddrIfSubtype.ipV4."...c" = INTEGER: ifIndex(2)
   LLDP-MIB::lldpLocManAddrIfId.ipV4."...c" = INTEGER: 1000
   LLDP-MIB::lldpLocManAddrOID.ipV4."...c" = OID: SNMPv2-SMI::enterprises.24686
   LLDP-MIB::lldpRemChassisIdSubtype.400.2.2 = INTEGER: local(7)
   LLDP-MIB::lldpRemChassisIdSubtype.1200.1.1 = INTEGER: local(7)
   LLDP-MIB::lldpRemChassisId.400.2.2 = STRING: "3S PN-Controller          1103000032           0000000010a4b5ee     0 V  1  0  0"
   LLDP-MIB::lldpRemChassisId.1200.1.1 = STRING: "(REMOVED)"
   LLDP-MIB::lldpRemPortIdSubtype.400.2.2 = INTEGER: local(7)
   LLDP-MIB::lldpRemPortIdSubtype.1200.1.1 = INTEGER: local(7)
   LLDP-MIB::lldpRemPortId.400.2.2 = STRING: "port-001.controller"
   LLDP-MIB::lldpRemPortId.1200.1.1 = STRING: "port-001"
   LLDP-MIB::lldpRemSysName.1200.1.1 = STRING: (REMOVED)
   LLDP-MIB::lldpRemSysDesc.1200.1.1 = STRING: (REMOVED)
   LLDP-MIB::lldpRemSysCapSupported.1200.1.1 = BITS: 01 stationOnly(7)
   LLDP-MIB::lldpRemSysCapEnabled.1200.1.1 = BITS: 01 stationOnly(7)
   LLDP-MIB::lldpRemManAddrIfSubtype.400.2.2.ipV4."...d" = INTEGER: systemPortNumber(3)
   LLDP-MIB::lldpRemManAddrIfSubtype.1200.1.1.ipV4."...." = INTEGER: ifIndex(2)
   LLDP-MIB::lldpRemManAddrIfId.400.2.2.ipV4."...d" = INTEGER: 1
   LLDP-MIB::lldpRemManAddrIfId.1200.1.1.ipV4."...." = INTEGER: 1
   LLDP-MIB::lldpRemManAddrOID.1200.1.1.ipV4."...." = OID: SNMPv2-SMI::enterprises.24686
   LLDP-EXT-PNO-MIB::lldpXPnoConfigSPDTxEnable.1 = INTEGER: true(1)
   LLDP-EXT-PNO-MIB::lldpXPnoConfigSPDTxEnable.2 = INTEGER: true(1)
   LLDP-EXT-PNO-MIB::lldpXPnoConfigSPDTxEnable.3 = INTEGER: true(1)
   LLDP-EXT-PNO-MIB::lldpXPnoConfigSPDTxEnable.4 = INTEGER: true(1)
   LLDP-EXT-PNO-MIB::lldpXPnoConfigPortStatusTxEnable.1 = INTEGER: true(1)
   LLDP-EXT-PNO-MIB::lldpXPnoConfigPortStatusTxEnable.2 = INTEGER: true(1)
   LLDP-EXT-PNO-MIB::lldpXPnoConfigPortStatusTxEnable.3 = INTEGER: true(1)
   LLDP-EXT-PNO-MIB::lldpXPnoConfigPortStatusTxEnable.4 = INTEGER: true(1)
   LLDP-EXT-PNO-MIB::lldpXPnoConfigAliasTxEnable.1 = INTEGER: true(1)
   LLDP-EXT-PNO-MIB::lldpXPnoConfigAliasTxEnable.2 = INTEGER: true(1)
   LLDP-EXT-PNO-MIB::lldpXPnoConfigAliasTxEnable.3 = INTEGER: true(1)
   LLDP-EXT-PNO-MIB::lldpXPnoConfigAliasTxEnable.4 = INTEGER: true(1)
   LLDP-EXT-PNO-MIB::lldpXPnoConfigMrpTxEnable.1 = INTEGER: true(1)
   LLDP-EXT-PNO-MIB::lldpXPnoConfigMrpTxEnable.2 = INTEGER: true(1)
   LLDP-EXT-PNO-MIB::lldpXPnoConfigMrpTxEnable.3 = INTEGER: true(1)
   LLDP-EXT-PNO-MIB::lldpXPnoConfigMrpTxEnable.4 = INTEGER: true(1)
   LLDP-EXT-PNO-MIB::lldpXPnoConfigPtcpTxEnable.1 = INTEGER: true(1)
   LLDP-EXT-PNO-MIB::lldpXPnoConfigPtcpTxEnable.2 = INTEGER: true(1)
   LLDP-EXT-PNO-MIB::lldpXPnoConfigPtcpTxEnable.3 = INTEGER: true(1)
   LLDP-EXT-PNO-MIB::lldpXPnoConfigPtcpTxEnable.4 = INTEGER: true(1)
   LLDP-EXT-PNO-MIB::lldpXPnoLocLPDValue.1 = Gauge32: 0 ns
   LLDP-EXT-PNO-MIB::lldpXPnoLocLPDValue.2 = Gauge32: 0 ns
   LLDP-EXT-PNO-MIB::lldpXPnoLocLPDValue.3 = Gauge32: 0 ns
   LLDP-EXT-PNO-MIB::lldpXPnoLocLPDValue.4 = Gauge32: 0 ns
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortTxDValue.1 = Gauge32: 1123 ns
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortTxDValue.2 = Gauge32: 1123 ns
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortTxDValue.3 = Gauge32: 1123 ns
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortTxDValue.4 = Gauge32: 0 ns
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortRxDValue.1 = Gauge32: 444 ns
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortRxDValue.2 = Gauge32: 444 ns
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortRxDValue.3 = Gauge32: 444 ns
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortRxDValue.4 = Gauge32: 0 ns
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT2.1 = INTEGER: off(0)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT2.2 = INTEGER: off(0)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT2.3 = INTEGER: off(0)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT2.4 = INTEGER: off(0)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT3.1 = INTEGER: off(0)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT3.2 = INTEGER: off(0)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT3.3 = INTEGER: off(0)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT3.4 = INTEGER: off(0)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortNoS.1 = STRING: b
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortNoS.2 = STRING: b
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortNoS.3 = STRING: b
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortNoS.4 = STRING: b
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortMrpUuId.1 = Hex-STRING: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortMrpUuId.2 = Hex-STRING: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortMrpUuId.3 = Hex-STRING: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortMrpUuId.4 = Hex-STRING: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortMrrtStatus.1 = INTEGER: off(0)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortMrrtStatus.2 = INTEGER: off(0)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortMrrtStatus.3 = INTEGER: off(0)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortMrrtStatus.4 = INTEGER: off(0)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortPtcpMaster.1 = STRING: 0:0:0:0:0:0
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortPtcpMaster.2 = STRING: 0:0:0:0:0:0
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortPtcpMaster.3 = STRING: 0:0:0:0:0:0
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortPtcpMaster.4 = STRING: 0:0:0:0:0:0
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortPtcpSubdomainUUID.1 = Hex-STRING: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortPtcpSubdomainUUID.2 = Hex-STRING: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortPtcpSubdomainUUID.3 = Hex-STRING: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortPtcpSubdomainUUID.4 = Hex-STRING: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortPtcpIRDataUUID.1 = Hex-STRING: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortPtcpIRDataUUID.2 = Hex-STRING: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortPtcpIRDataUUID.3 = Hex-STRING: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortPtcpIRDataUUID.4 = Hex-STRING: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortModeRT3.1 = INTEGER: standard(1)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortModeRT3.2 = INTEGER: standard(1)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortModeRT3.3 = INTEGER: standard(1)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortModeRT3.4 = INTEGER: standard(1)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortPeriodLength.1 = Wrong Type (should be Gauge32 or Unsigned32): INTEGER: 1000000
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortPeriodLength.2 = Wrong Type (should be Gauge32 or Unsigned32): INTEGER: 1000000
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortPeriodLength.3 = Wrong Type (should be Gauge32 or Unsigned32): INTEGER: 1000000
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortPeriodLength.4 = Wrong Type (should be Gauge32 or Unsigned32): INTEGER: 1000000
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortPeriodValidity.1 = INTEGER: true(1)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortPeriodValidity.2 = INTEGER: true(1)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortPeriodValidity.3 = INTEGER: true(1)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortPeriodValidity.4 = INTEGER: true(1)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortRedOffset.1 = Wrong Type (should be Gauge32 or Unsigned32): INTEGER: 0
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortRedOffset.2 = Wrong Type (should be Gauge32 or Unsigned32): INTEGER: 0
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortRedOffset.3 = Wrong Type (should be Gauge32 or Unsigned32): INTEGER: 0
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortRedOffset.4 = Wrong Type (should be Gauge32 or Unsigned32): INTEGER: 0
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortRedValidity.1 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortRedValidity.2 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortRedValidity.3 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortRedValidity.4 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortOrangeOffset.1 = Wrong Type (should be Gauge32 or Unsigned32): INTEGER: 0
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortOrangeOffset.2 = Wrong Type (should be Gauge32 or Unsigned32): INTEGER: 0
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortOrangeOffset.3 = Wrong Type (should be Gauge32 or Unsigned32): INTEGER: 0
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortOrangeOffset.4 = Wrong Type (should be Gauge32 or Unsigned32): INTEGER: 0
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortOrangeValidity.1 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortOrangeValidity.2 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortOrangeValidity.3 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortOrangeValidity.4 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortGreenOffset.1 = Wrong Type (should be Gauge32 or Unsigned32): INTEGER: 0
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortGreenOffset.2 = Wrong Type (should be Gauge32 or Unsigned32): INTEGER: 0
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortGreenOffset.3 = Wrong Type (should be Gauge32 or Unsigned32): INTEGER: 0
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortGreenOffset.4 = Wrong Type (should be Gauge32 or Unsigned32): INTEGER: 0
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortGreenValidity.1 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortGreenValidity.2 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortGreenValidity.3 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortGreenValidity.4 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT3OptimizationSupported.1 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT3OptimizationSupported.2 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT3OptimizationSupported.3 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT3OptimizationSupported.4 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT3PreambleShorteningSupported.1 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT3PreambleShorteningSupported.2 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT3PreambleShorteningSupported.3 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT3PreambleShorteningSupported.4 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT3PreambleShortening.1 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT3PreambleShortening.2 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT3PreambleShortening.3 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT3PreambleShortening.4 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT3FragmentationSupported.1 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT3FragmentationSupported.2 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT3FragmentationSupported.3 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT3FragmentationSupported.4 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT3Fragmentation.1 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT3Fragmentation.2 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT3Fragmentation.3 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoLocPortStatusRT3Fragmentation.4 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoRemLPDValue.400.2.2 = Gauge32: 0 ns
   LLDP-EXT-PNO-MIB::lldpXPnoRemLPDValue.1200.1.1 = Gauge32: 0 ns
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortTxDValue.400.2.2 = Gauge32: 0 ns
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortTxDValue.1200.1.1 = Gauge32: 0 ns
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortRxDValue.400.2.2 = Gauge32: 0 ns
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortRxDValue.1200.1.1 = Gauge32: 0 ns
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortStatusRT2.400.2.2 = INTEGER: off(0)
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortStatusRT2.1200.1.1 = INTEGER: off(0)
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortStatusRT3.400.2.2 = INTEGER: off(0)
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortStatusRT3.1200.1.1 = INTEGER: off(0)
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortNoS.400.2.2 = STRING: controller
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortNoS.1200.1.1 = STRING: (REMOVED)
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortMrpUuId.400.2.2 = Hex-STRING: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortMrpUuId.1200.1.1 = Hex-STRING: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortMrrtStatus.400.2.2 = INTEGER: off(0)
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortMrrtStatus.1200.1.1 = INTEGER: off(0)
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortPtcpMaster.400.2.2 = STRING: 0:0:0:0:0:0
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortPtcpMaster.1200.1.1 = STRING: 0:0:0:0:0:0
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortPtcpSubdomainUUID.400.2.2 = Hex-STRING: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortPtcpSubdomainUUID.1200.1.1 = Hex-STRING: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortPtcpIRDataUUID.400.2.2 = Hex-STRING: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortPtcpIRDataUUID.1200.1.1 = Hex-STRING: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortModeRT3.400.2.2 = INTEGER: standard(1)
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortModeRT3.1200.1.1 = INTEGER: standard(1)
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortPeriodLength.400.2.2 = Wrong Type (should be Gauge32 or Unsigned32): INTEGER: 0
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortPeriodLength.1200.1.1 = Wrong Type (should be Gauge32 or Unsigned32): INTEGER: 0
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortPeriodValidity.400.2.2 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortPeriodValidity.1200.1.1 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortRedOffset.400.2.2 = Wrong Type (should be Gauge32 or Unsigned32): INTEGER: 0
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortRedOffset.1200.1.1 = Wrong Type (should be Gauge32 or Unsigned32): INTEGER: 0
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortRedValidity.400.2.2 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortRedValidity.1200.1.1 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortOrangeOffset.400.2.2 = Wrong Type (should be Gauge32 or Unsigned32): INTEGER: 0
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortOrangeOffset.1200.1.1 = Wrong Type (should be Gauge32 or Unsigned32): INTEGER: 0
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortOrangeValidity.400.2.2 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortOrangeValidity.1200.1.1 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortGreenOffset.400.2.2 = Wrong Type (should be Gauge32 or Unsigned32): INTEGER: 0
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortGreenOffset.1200.1.1 = Wrong Type (should be Gauge32 or Unsigned32): INTEGER: 0
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortGreenValidity.400.2.2 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortGreenValidity.1200.1.1 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortStatusRT3PreambleShortening.400.2.2 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortStatusRT3PreambleShortening.1200.1.1 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortStatusRT3Fragmentation.400.2.2 = INTEGER: false(2)
   LLDP-EXT-PNO-MIB::lldpXPnoRemPortStatusRT3Fragmentation.1200.1.1 = INTEGER: false(2)
   LLDP-MIB::lldpExtensions.4623.1.1.1.1.1.1 = Hex-STRING: F0
   LLDP-MIB::lldpExtensions.4623.1.1.1.1.1.2 = Hex-STRING: F0
   LLDP-MIB::lldpExtensions.4623.1.1.1.1.1.3 = Hex-STRING: F0
   LLDP-MIB::lldpExtensions.4623.1.1.1.1.1.4 = Hex-STRING: F0
   LLDP-MIB::lldpExtensions.4623.1.2.1.1.1.1 = INTEGER: 1
   LLDP-MIB::lldpExtensions.4623.1.2.1.1.1.2 = INTEGER: 1
   LLDP-MIB::lldpExtensions.4623.1.2.1.1.1.3 = INTEGER: 1
   LLDP-MIB::lldpExtensions.4623.1.2.1.1.1.4 = INTEGER: 1
   LLDP-MIB::lldpExtensions.4623.1.2.1.1.2.1 = INTEGER: 1
   LLDP-MIB::lldpExtensions.4623.1.2.1.1.2.2 = INTEGER: 1
   LLDP-MIB::lldpExtensions.4623.1.2.1.1.2.3 = INTEGER: 1
   LLDP-MIB::lldpExtensions.4623.1.2.1.1.2.4 = INTEGER: 1
   LLDP-MIB::lldpExtensions.4623.1.2.1.1.3.1 = Hex-STRING: FF 00
   LLDP-MIB::lldpExtensions.4623.1.2.1.1.3.2 = Hex-STRING: FF 00
   LLDP-MIB::lldpExtensions.4623.1.2.1.1.3.3 = Hex-STRING: FF 00
   LLDP-MIB::lldpExtensions.4623.1.2.1.1.3.4 = Hex-STRING: FF 00
   LLDP-MIB::lldpExtensions.4623.1.2.1.1.4.1 = INTEGER: 16
   LLDP-MIB::lldpExtensions.4623.1.2.1.1.4.2 = INTEGER: 16
   LLDP-MIB::lldpExtensions.4623.1.2.1.1.4.3 = INTEGER: 16
   LLDP-MIB::lldpExtensions.4623.1.2.1.1.4.4 = INTEGER: 0
   LLDP-MIB::lldpExtensions.4623.1.2.3.1.1.1 = Hex-STRING: 00
   LLDP-MIB::lldpExtensions.4623.1.2.3.1.1.2 = Hex-STRING: 00
   LLDP-MIB::lldpExtensions.4623.1.2.3.1.1.3 = Hex-STRING: 00
   LLDP-MIB::lldpExtensions.4623.1.2.3.1.1.4 = Hex-STRING: 00
   LLDP-MIB::lldpExtensions.4623.1.2.3.1.2.1 = INTEGER: 0
   LLDP-MIB::lldpExtensions.4623.1.2.3.1.2.2 = INTEGER: 0
   LLDP-MIB::lldpExtensions.4623.1.2.3.1.2.3 = INTEGER: 0
   LLDP-MIB::lldpExtensions.4623.1.2.3.1.2.4 = INTEGER: 0
   LLDP-MIB::lldpExtensions.4623.1.2.4.1.1.1 = INTEGER: 1522
   LLDP-MIB::lldpExtensions.4623.1.2.4.1.1.2 = INTEGER: 1522
   LLDP-MIB::lldpExtensions.4623.1.2.4.1.1.3 = INTEGER: 1522
   LLDP-MIB::lldpExtensions.4623.1.2.4.1.1.4 = INTEGER: 1522
   LLDP-MIB::lldpExtensions.4623.1.3.1.1.1.400.2.2 = INTEGER: 1
   LLDP-MIB::lldpExtensions.4623.1.3.1.1.1.1200.1.1 = INTEGER: 1
   LLDP-MIB::lldpExtensions.4623.1.3.1.1.2.400.2.2 = INTEGER: 1
   LLDP-MIB::lldpExtensions.4623.1.3.1.1.2.1200.1.1 = INTEGER: 1
   LLDP-MIB::lldpExtensions.4623.1.3.1.1.3.400.2.2 = Hex-STRING: 6C 00
   LLDP-MIB::lldpExtensions.4623.1.3.1.1.3.1200.1.1 = Hex-STRING: EC 03
   LLDP-MIB::lldpExtensions.4623.1.3.1.1.4.400.2.2 = INTEGER: 16
   LLDP-MIB::lldpExtensions.4623.1.3.1.1.4.1200.1.1 = INTEGER: 30
   LLDP-MIB::lldpExtensions.32962.1.1.1.1.1.1 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.1.1.1.1.2 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.1.1.1.1.3 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.1.1.1.1.4 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.1.2.1.1.1.0 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.1.2.1.1.2.0 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.1.2.1.1.3.0 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.1.2.1.1.4.0 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.1.3.1.1.1.0 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.1.3.1.1.2.0 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.1.3.1.1.3.0 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.1.3.1.1.4.0 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.1.4.1.1.1.1 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.1.4.1.1.2.1 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.1.4.1.1.3.1 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.1.4.1.1.4.1 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.2.1.1.1.1 = INTEGER: 0
   LLDP-MIB::lldpExtensions.32962.1.2.1.1.1.2 = INTEGER: 0
   LLDP-MIB::lldpExtensions.32962.1.2.1.1.1.3 = INTEGER: 0
   LLDP-MIB::lldpExtensions.32962.1.2.1.1.1.4 = INTEGER: 0
   LLDP-MIB::lldpExtensions.32962.1.2.2.1.2.1.0 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.2.2.1.2.2.0 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.2.2.1.2.3.0 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.2.2.1.2.4.0 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.2.2.1.3.1.0 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.2.2.1.3.2.0 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.2.2.1.3.3.0 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.2.2.1.3.4.0 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.3.1.1.1.400.2.2 = INTEGER: 0
   LLDP-MIB::lldpExtensions.32962.1.3.1.1.1.1200.1.1 = INTEGER: 0
   LLDP-MIB::lldpExtensions.32962.1.3.2.1.2.400.2.2.0 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.3.2.1.2.1200.1.1.0 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.3.2.1.3.400.2.2.0 = INTEGER: 2
   LLDP-MIB::lldpExtensions.32962.1.3.2.1.3.1200.1.1.0 = INTEGER: 2
