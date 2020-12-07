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

Set the IP address to 192.168.0.25 and netmask to 255.255.255.0.

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


Additional hardware
-------------------
Some of the test cases requires additional hardware; a Profinet-enabled switch
("Device B") and an IO-controller ("Device A"). Also a remote controlled
power outlet can be used to simplify the tests.

+-------------------------+-----------------------------+-------------------+
| Item                    | IP address                  | Description       |
+=========================+=============================+===================+
| Device under test (DUT) | 192.168.0.50                |                   |
+-------------------------+-----------------------------+-------------------+
| ART tester on PC        | 192.168.0.25, 192.168.1.143 |                   |
+-------------------------+-----------------------------+-------------------+
| PLC ("Device A")        | 192.168.0.100               |                   |
+-------------------------+-----------------------------+-------------------+
| Switch (“Device B”)     | 192.168.0.99                |                   |
+-------------------------+-----------------------------+-------------------+
| Power outlet            | 192.168.1.244               | Separate network  |
+-------------------------+-----------------------------+-------------------+


Profinet-enabled switch
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Some of the test cases for the Automated RT Tester requires an Profinet-enabled
switch.

The test specification of version V 2.41 recommends the use of a
Siemens Scalance X204IRT (article number 6GK5204-0BA00-2BA3).
It should have IP address 192.168.0.99, netmask 255.255.255.0 and station name "b".
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


Remote controlled power outlet
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The Automated RT Tester can control an "Anel Net-PwrCtrl" power outlet via Ethernet.
It must be connected via a separate Ethernet
interface on the personal computer. Use a static IP address 192.168.1.243 with
subnet mask to 255.255.255.0 on that interface.

The Power outlet has a default IP address of 192.168.0.244, and it has a
built-in web server. Enter its IP address in your web browser to log in
(username and password printed on the hardware).
(You might need to temporary set your Ethernet interface to IP 192.168.0.1
and subnet mask to 255.255.255.0)
Modify the IP settings (on the "Einstellung" page) to use a static IP address
of 192.168.1.244.
On the "Steuerung" page you can control the individual power outputs.

Connect power for your device under test to connector number 3 on the power outlet.

Test the functionality from Automated RT Tester by clicking on the symbol to the
left of the "PowerOutlet" text in the tool bar. The symbol to the right of the
"PowerOutlet" text shows a green check mark when the outputs are on, and a
black cross when the outputs are off (or when the power outlet not is connected).

Hardware naming
^^^^^^^^^^^^^^^
Different types of Siemens hardware are used for the conformance test.
In order to simplify how the different units should be connected together,
a list of Siemens naming conventions is provided here:

* AI: Analog input module
* AQ: Analog output module
* BA: Basic
* BU: BaseUnit (for mounting input and output modules)
* CM: Communication module
* DI: Digital input module
* DP: Profibus DP
* DQ: Digital output module
* F-: Fail safe
* HF: High feature
* HS: High speed
* IM: Interface Module
* P: Port
* PN: Profinet
* R: Ring port for media redundancy
* SM: Special module
* ST: Standard
* TM: Technology module
* X: Interface

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

+-------------------------------------------------+---------------------------------------------------------------+
| Test case                                       | Notes                                                         |
+=================================================+===============================================================+
| DCP_1                                           | Power cycle 8 times.                                          |
+-------------------------------------------------+---------------------------------------------------------------+
| DCP_2                                           | Power cycle 2 times.                                          |
+-------------------------------------------------+---------------------------------------------------------------+
| DCP_3                                           | Power cycle 2 times.                                          |
+-------------------------------------------------+---------------------------------------------------------------+
| DCP_4                                           | Fast                                                          |
+-------------------------------------------------+---------------------------------------------------------------+
| DCP_ALIAS                                       | Requires additional hardware ("Device B")                     |
+-------------------------------------------------+---------------------------------------------------------------+
| DCP_IDN                                         | Fast.                                                         |
+-------------------------------------------------+---------------------------------------------------------------+
| DCP_NAME_1                                      | Power cycle 4 times.                                          |
+-------------------------------------------------+---------------------------------------------------------------+
| DCP_NAME_2                                      | Power cycle 4 times.                                          |
+-------------------------------------------------+---------------------------------------------------------------+
| DCP_ResetToFactory                              |                                                               |
+-------------------------------------------------+---------------------------------------------------------------+
| DCP_OPTIONS_SUBOPTIONS                          |                                                               |
+-------------------------------------------------+---------------------------------------------------------------+
| DCP_Router                                      |                                                               |
+-------------------------------------------------+---------------------------------------------------------------+
| DCP_Access                                      | Fast.                                                         |
+-------------------------------------------------+---------------------------------------------------------------+
| DCP_VLAN                                        | Power cycle 2 times                                           |
+-------------------------------------------------+---------------------------------------------------------------+
| DCP IP-parameter Remanence                      | Power cycle 4 times.                                          |
+-------------------------------------------------+---------------------------------------------------------------+
| Behavior Scenario 1 to 9                        | Power cycle                                                   |
+-------------------------------------------------+---------------------------------------------------------------+
| Behavior Scenario 10                            |                                                               |
+-------------------------------------------------+---------------------------------------------------------------+
| Behavior Scenario 11                            |                                                               |
+-------------------------------------------------+---------------------------------------------------------------+
| Different Access Ways                           | Requires additional hardware ("Device B")                     |
+-------------------------------------------------+---------------------------------------------------------------+
| PDEV_CHECK_ONEPORT                              | Requires additional hardware ("Device B"). Power cycle twice. |
+-------------------------------------------------+---------------------------------------------------------------+
| Diagnosis                                       | Power cycle.                                                  |
+-------------------------------------------------+---------------------------------------------------------------+
| Alarm                                           | Requires additional hardware ("Device B")                     |
+-------------------------------------------------+---------------------------------------------------------------+
| AR-ASE                                          | Power cycle                                                   |
+-------------------------------------------------+---------------------------------------------------------------+
| IP_UDP_RPC_I&M_EPM                              |                                                               |
+-------------------------------------------------+---------------------------------------------------------------+
| RTC                                             | Requires additional hardware ("Device B")                     |
+-------------------------------------------------+---------------------------------------------------------------+
| VLAN                                            | Turn off IO-controller ("device A")                           |
+-------------------------------------------------+---------------------------------------------------------------+
| Different access ways port-to-port              | Use port-to-port set up                                       |
+-------------------------------------------------+---------------------------------------------------------------+
| Manual: DCP_Signal                              | Flash Signal LED. Fast.                                       |
+-------------------------------------------------+---------------------------------------------------------------+
| Manual: Behavior of ResetToFactory              |                                                               |
+-------------------------------------------------+---------------------------------------------------------------+
| Manual: Checking of sending RTC frames          | Fast                                                          |
+-------------------------------------------------+---------------------------------------------------------------+
| Not automated: DataHoldTimer                    | PLC required. Use network tap at DUT.                         |
+-------------------------------------------------+---------------------------------------------------------------+
| Not automated: Interoperability                 | PLC required                                                  |
+-------------------------------------------------+---------------------------------------------------------------+
| Not automated: Interoperability with controller | PLC required                                                  |
+-------------------------------------------------+---------------------------------------------------------------+
| Security Level 1                                | PLC required                                                  |
+-------------------------------------------------+---------------------------------------------------------------+


Relevant test cases for conformance class B
-------------------------------------------
Set the GSDML file attributes ``ConformanceClass="B"`` and
``SupportedProtocols="SNMP;LLDP"``.

* Topology discovery check, standard setup
* Topology discovery check, non-Profinet-neighbour setup
* Port-to-port


Relevant test cases for multi-port devices
------------------------------------------

* PDEV_RECORDS


Relevant test cases for legacy startup mode
-------------------------------------------
Legacy startup mode is defined in Profinet version 2.2 and earlier.
Set the attribute ``StartupMode`` in the GSDML file to ``"Legacy;Advanced"``.
Also the attributes ``PNIO_Version`` and ``NumberOfAR`` affects the ART tester
behavior.

* SM_Legacy
* Different Access Ways
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


Relevant test cases for DHCP
----------------------------
Set the ``AddressAssignment`` attribute to ``DHCP``.

* DHCP


Other tests
-----------

* GSDMLcheck


Test details
------------


Data Hold Timer
^^^^^^^^^^^^^^^

* Manual checking of LLDP frames. Should reflect real port state.
* Check real cycle time
* Check IOPS in startup of cyclic data
* Disconnect controller, and study number of frames from IO device until alarm.
  Repeat for different data exchange cycle times.

Interoperability
^^^^^^^^^^^^^^^^
Run with PLC for 10 minutes without errors. Record startup and data exchange using Wireshark.

* ExpectedIdentification is equal to the RealIdentification?
* Additional net load?
* Implicit read?

Interoperability with controller
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Run with PLC, then switch PLC to stop. Study the outputs of the IO-device.
Disconnect cable from PLC. Study the outputs of the IO-device.
