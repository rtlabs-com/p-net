Multiple ports
==============
This section describes how to configure the p-net stack, sample application
and system for multiple network interfaces or ports.
So far, multiple port has only been tested using the Linux port.


Terminology
-----------

Interface
    Abstract group of ports. In Profinet context, interface typically doesn't mean a
    specific network interface. This is a common cause of confusion.
Port
    Network interface. The physical connectors are referred to as "physical ports".
    A "management port" is the network interface to which a controller / PLC connects.

In the example described in this section, ``br0`` is the management port
and ``eth0`` and ``eth1`` are the physical ports. The interface consists of
``br0``, ``eth0`` and ``eth1``.


Configuration of bridge using Linux
-----------------------------------
Tested with Raspberry PI 3B+ and USB Ethernet dongle USB3GIG.

            +-------------+
            |    br0      |
            +------+------+
            | eth0 | eth1 |
            +------+------+


p-net supports multiple Ethernet ports. To use multiple ports, these
shall be grouped into a bridge. In such a configuration the management port / main network interface
is the bridge and the Ethernet interfaces are named physical ports.

To create a bridge and add network interfaces to it, create the following files and content::

    # /etc/systemd/network/bridge-br0.netdev
    [NetDev]
    Name=br0
    Kind=bridge

    # /etc/systemd/network/bridge-br0.network
    [Match]
    Name=br0

    # /etc/systemd/network/br0-member-eth0.network
    [Match]
    Name=eth0
    [Network]
    Bridge=br0

    # /etc/systemd/network/br0-member-eth1.network
    [Match]
    Name=eth1
    [Network]
    Bridge=br0

Enable and start network daemon::

    sudo systemctl enable systemd-networkd
    sudo systemctl start systemd-networkd

Run ``ifconfig -a`` to check that the bridge is up and its network interfaces are all up::

   pi@pndevice-pi:~$ ifconfig -a
   br0: flags=4419<UP,BROADCAST,RUNNING,PROMISC,MULTICAST>  mtu 1500
         inet 192.168.0.50  netmask 255.255.255.0  broadcast 0.0.0.0
         ether 3e:9b:37:c6:3e:ee  txqueuelen 1000  (Ethernet)
         RX packets 209875  bytes 11260238 (10.7 MiB)
         RX errors 0  dropped 0  overruns 0  frame 0
         TX packets 201263  bytes 12927207 (12.3 MiB)
         TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

   eth0: flags=4419<UP,BROADCAST,RUNNING,PROMISC,MULTICAST>  mtu 1500
         inet6 fe80::ba27:ebff:fef3:9ab2  prefixlen 64  scopeid 0x20<link>
         ether b8:27:eb:f3:9a:b2  txqueuelen 1000  (Ethernet)
         RX packets 209581  bytes 12999322 (12.3 MiB)
         RX errors 0  dropped 2285  overruns 0  frame 0
         TX packets 205044  bytes 14541592 (13.8 MiB)
         TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

   eth1: flags=4419<UP,BROADCAST,RUNNING,PROMISC,MULTICAST>  mtu 1500
         inet6 fe80::a2ce:c8ff:fe1e:1745  prefixlen 64  scopeid 0x20<link>
         ether a0:ce:c8:1e:17:45  txqueuelen 1000  (Ethernet)
         RX packets 6019  bytes 1764099 (1.6 MiB)
         RX errors 0  dropped 2320  overruns 0  frame 0
         TX packets 6662  bytes 721472 (704.5 KiB)
         TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

   lo: flags=73<UP,LOOPBACK,RUNNING>  mtu 65536
         inet 127.0.0.1  netmask 255.0.0.0
         inet6 ::1  prefixlen 128  scopeid 0x10<host>
         loop  txqueuelen 1000  (Local Loopback)
         RX packets 38  bytes 3952 (3.8 KiB)
         RX errors 0  dropped 0  overruns 0  frame 0
         TX packets 38  bytes 3952 (3.8 KiB)
         TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

   wlan0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
         inet 192.168.42.92  netmask 255.255.255.0  broadcast 192.168.42.255
         inet6 fe80::d4bf:5c2c:48c0:bf1e  prefixlen 64  scopeid 0x20<link>
         ether b8:27:eb:a6:cf:e7  txqueuelen 1000  (Ethernet)
         RX packets 1883  bytes 135753 (132.5 KiB)
         RX errors 0  dropped 0  overruns 0  frame 0
         TX packets 126  bytes 20075 (19.6 KiB)
         TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

No IP address has to be assigned to the bridge or any of the Ethernet interfaces
in order for the bridge to forward packets from one Ethernet interface to
the other. However the bridge needs to be in ``UP`` state.

To show bridge status::

   pi@pndevice-pi:~$ bridge -d link
   2: eth0: <BROADCAST,MULTICAST,PROMISC,UP,LOWER_UP> mtu 1500 master br0 state forwarding priority 32 cost 4
      hairpin off guard off root_block off fastleave off learning on flood on mcast_flood on neigh_suppress off vlan_tunnel off isolated off
   3: br0: <BROADCAST,MULTICAST,PROMISC,UP,LOWER_UP> mtu 1500 master br0
   5: eth1: <BROADCAST,MULTICAST,PROMISC,UP,LOWER_UP> mtu 1500 master br0 state forwarding priority 32 cost 19
      hairpin off guard off root_block off fastleave off learning on flood on mcast_flood on neigh_suppress off vlan_tunnel off isolated off

You can also use the ``brctl`` Linux command from the ``bridge-utils`` package::

   pi@pndevice-pi:~$ brctl show
   bridge name    bridge id            STP enabled    interfaces
   br0            8000.3e9b37c63eee    no             eth0
                                                      eth1

Another useful Linux command is ``networkctl``::

   pi@pndevice-pi:~$ networkctl
   IDX LINK             TYPE               OPERATIONAL SETUP
     1 lo               loopback           carrier     unmanaged
     2 eth0             ether              degraded    configured
     3 br0              bridge             degraded    unmanaged
     4 eth1             ether              degraded    configured
     5 wlan0            wlan               routable    unmanaged

   5 links listed.

To disable the creation of the bridge at reboot::

   sudo systemctl disable systemd-networkd

If you do not use systemd, then a script ``enable_bridge.sh`` might be handy::

   ip link add name br0 type bridge
   ip link set br0 up
   ip link set eth0 up
   ip link set eth1 up
   ip link set eth0 master br0
   ip link set eth1 master br0

And correspondingly ``disable_bridge.sh``::

   ip link set eth0 nomaster
   ip link set eth1 nomaster
   ip link set br0 down
   ip link delete br0 type bridge


Configuration of p-net stack and sample application
---------------------------------------------------------
To run p-net and the sample application with multiple ports a couple
of things need to be done. Note that the settings described in the
following sections are changed by running ``ccmake .`` in the build folder,
and then ``options.h`` will be regenerated.

Reconfigure setting ``PNET_MAX_PHYSICAL_PORTS`` to the actual number of physical
ports available in the system. For this example the value shall be set to 2.

Reconfigure setting ``PNET_MAX_SUBSLOTS``. Each additional port will require an
additional subslot. For this example the value should be be set to 4.

Another way to set the options is to set them on the cmake command line::

    -DPNET_MAX_PHYSICAL_PORTS=2 -DPNET_MAX_SUBSLOTS=4

The configuration field ``.num_physical_ports`` must be set accordingly. For
the Linux sample application this is set automatically by parsing the command
line arguments, which need to be adjusted.

Example of initial log when starting the demo application with a multi port configuration::

    pi@pndevice-pi:~/profinet/build $ sudo ./pn_dev -v
    ** Starting Profinet demo application **
    Number of slots:      5 (incl slot for DAP module)
    P-net log level:      0 (DEBUG=0, FATAL=4)
    App verbosity level:  1
    Nbr of ports:         2
    Network interfaces:   br0,eth0,eth1
    Button1 file:
    Button2 file:
    Station name:         rt-labs-dev
    Management port:      br0 C2:38:F3:A6:0A:66
    Physical port [1]:    eth0 B8:27:EB:67:14:8A
    Physical port [2]:    eth1 58:EF:68:B5:11:0F
    Current hostname:     pndevice-pi
    Current IP address:   192.168.0.50
    Current Netmask:      255.255.255.0
    Current Gateway:      192.168.0.1
    Storage directory:    /home/pi/profinet/build

Update GSDML file
-----------------
The sample app GSDML file contains a commented out block that defines
a second physical port. In the sample application GSDML file, search for "IDS_P2"
and enable commented out lines as described in the GSDML file.

Note that you will have to the reload GSDML file in all tools you are using and
also the Automated RT tester any time the file is changed.


Running ART tester with multiple ports
--------------------------------------
Use the MAC-address of ``br0`` when running ART tester.


Routing traffic with multiple ports on Linux
---------------------------------------------
To see the MAC addresses and IP addresses of the neighbours, use the ``arp``
Linux command::

   pi@pndevice-pi:~$ arp
   Address                  HWtype  HWaddress           Flags Mask            Iface
   192.168.0.99             ether   20:87:56:ff:aa:83   C                     br0
   192.168.42.1             ether   b4:fb:e4:51:09:72   C                     wlan0
   192.168.0.98             ether   ac:4a:56:f4:02:89   C                     br0
   192.168.0.30             ether   54:ee:75:ff:95:a6   C                     br0

To see the IP routing table, use the ``route`` Linux command::

   pi@pndevice-pi:~$ route
   Kernel IP routing table
   Destination     Gateway         Genmask         Flags Metric Ref    Use Iface
   default         192.168.42.1    0.0.0.0         UG    305    0        0 wlan0
   192.168.0.0     0.0.0.0         255.255.255.0   U     0      0        0 br0
   192.168.42.0    0.0.0.0         255.255.255.0   U     305    0        0 wlan0


Adding multiple Ethernet ports on a microcontroller
---------------------------------------------------
Typically a microcontroller has only one Ethernet controller built in. One way
to provide more Ethernet ports is to use an external Ethernet switch chip.
However the Profinet stack must be able to control from which Ethernet port
a frame is sent from. This is because the outgoing LLDP frames should have
different contents on different ports (important for neighbourhood detection).
Similarly, the stack must be able to figure out on which port an incoming
LLDP frame did arrive, in order to determine the closest neighbour for each
Ethernet port.

Traffic not intended for the device should be routed between the external
Ethernet ports.

There are Ethernet switch chips with functionality to route traffic to
different Ethernet ports from a single Ethernet controller in a microcontroller.
This is done by "tail tagging", where a few additional bytes are put in the
end of outgoing frames. The switch chip will remove those bytes, but will
use the information to route the frame to the correct outgoing Ethernet port.

A detailed description of the concept is given on
https://www.segger.com/products/connectivity/emnet/technology/port-multiplication/

Examples of useful Ethernet switch chips (not tested):

* Microchip KSZ8863
* Microchip KSZ8873
* Microchip KSZ8895

The p-net stack does not implement support for tail tagging by default.
