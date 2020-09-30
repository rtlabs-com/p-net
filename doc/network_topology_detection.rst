Network topology detection
==========================


LLDP
----
Link Layer Discovery Protocol (LLDP) is used to find information on the closest
neighbours on an Ethernet network. Each IO-device, IO-controller and (managed)
switch send out frames containing its name and the portname, for each of their
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

Some of the information can also be queried via the Profinet DCE/RPC.


Network topology tools
----------------------

libreNMS:
https://www.librenms.org/

Icinga:
https://icinga.com

Zabbix:
https://www.zabbix.com/

nmap:
https://nmap.org/

Net-SNMP:
http://www.net-snmp.org/



Monitor incoming LLDP frames on Linux
-------------------------------------

Install the lldpd daemon::

   $ sudo apt install lldpd

It will start to collect information automatically, but also to send LLDP packets
every 30 seconds.

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

   sudo service lldpd stop
