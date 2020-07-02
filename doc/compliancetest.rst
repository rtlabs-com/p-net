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


Additional hardware
-------------------
Some of the test cases requires additional hardware; a Profinet-enabled switch
("Device B") and an IO-controller ("Device A").
Connection of the switch ports is described in the table below:

+-------------+-----------------------------------------------+
| Switch port | Connected to                                  |
+=============+===============================================+
| P1          | Personal computer running Automated RT Tester |
+-------------+-----------------------------------------------+
| P2          | IO-controller ("Device A")                    |
+-------------+-----------------------------------------------+
| P3          | Device under test (DUT) running p-net         |
+-------------+-----------------------------------------------+

The Automated RT Tester can control a power outlet via Ethernet. It must be
connected via a separate Ethernet interface on the personal computer.


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


Relevant test cases for Automated RT Tester
-------------------------------------------
(Not exhaustive)

+---------------------------------------+-----------------------------------------------------+
| Test case                             | Notes                                               |
+=======================================+=====================================================+
| DCP_1                                 |                                                     |
+---------------------------------------+-----------------------------------------------------+
| DCP_2                                 |                                                     |
+---------------------------------------+-----------------------------------------------------+
| DCP_3                                 |                                                     |
+---------------------------------------+-----------------------------------------------------+
| DCP_4                                 |                                                     |
+---------------------------------------+-----------------------------------------------------+
| DCP_IDN                               |                                                     |
+---------------------------------------+-----------------------------------------------------+
| DCP_NAME_1                            |                                                     |
+---------------------------------------+-----------------------------------------------------+
| DCP_NAME_2                            |                                                     |
+---------------------------------------+-----------------------------------------------------+
| DCP_ResetToFactory                    |                                                     |
+---------------------------------------+-----------------------------------------------------+
| DCP_OPTIONS_SUBOPTIONS                |                                                     |
+---------------------------------------+-----------------------------------------------------+
| DCP_Router                            |                                                     |
+---------------------------------------+-----------------------------------------------------+
| DCP_Access                            |                                                     |
+---------------------------------------+-----------------------------------------------------+
| DCP_VLAN                              |                                                     |
+---------------------------------------+-----------------------------------------------------+
| DCP IP-parameter Remanence            |                                                     |
+---------------------------------------+-----------------------------------------------------+
| Behavior Scenario 1 to 9              |                                                     |
+---------------------------------------+-----------------------------------------------------+
| Behavior Scenario 10                  |                                                     |
+---------------------------------------+-----------------------------------------------------+
| Behavior Scenario 11                  |                                                     |
+---------------------------------------+-----------------------------------------------------+
| Different Access Ways                 |                                                     |
+---------------------------------------+-----------------------------------------------------+
| Diagnosis                             |                                                     |
+---------------------------------------+-----------------------------------------------------+
| AR-ASE                                |                                                     |
+---------------------------------------+-----------------------------------------------------+
| IP_UDP_RPC_I&M_EPM                    |                                                     |
+---------------------------------------+-----------------------------------------------------+
| VLAN                                  |                                                     |
+---------------------------------------+-----------------------------------------------------+
| DCP_Signal (Manual)                   | Flash Signal LED                                    |
+---------------------------------------+-----------------------------------------------------+
| Behavior of ResetToFactory (manual)   |                                                     |
+---------------------------------------+-----------------------------------------------------+
| Manual checking of sending RTC frames |                                                     |
+---------------------------------------+-----------------------------------------------------+

Possibly also these test cases:

* DCP_ALIAS
* ALARM
* PDEV_CHECK_ONEPORT
* RTC

For conformance class B:

* Topology discovery check
* Non-Profinet neighbour setup
* Port-to-port
