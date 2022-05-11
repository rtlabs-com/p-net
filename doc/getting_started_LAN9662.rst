LAN9662
=======

Introduction
------------
Add description of LAN9662 when datasheet is available.

For links to Microchip LAN9662 resources see section  :ref:`lan9662-resources`.

Known limitations:

- One byte write operations are not supported from
  LAN9662 RTE to the I/O FPGA. This means the sample submodule
  Digital Output 1x8 is not working in full rte mode.
- Submodule default values are not applied on AR teardown
  in full rte mode.

LAN9662 Profinet Solution
-------------------------

P-Net implements support for the networking hardware offload features in the LAN9662.
When hardware offload is enabled, Profinet cyclic data frames are handled by the LAN9662 Real
Time Engine (RTE). During AR / connection establishment the RTE is configured by the P-Net stack and
sending and receiving cyclic data is handed over the RTE. The P-Net stack still handles
all non-cyclic Profinet frames.

The cyclic process data is defined by the Profinet device application.
In Profinet the device input data is sent to the controller using the Producer Protocol Machine (PPM) and
the output data is received using the the Consumer Protocol Machine (CPM).
For the PPM side the RTE is configured to read input data sources and build a frame
that is periodically sent to the controller.
For the CPM side the RTE is configured to extract output data in received
frames and write it to configured output sinks.

The RTE process data can be mapped to SRAM or a QSPI HW device.

When hardware offload is enabled all process data by default is mapped to SRAM
by P-Net and the application uses the P-Net API the same way as usual.

To map the process data to a QSPI interface the application uses the P-Net API functions
``pnet_mera_input_set_rte_config()`` for input data and
``pnet_mera_output_set_rte_config()`` for output data.
When a QSPI data source or sink is configured the process data
is handled by the configured HW device and not by the application.
The P-Net stack still keeps a copy of the process data in SRAM
and the application can read the process data from this copy.

All P-Net interactions with the LAN9662 RTE is done through the MERA library provided by Microchip.
Detailed information about the LAN9662 and the RTE can be found in the references in section :ref:`lan9662-resources`.

Table below shows how the the P-Net API for handling the process data is affected when RTE is enabled.

+---------------------------------+-----------+------------------------------------------------------+
| API Function                    | RTE-SRAM  | RTE-QSPI                                             |
+=================================+===========+======================================================+
| pnet_input_set_data_and_iops    | Supported | Not supported. Input IO data from QSPI.              |
+---------------------------------+-----------+------------------------------------------------------+
| pnet_input_get_data_and_iops    | Supported | Supported                                            |
+---------------------------------+-----------+------------------------------------------------------+
| net_input_get_iocs              | Supported | Supported                                            |
+---------------------------------+-----------+------------------------------------------------------+
| pnet_output_get_data_and_iops   | Supported | Supported for reading output IO data mapped to QSPI. |
+---------------------------------+-----------+------------------------------------------------------+
| pnet_output_set_iocs            | Supported | Supported                                            |
+---------------------------------+-----------+------------------------------------------------------+
| pnet_mera_output_set_rte_config | Not used  | Used for configuration                               |
+---------------------------------+-----------+------------------------------------------------------+
| pnet_mera_input_set_rte_config  | Not used  | Used for configuration                               |
+---------------------------------+-----------+------------------------------------------------------+


Build-time configuration
------------------------
Support for LAN9962 features in the P-Net stack is enabled using the following CMake options:

- PNET_OPTION_DRIVER_ENABLE - Enable the P-Net support for hardware drivers in general
- PNET_OPTION_DRIVER_LAN9662 - Enable the support for the LAN9662 driver
- PNET_OPTION_LAN9662_SHOW_RTE_INFO - Print the RTE configuration to the application log

The LAN9662 sample is built with the following P-Net options::

    set(HAS_PNET ON)
    set(BUILD_TESTING OFF)
    set(PNET_OPTION_DRIVER_ENABLE ON)
    set(PNET_OPTION_DRIVER_LAN9662 ON)
    set(PNET_OPTION_LAN9662_SHOW_RTE_INFO OFF)
    set(PNET_MAX_PHYSICAL_PORTS "2" CACHE STRING "LAN9962 2 Ports" FORCE)
    set(PNET_MAX_SLOTS "13" CACHE STRING "LAN9962 sample application" FORCE)
    set(PNET_MAX_SUBSLOTS "4" CACHE STRING "LAN9962 2 Ports" FORCE)
    set(LOG_LEVEL "FATAL" CACHE STRING "Enable logging" FORCE)


Run-time configuration
----------------------
The build-time configuration described in previous section enables the support for the LAN9662 driver.
The application decides if the hardware offload shall be used when p-net stack is initialized.
The hardware offload feature is enabled using the driver_enable parameter part of
the configuration (pnet_cfg_t) passed to P-Net during initialization.
When set to true, LAN9662 specific CPM and PPM drivers are used in the P-Net stack and handling of cyclic data frames
is handed off to the LAN9662 RTE during AR establishment.
The driver_config parameter contains base IDs for various resources in the LAN9662 RTE.
In a system where P-Net is the only feature using the RTE these don't need to be considered.
If not, the IDs must be set not to conflict with other parts of the system.

Input data is mapped to QSPI using the operation ``pnet_input_set_rte_config()``.
Output data is mapped to QSPI using the operation ``pnet_output_set_rte_config()``.


EVB-LAN9662
-----------
Both the default P-Net sample application and the LAN9662 sample application can be run on the EVB-LAN9662.
Utility scripts for configuration of LEDs and buttons and starting the applications are provided in the ``p-net/samples/pn_dev_lan9662/`` directory.
EVB-LAN9662 Features used by P-Net samples:

- Shell is available on the EVB-LAN9662 USB connector marked ``CONSOLE``. Use 115200 baud, no flow control.
- The io-fpga is connected to the EVB-LAN9662-Carrier USB connector marked ``FPGA SPI``.
- Two ethernet ports are supported. Sample scripts show how to configure a network bridge.

LAN9662 Sample Application
--------------------------

The application focus on the process data and its mapping to the RTE.
The source code is is found in ``/p-net/samples/pn_dev_lan9662/``.
The sample application builds for and runs on the EVB-LAN9662.

It supports the following I/O-data:

============== ======================= =========================================== ============
[Slot,Subslot] Name                    Shared memory area                          FPGA addr [func]
============== ======================= =========================================== ============
[1,1]          Digital Input 1x8       /dev/shm/pnet-in-1-1-digital_input_1x8      0x100 [mem]
[2,1]          Digital Output 1x8      /dev/shm/pnet-out-2-1-digital_output_1x8    0x104 [mem]
[3,1]          Digital Input 1x64      /dev/shm/pnet-in-3-1-digital_input_1x64     0x108 [mem]
[4,1]          Digital Input 2x32a     /dev/shm/pnet-in-4-1-digital_input_2x32_a   0x110 [mem]
[5,1]          Digital Input 2x32b     /dev/shm/pnet-in-5-1-digital_input_2x32_b   0x118 [mem]
[6,1]          Digital Input 1x800     /dev/shm/pnet-in-6-1-digital_input_1x800    0x120 [mem]
[7,1]          Digital Output 1x64     /dev/shm/pnet-out-7-1-digital_out_1x64      0x184 [mem]
[8,1]          Digital Output 2x32a    /dev/shm/pnet-out-8-1-digital_output_2x32_a 0x18c [mem]
[9,1]          Digital Output 2x32b    /dev/shm/pnet-out-9-1-digital_output_2x32_b 0x194 [mem]
[10,1]         Digital Output 1x800    /dev/shm/pnet-out-10-1-digital_output_1x800 0x19c [mem]
[11,1]         Digital Input Port A    Not supported                               0x200 [gpios]
[12,1]         Digital Output Port A   Not supported                               0x10  [gpios]
============== ======================= =========================================== ============

Note that the I/Os on slots 11 and 12 are available at the EVB-LAN9662 pin lists IN-A and OUT-A.
The sample application gsdml file is available at ``/p-net/samples/pn_dev_lan9662/``.

The application has three modes of operation. The mode is a runtime configuration defined by the mode (-m) argument:

-m none     RTE disabled. Application process data mapped to shared memory.
-m cpu      RTE enabled. RTE maps process data to SRAM. Application process data mapped to shared memory.
-m full     RTE enabled. RTE maps process data to QSPI. Application process data mapped to io-fpga.

**Mode none**

In this mode the input/output data is mapped to shared memory.
The shared memory can be accessed using the pn_shm_tool or by another
application in the system. HW offload is disabled and the default data
path of the P-Net stack is used.

The shared memory is accessed using the pn_shm_tool. Run ``/usr/bin/pn_shm_tool -h`` for further details.

**Mode cpu**

This mode shows how to use the P-Net with LAN9662 RTE SRAM data.
Also in this mode the input/output data is mapped to shared memory.
The shared memory can be accessed using the pn_shm_tool or by another
application in the system. HW offload is enabled and the cyclic data is
is handled by the LAN9662 RTE. To a user of the application it is no
difference to the "none"-mode. However P-Net copies application process
data to SRAM which is mapped to the cyclic data frames handled by the RTE.

The shared memory is accessed using the pn_shm_tool. Run ``/usr/bin/pn_shm_tool -h`` for further details.

**Mode full**

This mode shows how to use P-Net with LAN9662 RTE QSPI data.
Shared memory is not used. The input/output data is mapped to
the IO-FPGA on the EVB-LAN9662.

The fpga is accessed using the mera-iofpga-rw tool. Run ``mera-iofpga-rw -h`` for further details.
Note that Port A outputs and Port A inputs can be accessed without the mera-iofpga-rw tool since they are physically available on the EVB-LAN9662.

Note that the mera-iofpga-rw tool is run on a host system, not on the LAN9662.
See :ref:`lan9662-resources` for further information on the io-fpga tool.

Running the LAN9662 Sample Application
--------------------------------------
Start the LAN992 sample application using the script ``switchdev-profinet-example.sh``.
The log from a scenario with a PLC using input port A and output port A is shown below.

log::

    switchdev-profinet-example.sh
    Starting switchdev-profinet-example
    [   31.365807] EXT4-fs (mmcblk0p2): recovery complete
    [   31.371838] EXT4-fs (mmcblk0p2): mounted filesystem with ordered data mode. Opts: (null). Quota mode: disabled.
    ANA_PGID[61]                                                              = 0x0000010f -> 0x000001ff
    net.ipv6.conf.br0.disable_ipv6 = 1
    [   31.647312] lan966x-switch e2000000.switch eth0: PHY [e200413c.mdio-mii:01] driver [Microchip INDY Gigabit Internal] (irq=POLL)
    [   31.658804] lan966x-switch e2000000.switch eth0: configuring for phy/gmii link mode
    [   31.786815] 8021q: adding VLAN 0 to HW filter on device eth0
    [   31.877381] lan966x-switch e2000000.switch eth1: PHY [e200413c.mdio-mii:02] driver [Microchip INDY Gigabit Internal] (irq=POLL)
    [   31.888861] lan966x-switch e2000000.switch eth1: configuring for phy/gmii link mode
    [   32.016817] 8021q: adding VLAN 0 to HW filter on device eth1
    [   32.032693] br0: port 1(eth0) entered blocking state
    [   32.037722] br0: port 1(eth0) entered disabled state
    [   32.042987] device eth0 entered promiscuous mode
    [   32.047909] br0: port 1(eth0) entered blocking state
    [   32.052820] br0: port 1(eth0) entered forwarding state
    [   32.068835] br0: port 2(eth1) entered blocking state
    [   32.073767] br0: port 2(eth1) entered disabled state
    [   32.079243] device eth1 entered promiscuous mode
    [   32.084021] br0: port 2(eth1) entered blocking state
    [   32.088977] br0: port 2(eth1) entered forwarding state
    QSYS_SW_PORT_MODE[4]                                                      = 0x00005002 -> 0x00045000
    Starting LAN9662 Profinet sample application
    RTE mode: full

    ** Starting P-Net sample application 0.2.0 **
    Number of slots:      13 (incl slot for DAP module)
    P-net log level:      2 (DEBUG=0, FATAL=4)
    App log level:        0 (DEBUG=0, FATAL=4)
    Max number of ports:  2
    Network interfaces:   br0,eth0,eth1
    Button1 file:
    Button2 file:
    Default station name: lan9662-dev
    Management port:      br0 12:A9:2D:16:93:83
    Physical port [1]:    eth0 12:A9:2D:16:93:81
    Physical port [2]:    eth1 12:A9:2D:16:93:82
    Hostname:             vcoreiii
    IP address:           0.0.0.0
    Netmask:              0.0.0.0
    Gateway:              0.0.0.0
    Storage directory:    /tmp/pn_data

    Application RTE mode "full"
    Slot [1,1] Digital Input 1x8 mapped to FPGA address 0x100
    Slot [2,1] Digital Output 1x8 mapped to FPGA address 0x104
    Slot [3,1] Digital Input 1x64 mapped to FPGA address 0x108
    Slot [4,1] Digital Input 2x32 a mapped to FPGA address 0x110
    Slot [5,1] Digital Input 2x32 b mapped to FPGA address 0x118
    Slot [6,1] Digital Input 1x800 mapped to FPGA address 0x120
    Slot [7,1] Digital Output 1x64 mapped to FPGA address 0x184
    Slot [8,1] Digital Output 2x32 a mapped to FPGA address 0x18c
    Slot [9,1] Digital Output 2x32 b mapped to FPGA address 0x194
    Slot [10,1] Digital Output 1x800 mapped to FPGA address 0x19c
    Slot [11,1] Digital Input Port A mapped to FPGA address 0x200
    Slot [12,1] Digital Output Port A mapped to FPGA address 0x10
    Profinet signal LED indication. New state: 0
    LED 2 new state 0
    Network script for br0:  Set IP 0.0.0.0   Netmask 0.0.0.0   Gateway 0.0.0.0   Permanent: 1   Hostname: lan9662-dev   Skip setting hostname: true
    No valid default gateway given. Skipping setting default gateway.
    LED 1 new state 0
    Plug DAP module and its submodules
    Module plug indication API 0
    [0] Pull old module
    [0] Plug module. Module ID: 0x1 "DAP 1"
    Submodule plug indication API 0
    [0,1] Pull old submodule.
    [0,1] Plug submodule. Submodule ID: 0x1 Data Dir: NO_IO In: 0 Out: 0 "DAP Identity 1"
    Submodule plug indication API 0
    [0,32768] Pull old submodule.
    [0,32768] Plug submodule. Submodule ID: 0x8000 Data Dir: NO_IO In: 0 Out: 0 "DAP Interface 1"
    Submodule plug indication API 0
    [0,32769] Pull old submodule.
    [0,32769] Plug submodule. Submodule ID: 0x8001 Data Dir: NO_IO In: 0 Out: 0 "DAP Port 1"
    Submodule plug indication API 0
    [0,32770] Pull old submodule.
    [0,32770] Plug submodule. Submodule ID: 0x8002 Data Dir: NO_IO In: 0 Out: 0 "DAP Port 2"
    Waiting for PLC connect request

    [   32.544399] br0: port 1(eth0) entered disabled state
    [   32.555613] br0: port 2(eth1) entered disabled state
    [   34.164904] lan966x-switch e2000000.switch eth1: Link is Up - 100Mbps/Full - flow control off
    [   34.173514] IPv6: ADDRCONF(NETDEV_CHANGE): eth1: link becomes ready
    [   34.179964] br0: port 2(eth1) entered blocking state
    [   34.184904] br0: port 2(eth1) entered forwarding state
    [   34.884950] lan966x-switch e2000000.switch eth0: Link is Up - 1Gbps/Full - flow control rx/tx
    [   34.893743] IPv6: ADDRCONF(NETDEV_CHANGE): eth0: link becomes ready
    [   34.900201] br0: port 1(eth0) entered blocking state
    [   34.905155] br0: port 1(eth0) entered forwarding state
    [   37.214719] NOHZ tick-stop error: Non-RCU local softirq work is pending, handler #08!!!
    Network script for br0:  Set IP 192.168.0.50   Netmask 255.255.255.0   Gateway 192.168.0.50   Permanent: 0   Hostname: lan9662-dev   Skip setting hostname: true
    [   39.225858] NOHZ tick-stop error: Non-RCU local softirq work is pending, handler #08!!!
    [   39.235956] NOHZ tick-stop error: Non-RCU local softirq work is pending, handler #08!!!
    Module plug indication API 0
    [11] Pull old module
    [11] Plug module. Module ID: 0x100b "DI Port A"
    Submodule plug indication API 0
    [11,1] Pull old submodule.
    [11,1] Plug submodule. Submodule ID: 0x200b Data Dir: INPUT In: 4 Out: 0 "Digital Input Port A"
    [11,1,"Digital Input Port A"]            Set RTE QSPI address 0x200
    Module plug indication API 0
    [12] Pull old module
    [12] Plug module. Module ID: 0x100c "DO Port A"
    Submodule plug indication API 0
    [12,1] Pull old submodule.
    [12,1] Plug submodule. Submodule ID: 0x200c Data Dir: OUTPUT In: 0 Out: 4 "Digital Output Port A"
    [12,1,"Digital Output Port A"]           Set RTE QSPI address 0x10
    PLC connect indication. AREP: 1
    ANA_RT_VLAN_PCP[1].PCP_MASK                                                                                       = 0x00000000 -> 0x000000ff
    ANA_RT_VLAN_PCP[1].VLAN_ID                                                                                        = 0x00000000 -> 0x00000000
    ANA_RT_VLAN_PCP[1].VLAN_PCP_ENA                                                                                   = 0x00000000 -> 0x00000001
    vcap add is1 10 3 s1_rt first 0 rt_vlan_idx 1 0x7 l2_mac 12:A9:2D:16:93:83 ff:ff:ff:ff:ff:ff rt_type 1 0x3 rt_frmid 32769 0xffff s1_rt rtp_id 5 fwd_ena 1 fwd_mas0
    key field first: value: 0
    key field rt_vlan_idx: value: 01 mask: 07
    key field l2_mac: value: 8393162da912 mask: ffffffffffff
    key field rt_type: value: 01[   39.549942] NOHZ tick-stop error: Non-RCU local softirq work is pending, handler #08!!!
    mask: 03
    key field rt_frmid: value: 00008001 mask: 0000ffff
    act field rtp_id: val[   39.565901] NOHZ tick-stop error: Non-RCU local softirq work is pending, handler #08!!!
    ue: 0x5
    act field fwd_ena: value: 0x1
    act field fwd_mask: value: 0x10
    Event indication PNET_EVENT_STARTUP   AREP: 1
    PLC dcontrol message. AREP: 1  Command: PRM_END
    Event indication PNET_EVENT_PRMEND   AREP: 1
    [0,1,"DAP Identity 1"]                   Set input data and IOPS. Size: 0 IOPS: GOOD
    [0,32768,"DAP Interface 1"]              Set input data and IOPS. Size: 0 IOPS: GOOD
    [0,32769,"DAP Port 1"]                   Set input data and IOPS. Size: 0 IOPS: GOOD
    [0,32770,"DAP Port 2"]                   Set input data and IOPS. Size: 0 IOPS: GOOD
    [11,1,"Digital Input Port A"]            Set input data and IOPS. Size: 4 IOPS: GOOD
    [12,1,"Digital Output Port A"]           Set output IOCS: GOOD
    vcap add is1 10 2 s1_rt first 0 l2_mac 12:A9:2D:16:93:83 ff:ff:ff:ff:ff:ff rt_vlan_idx 0 0x7 rt_frmid 32768 0xffff s1_rt rtp_id 4 rtp_subid 0 rte_inb_upd 1 fwd_e0
    key field first: value: 0
    key field l2_mac: value: 8393162da912 mask: ffffffffffff
    key field rt_vlan_idx: value: 00 mask: 07
    key field rt_frmid: value: 00008000 mask: 0000ffff
    act field rtp_id: value: 0x4
    act field rtp_subid: value: 0x0
    act field rte_inb_upd: value: 0x1
    act field fwd_ena: value: 0x1
    act field fwd_mask: value: 0x10
    [11,1,"Digital Input Port A"]            PLC reports Consumer Status (IOCS) GOOD
    Application will signal that it is ready for data, for AREP 1.
    Event indication PNET_EV[   39.883951] NOHZ tick-stop error: Non-RCU local softirq work is pending, handler #08!!!
    ENT_APPLRDY   AREP: 1
    Event indication PNET_EVENT_DATA   AREP: 1
    Cyclic data transmission started

    PLC ccontrol message confirmation. AREP: 1  Status codes: 0 0 0 0
    [12,1,"Digital Output Port A"]           PLC reports Provider Status (IOPS) GOOD



Building the LAN9662 Sample Application
---------------------------------------
Add step by step guide describing how to build the LAN9662
sample application and which Microchip resources to download
when that information is available.


P-Net on LAN9962 Application Summary
------------------------------------
- To map process data to QSPI the application must use the operations ``pnet_output_set_rte_config()`` and ``pnet_input_set_rte_config()``
- If process data is handled by the application and not mapped to QSPI the hardware offload can be enabled and used without any change in the application. API usage is identical except that the hardware offload is enabled during stack initialization.
- ``p-net/samples/pn_dev_lan9662/switchdev-profinet-example.sh`` shows the required systems configurations for a 2 port Profinet device application.

.. _lan9662-resources:

LAN9662 Resources
-----------------

Microchip provides pre-built buildroot images for the LAN9662 which is used
to run the Profinet sample application:

- http://mscc-ent-open-source.s3-eu-west-1.amazonaws.com/index.html?prefix=public_root/

To build the Profinet sample application the following bsp and toolchain are used:

- http://mscc-ent-open-source.s3-eu-west-1.amazonaws.com/public_root/bsp/mscc-brsdk-arm-2021.09.tar.gz
- http://mscc-ent-open-source.s3-eu-west-1.amazonaws.com/public_root/toolchain/mscc-toolchain-bin-2021.02-090.tar.gz

Documentation and data sheets:

- http://mscc-ent-open-source.s3-eu-west-1.amazonaws.com/public_root/bsp/mscc-brsdk-doc-2021.09.html
- Add more documents when LAN9662 documentation is available.

