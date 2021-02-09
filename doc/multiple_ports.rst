Multiple ports
==============
This section describes how to configure the p-net stack, sample application
and system for multiple network interfaces or ports.
So far, multiple port has only been tested using the linux port.


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

Run ``ifconfig`` to check that the bridge is up and its network interfaces are all up::

    pi@pndevice-pi:~$ ifconfig
    br0: flags=4419<UP,BROADCAST,RUNNING,PROMISC,MULTICAST>  mtu 1500
        ether c2:38:f3:a6:0a:66  txqueuelen 1000  (Ethernet)
        RX packets 6871  bytes 614728 (600.3 KiB)
        RX errors 0  dropped 12  overruns 0  frame 0
        TX packets 929  bytes 288939 (282.1 KiB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

    eth0: flags=4099<UP,BROADCAST,MULTICAST>  mtu 1500
        ether b8:27:eb:67:14:8a  txqueuelen 1000  (Ethernet)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

    eth1: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet6 fe80::c641:1eff:fe82:f67f  prefixlen 64  scopeid 0x20<link>
        ether c4:41:1e:82:f6:7f  txqueuelen 1000  (Ethernet)
        RX packets 6901  bytes 617674 (603.1 KiB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 972  bytes 293401 (286.5 KiB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

    lo: flags=73<UP,LOOPBACK,RUNNING>  mtu 65536
        inet 127.0.0.1  netmask 255.0.0.0
        inet6 ::1  prefixlen 128  scopeid 0x10<host>
        loop  txqueuelen 1000  (Local Loopback)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

Configuration of p-net stack and sample application 
---------------------------------------------------------
To run p-net and the sample application with multiple ports a couple
of things need to be done. Note that the settings described in the 
following sections are changed by running ``ccmake .`` in the build folder.
``options.h`` will be regenerated. Another way to set the options is to 
set them on the cmake command line (-DPNET_NUMBER_OF_PHYSICAL_PORTS=2 -DPNET_MAX_SUBSLOTS=4).

Reconfigure setting ``PNET_NUMBER_OF_PHYSICAL_PORTS`` to the actual number of physical ports available in the system.
For this example ``PNET_NUMBER_OF_PHYSICAL_PORTS`` shall be set to 2. 

Reconfigure setting ``PNET_MAX_SUBSLOTS``. Each additional port will require an additional subslot.
For this example the ``PNET_MAX_SUBSLOTS`` should be be set to 4.

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

Update gsdml file
-----------------
The sample app gsdml file contains a commented out block that defines 
a second physical port. In the sample application gsdml file, search for "IDS_P2" 
and enable commented out lines as described in the gsdml file. 

Note that you will have to the reload gsdml file in all tools you are using and 
also the Automated RT tester any time the file is changed.