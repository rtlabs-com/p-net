Network topology detection
==========================

All Profinet units send LLDP packets for network topology discovery. This information
is collected in switches (and other units), and can be queried using the SNMP protocol.

A LLDP frame is sent once per 5 seconds on each Ethernet interface.


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
