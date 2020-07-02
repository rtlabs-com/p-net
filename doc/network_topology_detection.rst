Network topology detection
==========================


LLDP
----
Link Layer Discovery Protocol (LLDP) is used to find information on the closest
neighbours on an Ethernet network. Each IO-device, IO-controller and (managed)
switch send out frames containing its name and the portname, for each of their
Ethernet ports.

A LLDP frame is typically sent once per 5 seconds on each Ethernet interface.

Managed switches filter LLDP frames, and send its own LLDP frames.
That way everyone can know what is their closest neighbour. Only LLDP frames from
the closest neighbour is received on each port, thanks to the filtering done by
the switch.


SNMP
----
It is possible to ask an IO-Controller or IO-device (conformance class B or
higher) about its neighbours, using the Simple Network Management Protocol (SNMP).
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
