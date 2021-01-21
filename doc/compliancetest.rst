Compliance testing of IO-devices
================================
All Profinet products need to be certified. That is done after testing by an
accredited PROFIBUS & PROFINET International test lab (PITL).
The full hardware, communication stack and application software is tested
together.

Each manufacturer of Profinet equipment needs a Vendor ID, which is obtained
from the PI Certification Office.

The tests checks the GSD file, the hardware interface and the Profinet
behavior. The tests should be performed on a device from series production.

The Profinet equipment also needs to fulfill parts of the IEEE802 standards,
for example uniqueness of MAC addresses. This is not tested by the test lab.

A detailed description of the certification process can be downloaded from
https://www.profibus.com/download/how-to-get-a-certificate-for-a-profinet-device/


General requirements
--------------------
It should be possible to do a factory reset without an engineering tool. For
example a hardware switch can be used. Routing of Ethernet frames must not be
affected by a factory reset.

A Profinet signal LED must be visible on the IO-Device. A callback is available
from p-net to control the signal LED, so you can implement your board specific
hardware.

Connectors and cables should fulfill the requirements in "PROFINET Cabling and
Interconnection Technology". For recommended diagnosis indicators (for example
LEDs) see "Diagnosis for PROFINET".
Consider the housing (often IP65) and grounding of the device.
In general the MAC address of the device should be visible when installed.

Hardware requirements for the Ethernet ports:

* At least 100 Mbit/s full duplex
* Standard and crossover cables should be handled
* Auto polarity
* Auto negotiation (should be possible to turn off for fast startup)

Needed for certification:

* EMC test report
* European Union declaration of conformity (for CE marking)
* The user manual must be available.
* A valid GSD file


Tool for pretesting
--------------------
For Profinet members, the "Automated RT tester" tool is available for download.
It is useful for pretesting before a full compliance test.

Profinet devices must fulfill "Security Level 1" with regards to the net
load. This is tested by a separate tool (not the "Automated RT tester").


Installation of Automated RT tester on a Windows PC
---------------------------------------------------
Unzip the downloaded file, and double-click the
"AutomatedRtTester_2.4.1.3_Setup.exe" file.


Adjust the Ethernet network card on a Windows PC
------------------------------------------------
To use the "Automated RT tester" tool you need to adjust the Ethernet interface
settings of your PC. In the properties for the driver, go to the "Advanced"
tab. For the "Packet priority & VLAN" select "Packet priority & VLAN Disable".
More details are given in the "Product Documentation" document for the tool.

You might also need to turn off LLDP protocol for the selected network
interface. Both Windows and Simatic TIA can have LLDP implemented.

Adjust the settings of the Ethernet card of your personal computer to use
100 Mbit/s full duplex (otherwise the test case "Different access ways
port-to-port" will fail).

Set the IP address to ``192.168.0.25`` and netmask to ``255.255.255.0``.

Use a separate network for running tests with Advanced RT tester
(avoid running it on a network with unrelated devices).


Supported GSD versions
----------------------
The "Automated RT tester" tool is compatible only with a few versions of GSDML
files. See the document "PN_Versions_for_certifications" included in the
download.


Create a project
----------------
Use the menu File > New > "Device Test Project". Follow the wizard.
Enter the MAC address of the IO-Device. On the "Profinet settings" page, select
the GSDML file for the IO-Device.

Populate modules/submodules in slots/subslots by
marking a module in the left column, and then click the -> arrow. To insert a
submodule, mark the relevant submodule in the left column and mark the
appropriate slot in the right column.

To remove from a slot or subslot, mark it (in the right column) and press the
<- arrow.


Adjust settings
---------------
You need to adjust the device MAC address. This is done via the menu
Tools > Options > DUT. Enter the value and press OK.

If the device does not have support for remote change of values, you might
need to adjust the device station name, IP address and subnet mask.
This is done via the menu Tools > Options > Setting. You might need to
click the "Show expert settings".


Run tests
---------
Select the tests (in the left side menu) to run. Failing tests are time
consuming, so start with a single test to verify the communication. Disable all
tests by the left side menu. Press the "Deselect all" icon. Then open "Automated
test cases" > "Standard Setup" > DCP, and enable "DCP - DCP_IDN". Use the menu
Project > Run.

When communication is verified, enable all relevant test cases.

The ART tester tool stores Wireshark files (.pcap files) in the
project directory. See the ``EthernetDump`` subdirectory.

Additional hardware
-------------------
Some of the test cases requires additional hardware; a Profinet-enabled switch
("Device B") and an IO-controller ("Device A"). Also a remote controlled
power outlet can be used to simplify the tests.

+-------------------------+-----------------------------+------------------------+
| Item                    | IP address                  | Description            |
+=========================+=============================+========================+
| Device under test (DUT) | 192.168.0.50                |                        |
+-------------------------+-----------------------------+------------------------+
| ART tester on PC        | 192.168.0.25, 192.168.1.143 |                        |
+-------------------------+-----------------------------+------------------------+
| PLC ("Device A")        | 192.168.0.100               |                        |
+-------------------------+-----------------------------+------------------------+
| Switch (“Device B”)     | 192.168.0.99                |                        |
+-------------------------+-----------------------------+------------------------+
| Neighbour (“Device D”)  | 192.168.0.98                | To port 2 of DUT       |
+-------------------------+-----------------------------+------------------------+
| Neighbour (“Device E”)  | 192.168.0.97                | To highest port of DUT |
+-------------------------+-----------------------------+------------------------+
| Power outlet            | 192.168.1.244               | Separate network       |
+-------------------------+-----------------------------+------------------------+


Profinet-enabled switch
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Some of the test cases for the Automated RT Tester requires an Profinet-enabled
switch.

The test specification of version V 2.41 recommends the use of a
Siemens Scalance X204IRT (article number 6GK5204-0BA00-2BA3).
It should have IP address ``192.168.0.99``, netmask ``255.255.255.0`` and station name "b".
Use for example Codesys to scan for the device, and to adjust the IP settings.
Alternatively, use SinecPni to change the IP address (see the Simatic
page in this documentation).

The switch has a web interface, but it is not necessary to do any setting
adjustments via the web interface.
Log in to the web interface by directing your web browser to its IP address.
User name "admin", factory default password "admin".

Connection of the switch ports is described in the table below:

+-------------+-----------------------------------------------+
| Switch port | Connected to                                  |
+=============+===============================================+
| P1          | Personal computer running Automated RT Tester |
+-------------+-----------------------------------------------+
| P2          | IO-controller ("Device A" port X1 P1)         |
+-------------+-----------------------------------------------+
| P3          | Device under test (DUT) running p-net         |
+-------------+-----------------------------------------------+

The Automated RT tester will detect "Device B" by itself. No configuration is
required in the Automated RT tester menu.

The setting "Use IEC V2.2 LLDP mode" available via the STEP7 Profinet setup
tool controls the format of the sent portID in LLDP frames.
With the "Use IEC V2.2 LLDP mode" enabled the portID is sent as ``port-001``,
while it is sent as ``port-001.b`` if disabled. The latter format is used in
Profinet 2.3 and newer. The ART tester requires the LLDP format to be in the
2.2 format, otherwise it will complain about portID length.
One way to restore the behavior to the 2.2 format is to do a factory reset
of the switch via the web interface or by pressing the SET button for more than
20 seconds (if the button not is disabled in the web interface).


Remote controlled power outlet
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The Automated RT Tester can control an "Anel Net-PwrCtrl" power outlet via Ethernet.
It must be connected via a separate Ethernet
interface on the personal computer. Use a static IP address ``192.168.1.243`` with
subnet mask to ``255.255.255.0`` on that interface.

The Power outlet has a default IP address of ``192.168.0.244``, and it has a
built-in web server. Enter its IP address in your web browser to log in
(username and password printed on the hardware).
(You might need to temporary set your Ethernet interface to IP ``192.168.0.1``
and subnet mask to ``255.255.255.0``)
Modify the IP settings (on the "Einstellung" page) to use a static IP address
of ``192.168.1.244``.
On the "Steuerung" page you can control the individual power outputs.

Connect power for your device under test to connector number 3 on the power outlet.

Test the functionality from Automated RT Tester by clicking on the symbol to the
left of the "PowerOutlet" text in the tool bar. The symbol to the right of the
"PowerOutlet" text shows a green check mark when the outputs are on, and a
black cross when the outputs are off (or when the power outlet not is connected).

+--------------+------------------------------------------------------------+
| Power outlet | Connected to                                               |
+==============+============================================================+
| 1            | PLC "A"                                                    |
+--------------+------------------------------------------------------------+
| 2            | Profinet enabled switch "B"                                |
+--------------+------------------------------------------------------------+
| 3            | Device under test (DUT) running p-net                      |
+--------------+------------------------------------------------------------+
| 4            | Neighbour device "D", connected to DUT port 2              |
+--------------+------------------------------------------------------------+
| 5            | Neighbour device "E", connected to DUT highest port number |
+--------------+------------------------------------------------------------+


Hardware naming
^^^^^^^^^^^^^^^
Different types of Siemens hardware are used for the conformance test.
In order to simplify how the different units should be connected together,
a list of Siemens naming conventions is provided here:

* AI: Analog input module
* AQ: Analog output module
* BA: Basic
* BA: Busadapter (with RJ45 or fiber optic connectors)
* BU: BaseUnit (for mounting input and output modules)
* CM: Communication module
* DI: Digital input module
* DP: Profibus DP
* DQ: Digital output module
* F-: Fail safe
* FC: Fast Connect (A bus adapter for network cables)
* HF: High feature
* HS: High speed
* IM: Interface Module
* L+: +24 V DC
* M: Ground connection
* P: Port
* PN: Profinet
* R: Ring port for media redundancy
* SM: Special module
* SP: Scalable Peripherals
* ST: Standard
* TM: Technology module
* X: Interface


Siemens IO-device for verification of multi-port devices
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+--------------------------------------+-------------------------------------------+
| Part                                 | Comments                                  |
+======================================+===========================================+
| Interface module ET200 IM155-6PN/2HF |                                           |
+--------------------------------------+-------------------------------------------+
| Digital output module DQ 132         | In slot 1 (closest to interface module)   |
+--------------------------------------+-------------------------------------------+
| Digital input module DI 131          | In slot 2                                 |
+--------------------------------------+-------------------------------------------+
| Base uint A0 (24 VDC, light colored) | One for each input/output module          |
+--------------------------------------+-------------------------------------------+
| Bus adapter                          | With two RJ45 connectors                  |
+--------------------------------------+-------------------------------------------+
| Server module                        | Delivered with the interface module. Put  |
|                                      | it in slot 3.                             |
+--------------------------------------+-------------------------------------------+

See the Profinet test specification for part numbers.

Light-colored bus adapters are used for supply voltage distribution.
The cyan-colored (auxiliary) terminals on bus-adapters are all connected together.
If you only use light-colored bus adapters, then the cyan-colored terminals on
one bus adapter are isolated from the corresponding terminals on other bus adapters.

Connect +24 V to the red terminals of the interface module and the base units.
Connect 0 V to the blue terminals of the interface module and the base units.

Connect a button via wires to the digital input (DI) module. Connect it between
DI.7 (pin 18) and and +24 V. The LED ".7" on the input module will be green
when the button is pressed.

The LED ".7" on the digital output module (DQ) will be green when the output
is high (+24 V).

.. image:: illustrations/SimaticIoDevice.jpg

See the page on setting up a Simatic PLC in this documentation for
instructions on usage.


Set up Cisco SF352-08P switch
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
For multiport Profinet devices, also SNMP-communication to non-Profinet
devices is verified. This Cisco switch can be used for that purpose.

Connect an Ethernet cable to port G1.
Set your laptop IP address to ``192.168.1.143`` and netmask to be ``255.255.255.0``.
Log in to ``192.168.1.254``. Default username is ``cisco`` and password is ``cisco``.
Change password when prompted.

Set the IP address via the left side menu "IP configuration" -> "IPv4 Management and Interfaces" -> "IPv4 Interface".
Click "Add" and enter the static IP address ``192.168.0.98``. Use netmask ``255.255.255.0``.
The switch will change IP address to a new subnet, so you might need to change your
laptop network setting before connecting to the new IP address.

Adjust LLDP settings via menu Administration -> "Discovery - LLDP" -> Properties.
In the page top bar, set "Display mode" to Advanced. Set "Chassis ID Advertisement"
to "MAC Address".

Via Administration -> "Discovery - LLDP" -> "Port settings" select port FE1 and
click Edit. Enable SNMP notification. Select the optional TLVs that start with "802.3".

Via the menu Security -> "TCO/UDP Services", enable "SNMP Service".

In the page top bar, set "Display mode" to Advanced.
Add a SNMP community via the menu SNMP -> Communities and click Add. The
community string should be "public". Set "SNMP Management Station" to "All".
Click "Apply" and "Close".

In the top of the page click the "Save" icon.

For the actual measurements, use the port 1 on the Cisco switch.


Tips and ideas
--------------
If you end up with ``Pass with Hint "The device made a EPM Request from a
not Profinet port"``, that means that wrong source port was used when sending
UDP messages. See the page on Linux in this documentation on how to adjust the
ephemeral port range.

If your software version indicates that it is a prototype version (letter "P")
the Automated RT Tester will mark this as pass with hint.

The Automated RT Tester has a convenient feature for remotely setting the
station name, IP address, netmask and gateway of the device under test (DUT).
Use the menu Tools > "Set DUT name and IP".
It will change the settings of the IO device via DCP communication. It is also
possible to do a factory reset of the IO device.


Reduce timeout values to speed up testing
-----------------------------------------
It is possible to reduce the timeout values used by Automated RT Tester. This
can be convenient during development, in order to speed up the tests.
Use the menu Tools > Options, and enable "Show expert settings". The time
settings are found on the "Expert Settings" tab.
The times are given in milliseconds.
Remember to use the default values when doing pre-certification testing.

These values have large impact on test execution times:

* ApplicationReadyReqTimeout
* ConnectResTimeout
* DcpResetToFactoryTestSetupTime
* DutBootUpTime
* ReleaseArResTimeout
* StandardTestSetupBootTime
* WriteResTimeout


Relevant test cases for conformance class A
-------------------------------------------

+-------------------------------------------------+-----------------------------------------------------------------+
| Test case                                       | Notes                                                           |
+=================================================+=================================================================+
| DCP_1                                           | Power cycle 8 times.                                            |
+-------------------------------------------------+-----------------------------------------------------------------+
| DCP_2                                           | Power cycle 2 times.                                            |
+-------------------------------------------------+-----------------------------------------------------------------+
| DCP_3                                           | Power cycle 2 times.                                            |
+-------------------------------------------------+-----------------------------------------------------------------+
| DCP_4                                           | Fast                                                            |
+-------------------------------------------------+-----------------------------------------------------------------+
| DCP_ALIAS                                       | Requires additional hardware ("Device B")                       |
+-------------------------------------------------+-----------------------------------------------------------------+
| DCP_IDN                                         | Fast.                                                           |
+-------------------------------------------------+-----------------------------------------------------------------+
| DCP_NAME_1                                      | Power cycle 4 times.                                            |
+-------------------------------------------------+-----------------------------------------------------------------+
| DCP_NAME_2                                      | Power cycle 4 times.                                            |
+-------------------------------------------------+-----------------------------------------------------------------+
| DCP_ResetToFactory                              |                                                                 |
+-------------------------------------------------+-----------------------------------------------------------------+
| DCP_OPTIONS_SUBOPTIONS                          |                                                                 |
+-------------------------------------------------+-----------------------------------------------------------------+
| DCP_Router                                      |                                                                 |
+-------------------------------------------------+-----------------------------------------------------------------+
| DCP_Access                                      | Fast.                                                           |
+-------------------------------------------------+-----------------------------------------------------------------+
| DCP_VLAN                                        | Power cycle 2 times                                             |
+-------------------------------------------------+-----------------------------------------------------------------+
| DCP IP-parameter Remanence                      | Power cycle 4 times.                                            |
+-------------------------------------------------+-----------------------------------------------------------------+
| Behavior Scenario 1 to 9                        | Power cycle                                                     |
+-------------------------------------------------+-----------------------------------------------------------------+
| Behavior Scenario 10                            |                                                                 |
+-------------------------------------------------+-----------------------------------------------------------------+
| Behavior Scenario 11                            |                                                                 |
+-------------------------------------------------+-----------------------------------------------------------------+
| Different Access Ways                           | Requires additional hardware ("Device B")                       |
+-------------------------------------------------+-----------------------------------------------------------------+
| PDEV_CHECK_ONEPORT                              | Requires additional hardware ("Device B"). Power cycle 3 times. |
+-------------------------------------------------+-----------------------------------------------------------------+
| Diagnosis                                       | Requires additional hardware ("Device B"). Power cycle.         |
+-------------------------------------------------+-----------------------------------------------------------------+
| Alarm                                           | Requires additional hardware ("Device B")                       |
+-------------------------------------------------+-----------------------------------------------------------------+
| AR-ASE                                          | Power cycle                                                     |
+-------------------------------------------------+-----------------------------------------------------------------+
| IP_UDP_RPC_I&M_EPM                              | Power cycle                                                     |
+-------------------------------------------------+-----------------------------------------------------------------+
| RTC                                             | Requires additional hardware ("Device B")                       |
+-------------------------------------------------+-----------------------------------------------------------------+
| VLAN                                            | Turn off IO-controller ("device A")                             |
+-------------------------------------------------+-----------------------------------------------------------------+
| Different access ways port-to-port              | Use port-to-port set up                                         |
+-------------------------------------------------+-----------------------------------------------------------------+
| Manual: DCP_Signal                              | Flash Signal LED. Fast.                                         |
+-------------------------------------------------+-----------------------------------------------------------------+
| Manual: Behavior of ResetToFactory              |                                                                 |
+-------------------------------------------------+-----------------------------------------------------------------+
| Manual: Checking of sending RTC frames          | Fast                                                            |
+-------------------------------------------------+-----------------------------------------------------------------+
| Not automated: DataHoldTimer                    | PLC required. Use network tap at DUT.                           |
+-------------------------------------------------+-----------------------------------------------------------------+
| Not automated: Interoperability                 | PLC required                                                    |
+-------------------------------------------------+-----------------------------------------------------------------+
| Not automated: Interoperability with controller | PLC required                                                    |
+-------------------------------------------------+-----------------------------------------------------------------+
| Security Level 1                                | PLC required                                                    |
+-------------------------------------------------+-----------------------------------------------------------------+


Relevant test cases for conformance class B
-------------------------------------------
Set the GSDML file attributes ``ConformanceClass="B"`` and
``SupportedProtocols="SNMP;LLDP"``.

* Behavior scenario 10
* Topology discovery check, standard setup
* Topology discovery check, non-Profinet-neighbour setup
* Port-to-port
* Behavior of reset to factory (manual)


Relevant test cases for multi-port devices
------------------------------------------

* PDEV_RECORDS Requires additional hardware ("Device B")


Relevant test cases for legacy startup mode
-------------------------------------------
Legacy startup mode is defined in Profinet version 2.2 and earlier.
Set the attribute ``StartupMode`` in the GSDML file to ``"Legacy;Advanced"``.
Also the attributes ``PNIO_Version`` and ``NumberOfAR`` affects the ART tester
behavior.

* SM_Legacy
* Different Access Ways
* Different Access Ways port-to-port
* DCP
* AR-ASE
* IP_UDP_RPC_I&M_EPM
* Behavior
* FSU (if also supporting fast startup)
* Interoperability (use another PLC)
* Interoperability with controller (use another PLC)


Relevant test cases for fast startup (FSU)
------------------------------------------
Set the parameters ``ParameterizationSpeedupSupported="true"`` and
``DCP_HelloSupported="true"``. The attribute ``PowerOnToCommReady="700"``
describes the startup time in milliseconds.

* FSU
* Different Access Ways
* Manual FSU test case
* Hardware (no auto-negotiation)


Relevant test cases for DHCP
----------------------------
Set the ``AddressAssignment`` attribute to ``DHCP``.

* DHCP


Other tests
-----------
Your GSDML file should pass the verification with the "GSDMLcheck" tool.


Details on tests with PLC
-------------------------

Load PLC program
^^^^^^^^^^^^^^^^
Verify that the sample application PLC program is working properly with your
IO-device. Button1 should be able to control the state of data LED (LED1).

Interoperability
^^^^^^^^^^^^^^^^
Run with PLC for 10 minutes without errors.
If the device under test has more than one port, there should be 5 IO-devices
connected to the non-PLC port.

The timing should be the fastest allowed according to the GSDML
file, and use 3 "accepted update cycles without IO data".
Record startup and data exchange using Wireshark.

In the Wireshark file, make sure IOPS and IOCS in the cyclic data from the
IO-device have the value GOOD after it has sent the "application ready"
message.
Also verify that there have been no alarms (sort the frames by protocol).

* "Record data"?
* ExpectedIdentification is equal to the RealIdentification?
* How to create additional net load? (using DCP Identify all)
* Implicit read?

Data Hold Timer
^^^^^^^^^^^^^^^
Run with PLC. The timing should be the fastest allowed according to the GSDML
file, and use 3 "accepted update cycles without IO data".
Record startup and data exchange using Wireshark.

Unplug network cable from the PLC.

In the Wireshark file:

* Count the number of cyclic data frames sent by the IO-device before the
  alarm frame appears. It is allowed that 3-6 data frames are sent before
  the alarm frame.
* At startup the first valid data frame should be sent within the data
  hold time.
* The IOCS in the cyclic data from the IO-device should have the value GOOD
  after the "application ready" message has been sent.
* Verify the data cycle time.

Repeat the cable unplugging measurements with reduction ratios (1), 2, 4, 8
and 16. With a cycle time of for example 1 ms this corresponds to a frame
send interval of 1 ms to 16 ms, and a data hold time of 3 ms to 48 ms.

Check that a LLDP frame is sent within 5 seconds, and then every 5 seconds.
The TTL value in the LLDP frame should be 20 seconds.
The MAUtype, "autonegotiation supported" and "autonegotiation enabled" must
be correct.

Interoperability with controller
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Run with PLC. The timing should be the fastest allowed according to the GSDML
file, and use 3 "accepted update cycles without IO data".
Record startup and data exchange using Wireshark.

Verify that the outputs are according to the manual of your IO-device when
you do these actions (repeat several times):

* PLC powered off
* PLC powered on. The program should be running.
* Switch the PLC to stop.
* Switch the PLC to run.
* Disconnect cable from PLC.
* Reconnect the PLC cable.

In the Wireshark file, make sure IOPS and IOCS in the cyclic data from the
IO-device have the value GOOD after it has sent the "application ready"
message.

* Record data?

Security Level 1 tester
-----------------------
Install the tester software on an Ubuntu machine, or in a virtual Ubuntu
machine running on Windows.
See the PDF in the "Security Level 1"/"tester" folder in the downloaded
test bundle.

Use a non-Profinet switch (no LLDP packet filtering) to connect the device
under test (DUT), the PLC and the personal computer running the Security
Level 1 tester software. For single port devices, use port 1 on the PLC and
port 1 on the DUT.

A PLC program is used to both establish cyclic data communication, and to
repeatedly do acyclic data read out from the IO-device.

In Siemens TIA Portal, open the file "normal_d_V2.40.0_V15.1.zap15_1" as
an existing project. Give the path to a local directory that will be used
for the project.

Delete the existing "dut" device and "d" device. Insert your IO-device and
adjust the settings (as described on the page about Siemens PLCs in
this documentation).

In the "Main [OB1]" program, change the line with ``Ihw_ID`` to::

   Ihw_ID := "rt-labs-dev~Head",

Right-click on the PLC icon, and select Compile > "Hardware (rebuild all)" and
then "Software (rebuild all)". Use "Download to device" > "Hardware
configuration" and then "Software (all)".

Verify that there is cyclic communication, and that there is repeated
acyclic data read out.
