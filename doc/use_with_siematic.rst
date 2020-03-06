Using a Simatic PLC as IO controller
====================================
This have been tested with:

* Simatic S7-1200 CPU1215C DC/DC/DC 6ES7 215-1AG40-0xXB0 Firmware version 4.2

It uses a 24 V DC supply voltage.


Install Siemens TIA on a Windows PC
-----------------------------------
Install TIA (SIMATIC STEP 7 and WinCC) V15.1

Download the files from
`https://support.industry.siemens.com/cs/document/109752566/simatic-step-7-and-wincc-v15-trial-download-?dti=0&lc=en-US <https://support.industry.siemens.com/cs/document/109752566/simatic-step-7-and-wincc-v15-trial-download-?dti=0&lc=en-US>`_

* "SIMATIC STEP 7 Basic V15.1 is the easy-to-use engineering system for the
  small modular SIMATIC S7-1200 controller and the associated I/Os."
* "SIMATIC STEP 7 Professional V15.1 is the high-performance, integrated
  engineering system for the latest SIMATIC controllers S7-1500, S7-1200,
  S7-300, S7-400, WinAC and ET 200 CPU."

"SIMATIC WinCC is a supervisory control and data acquisition (SCADA) and
human-machine interface (HMI) system from Siemens."
See https://en.wikipedia.org/wiki/WinCC

Download "WinCC Professional". Use all files from "DVD 1 Setup". The "DVD 2"
files are not necessary.

The installer is split in a number of files, each 2 GByte.
Put the files in a directory on your Windows PC.
To start the installation, double-click the exe file. Restart of your computer
is required.

To show installed software version, use the "Portal view" and the "Start" tab
to the left of the screen. Click "Installed software".


Using Siemens TIA on a Windows PC: Add devices
----------------------------------------------
Start the application by searching for "TIA portal" in the Windows start menu.

In the start screen, select "Create new project" and enter the details.
Also on the start screen, select "Devices and networks", "Add new device" and
select your PLC model (a CPU that will act as IO-controller).

Import a GSD file by using the menu "Options > Manage GSD files". Browse to
the directory with your GSD file. Mark the line with the file, and click
"Install".

In the project view, in the left menu select the PLC and the subitem "Device
configuration". Click the "Network view" tab. At the right edge open the
"Hardware catalog". Select "Other field devices" > Profinet IO > I/O > rt-labs >
rt-labs dev > "rt-labs DEMO device". Double-click it, and it will appear in the
main window.

Use the "Network view" tab, and right-click "Not assigned" on the rt-labs-dev
icon. Select "Assign to new IO-controller" and "PLC_1.PROFINETinterface_1".


Set IP addresses and connect physically
---------------------------------------
Connect to the Siemens PLC directly via an Ethernet cable to your laptop. For
the Simatic S7-1200 PLC you can use any of the two X1 RJ45 connectors. Your
laptop should have a manual IP address in the same subnet range as the PLC
(first three groups in IP address should be the same). Changing the subnet
mask size does not seems to have an impact.

In the project view, in the left menu select the PLC and the subitem "Device
configuration". In the "Properties" tab, use the "General" sub-tab. Select
"PROFINET interface [X1]" and "Ethernet addresses". Enter the existing IP
address of the PLC. The subnet mask should be 255.255.255.0 and subnet
"PN/IE_1". Right-click the icon of the PLC, and select "Go online". Use type of
interface "PN/IE", your Ethernet network card and "Direct at slot 1 X1". Click
"Start search". The table should be updated with "Device type" = "CPU 1215C
..."'if the connection is OK.

To enter the IP-address of the IO-device, go to "Device view" for the IO-device
and click the IO-device icon. in the "Properties" tab, select the "General"
sub-tab. Select
"PROFINET interface [X1]" and "Ethernet addresses". Enter the existing IP
address of the IO-device.


Add modules to IO-device
------------------------
In the device view, select the "rt-labs-dev" device (by using the dropdown).
Add a module to the "Device overview" tab. That is done by dragging a module
from the "Hardware catalog", which is located to the right of screen.
The modules are found in "Other field devices" > Profinet IO > I/O > rt-labs >
rt-labs dev > Module. Drop the module in the correct slot row in the "Device
overview table". Only modules that fit in the respective slot seems to
stick.


Run the application
-------------------
Connect one Ethernet cable between the PLC and the Windows laptop, and one
Ethernet cable between the PLC and the IO-device.

In the network view, right-click the PLC icon and select Compile > "Hardware
(rebuild all)". Repeat for Compile > "Software (rebuild all)". Then right-click
and select "Download to device" > "Hardware configuration". Repeat for "Download
to device" > "Software (all)". Click Load in the pop-up window, and then Finish.

In the main menu, use Online > "Go online". In the right part of the screen,
use "Online tools" to see the PLC LED states and to go to RUN and STOP modes.

There will be cyclic communication regardless whether the PLC is in RUN or
STOP mode, and the run state information is available in the cyclic data.

No PLC program is necessary for the cyclic communication to take place, but
the payload from the PLC is probably zeros.


Change IO-device station name
-----------------------------
In the network view, click the icon of the IO-device. Select and change the
name (on top row) in the icon. Do a download (hardware and software) to the
PLC. You can verify the result by looking at the PN-DCP frames in Wireshark.


Factory reset of Simatic ET200SP
--------------------------------
Use the mode switch on the front panel to do a factory reset. See the user
manual for details. This will reset also the IP address.

Connect the PLC to your laptop, and run Wireshark to figure out the IP address.
It is given inside the LLDP frame. Also the detailed model name, firmware
version etc are given in the LLDP frame.


Change IP address
-----------------
The IP address is changed via the left side menu "Online and diagnostics". Use
"Functions > Assign IP address". The MAC address is printed on the front of the
PLC. It is unclear how to actually do the change.

It should be possible to ping the PLC when the IP address is OK.


Connect inputs and outputs
--------------------------
First find the address of the IO-device input byte and output byte. In the
"Device view", look in the "Device overview" table. The module "8 bits I
8 bits O" should appear (if previously inserted). Look for the I (input)
address and Q (output) address. The value can be for example 2.

In the Project tree, select PLC_1 > PLC tags > Show all tags.
Create a new tag by clicking the first line and enter the name "ButtonIn".
Use "Default tag table" and DataType Bool. The address should be of operand
identifier "I" (input). Use address from the "Device view" as described above.
The bit number should be 7. This is written as "%I2.7".

Create a new tag "LEDout", also of Bool type. The bit number should be 7, so
the address should be for example "%Q2.7".

In order to study the values while running, you need to create an watch table.
In the Project tree, select PLC_1 > "Watch and force table" > "Add new watch
table". In the first empty line, double-click on the small icon on the Name field.
Select "button". Repeat on next line with "LEDout".

When running, in order to study the values, connect to the PLC ("Online"). On
the "Watch table_1" page, click the small "Monitor all" icon. The values on the
page will be continuously updated.


Enter PLC program
-----------------
In the project tree, under PLC1 > Program blocks > Add new block. In the pop-up
window, select "Function block" and language SCL. Give it the name "Flasher".
Click "OK".

Note that Structured Control Language (SCL) is Siemens name for Structured Text
(ST) programming language.

In the "Flasher [FB1]" window upper part, add an input pin in the "Input"
section. Name it "in_pin_button_LED", and give it the data type Bool. In the
"Output" section, add a "out_pin_LED" which also should be Bool. In the "Static"
section, add:

* in_pin_button_LED_previous: BOOL
* flashing: BOOL
* oscillator_state: BOOL
* oscillator_cycles: Int

.. highlight:: none

In the program part of the window, insert this::

   #oscillator_cycles := #oscillator_cycles + 1;
   IF #oscillator_cycles > 2000 THEN
      #oscillator_cycles := 0;
      #oscillator_state := NOT #oscillator_state;
   END_IF;

   IF #in_pin_button_LED = TRUE THEN
      IF #in_pin_button_LED_previous = FALSE THEN
         #flashing := NOT #flashing;
      END_IF;
      #out_pin_LED := TRUE;
   ELSIF #flashing = TRUE THEN
      #out_pin_LED := #oscillator_state;
   ELSE
      #out_pin_LED := FALSE;
   END_IF;
   #in_pin_button_LED_previous := #in_pin_button_LED;

In the "Main [OB1]" block, drag the "Empty box" icon to the "Network 1" line.
In the top of the inserted box, select "Flasher". In the pop up asking for
data block, select "Flasher_DB".

Connect the input on the "Flasher" block by double-clicking it. Select "button".
Similarly connect the output to "LEDout".

The block "Flasher_DB [DB1]" have been created automatically. All the input-,
output- and static variables should appear inside it.


Adjust data cycle time and watchdog
-----------------------------------
In the device view, select the IO-device in the dropdown. Use the "Properties"
tag and "General" subtab. Select "PROFINET interface [X1]" > Advanced options
> Real time settings. Adjust "Update time" in ms and the watchdog time (which
is the allowed number of cycles without data before an alarm is triggered).


Opening an archived project
----------------------------
Open an archived project by using the project view menu Project > Retrieve
and select the ``.zap15_1`` file. Create a new empty folder when asked for
target directory.


Alarm when IO-device is terminated
----------------------------------
Approximately 7-8 ms after the last cyclic data frame is received from the
IO-device, the Simatic PLC will send an alarm frame about missing data (if
using default values).

The Wireshark tool will display::

    Status: Error: "RTA error", "PNIO", "RTA_ERR_CLS_PROTOCOL", "AR consumer DHT/WDT expired (RTA_ERR_ABORT)"


Show connection errors to IO-device
-----------------------------------
If you are connected to the PLC ("online"), then it is possible to see if
there are communication problems to the IO-device. In the project tree > PLC_1
> Distributed I/O > Profinet IO-System > rt-labs-dev. In case of communication
errors, the hover text on the small icon is showing "Not reachable".
