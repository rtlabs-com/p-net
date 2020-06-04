Using Codesys soft PLC
======================
We run the Codesys ("COntroller DEvelopment SYStem") runtime on a Raspberry Pi,
and the setup is done using a Windows-based software (Codesys Development
System).


Download and installation of Codesys Development System on a Windows PC
-----------------------------------------------------------------------
The program can be downloaded from https://store.codesys.com/codesys.html
A trial version is available. Registration is required.

Download "CODESYS Development System V3". The file is named for example
"CODESYS 64 3.5.15.30.exe". Install it on a Windows machine by double clicking
the icon.

Also download "Codesys Control for Raspberry Pi".
Install it by double-clicking the ``.package`` file.

Restart the program after the installation.

If you use the trial version, you need to restart the Codesys runtime on the
Raspberry Pi every two hours.


Scan network to find Raspberry Pi, and install the Codesys runtime onto it
--------------------------------------------------------------------------
Make sure your Windows machine and the Raspberry Pi are connected to the
same local network.

In Codesys on Windows, use the menu Tools -> "Update Raspberry Pi".
Click "Scan" to find the IP address.

Click "Install" for the Codesys Runtime package. Use "Standard" runtime
in the pop-up window.


Create a project in Codesys
---------------------------
On the Windows machine, first create a suitable project area on your hard
drive. For example Documents/Codesys/Democontroller.

From the Codesys menu, create a new project. Use the recently created
directory, and "Standard Project". Name it "Democontroller".
Select device "Codesys Control for Raspberry Pi SL", and select to program in
"Structured Text (ST)"

It is important that you select the same version of the runtime (single core =
"SL" or multicore = "SL MC") both in the runtime and in the project, otherwise the
controller will not be found when you try to use it.

Verify that the "Device" in the left hand menu shows as "Codesys Control for
Raspberry Pi SL". Double-click the "Device". Click "Scan network" tab,
and select the Raspberry Pi. The marker on the image should turn green. Use
tab "Device" and "Send Echo Service" to verify the communication.

In the Codesys menu "Tools", select "Device Repository". Click "Install" and
select the GSDML file from your hard drive.

* On the "Device" in the left hand panel, right-click and select Add Device. Use "Ethernet".
* On the "Ethernet", right-click and select Add Device. Use "Profinet IO master".
* On the "PN_Controller", right-click and select Add Device. Use "rt-labs DEMO device".
* On the "rt_labs_DEMO_device", right-click and select Add Device. Use "8 bits I 8 bits 0".

Double-click the "Ethernet" node in the left menu. Select interface "eth0".
The IP address will be updated accordingly.

Double-click the "PN_controller" node in the left menu. Adjust the IP range
using "First IP" and "Last IP" to both have the existing IP-address of your
IO-device (for example a Linux laptop or embedded Linux board running the
sample_app).

Double-click the "rt_labs_DEMO_device" node in the left menu. Set the
IP-address to the existing address of your IO-device.


Structured Text programming language for PLCs
---------------------------------------------
Structured Text (ST) is a text based programming language for PLCs.
Read about it on https://en.wikipedia.org/wiki/Structured_text

A tutorial is found here: https://www.plcacademy.com/structured-text-tutorial/


Create controller application
-----------------------------
Enter program in "PLC_PRG".

Variables section::

    PROGRAM PLC_PRG
    VAR
        in_pin_button_LED: BOOL;
        out_pin_LED: BOOL;

        in_pin_button_LED_previous: BOOL;
        flashing: BOOL;
        oscillator_state: BOOL := FALSE;
        oscillator_cycles: UINT := 0;
    END_VAR

Program section::

    oscillator_cycles := oscillator_cycles + 1;
    IF oscillator_cycles > 200 THEN
        oscillator_cycles := 0;
        oscillator_state := NOT oscillator_state;
    END_IF

    IF in_pin_button_LED = TRUE THEN
        IF in_pin_button_LED_previous = FALSE THEN
            flashing := NOT flashing;
        END_IF
        out_pin_LED := TRUE;
    ELSIF flashing = TRUE THEN
        out_pin_LED := oscillator_state;
    ELSE
        out_pin_LED := FALSE;
    END_IF
    in_pin_button_LED_previous := in_pin_button_LED;

On the "_8_bits_I_8_bits_O" node, right-click and select "Edit IO mapping".
Double-click the row that you would like the edit.
Map "Input Bit 7" to "in_pin_button_LED", and "Output Bit 7" to "out_pin_LED"

In the "Application -> MainTask" select "Cyclic" with 4 ms.

In the "Application -> Profinet_CommunicationTask" select "Cyclic" with 10 ms.
Use priority 30.


Transfer controller application to (controller) Raspberry Pi
------------------------------------------------------------

* In the top menu, use Build -> Build.
* Transfer the application to the Raspberry Pi by using the top menu
  Online -> Login. Press "Yes" in the pop-up window.
* In the top menu, use Debug -> Start

You can follow the controller log by using the top menu Tools -> "Update
Raspberry Pi". Click the "System info" button, and look in the "Runtime Info"
text box. It will show an error message if it can't find the IO-device on
the network.

Use Wireshark to verify that the controller sends LLDP packets every 5 seconds.
Every 15 seconds it will send an ARP packet to ask for the (first?) IO-device
IP address, and a PN-DCP packet to ask for the IO-device with the name
"rt-labs-dev".


Running the application
-----------------------
See the "Tutorial" page.


Adjust PLC timing settings
--------------------------
It is possible to adjust the cycle time that the IO-controller (PLC) is using
for cyclic data communication with an IO-device.

In the left menu, double-click the "rt_labs_DEMO_device", and open the "General"
tab. The "Communication" section shows the send clock in milliseconds, as read
from the GSDML file. By using the "Reduction ratio" you can slow down the
communication, by multiplying the cycle time by the factor given in the
dropdown.

It is also possible to increase the watchdog time, after which the PLC will set
an alarm for missing incoming cyclic data. The watchdog will also shut down the
communication, and trigger a subsequent restart of communication.

In case of problems, increase the reduction ratio (and timeout) value a lot,
and then gradually reduce it to find the smallest usable value.


Writing and reading IO-device parameters
----------------------------------------
The parameters for different submodules are written as part of normal startup.

To manually trigger parameter sending via Codesys, double-click the
_8_bits_I_8_bits_O device in the left menu. Use the General tab, and the
Settings part of the page. Right-click the parameter you would like to send,
and select "Write to device".

It is also possible to read a parameter similarly.

When clicking the "Write All Values" icon, one write request is sent per
parameter.

In order to change a parameter value in the Codesys GUI, you need to first go
offline.


Setting output producer status (IOPS)
-------------------------------------
Normally Codesys will set the Output PS to GOOD (0x80 = 128) when running.
Clicking the "Output PS" checkbox on the "IOxS" tab on the Profinet IO-device
sets the value to BAD (0).


Displaying errors
-----------------
Click on the IO-device in the tree structure in the left part of the screen.
Use the "Log" tab to diplay errors.


Displaying alarms
-----------------
Alarms are displayed on the "Status" tabs for modules. Also the IO-device
itself has a "Status" tab with alarms related to built-in (DAP) modules.


Connection status
-----------------
Go to the Profinet IO-device page, and see the "PNIO IEC objects" tab. Expand
the topmost row. The states of these boolean fields are shown:

* xRunning: Periodic data is sent
* xBusy: The controller is trying to connect to the IO-device
* xError: Failure to connect to the IO-device

If there is no connection at all to the IO-device, the state will shift to
xBusy from xError once every 5 seconds.


Change IP address of IO-device
------------------------------
Change the IP address by double-clicking the "rt_labs_DEMO_device" node
in the left menu, using the "General" tab. Set the IP-address to new value.

The IO-controller will send the new IP address in a "DCP Set" message to the
IO-device having the given station name. Then it will use ARP messages to
the IO-device to find its MAC address, and to detect IP address collisions.


Scan for devices, assign IP address, reset devices and change station name
--------------------------------------------------------------------------
In the left side menu, right-click the PN_Controller and select "Scan for
devices". The running IO-devices will show up, and it is possible to see which
modules are plugged into which slot.

This is implemented in Codesys by sending the "Ident request, all" DCP
message from the IO-controller.
It works also if there are no IO-devices loaded in the left side hierarchy menu.
An IO-device will respond with the "Ident OK" DCP message. Then the IO-controller
will do a "Read implicit request" for "APIData", on which the IO device
responds with the APIs it supports. A similar request for
"RealIdentificationData for one API" is done by the IO-controller, on which the
IO-device responds which modules (and submodules) are plugged into which slots
(and subslots).

To factory reset a device, select it in the list of scanned devices and click
the "reset" button.

At a factory reset, the IO-controller sends a "Set request" DCP message
with suboption "Reset factory settings". After sending a response, the
IO-device will do the factory reset and also send a LLDP message with the
new values. Then the IO-controller sends a "Ident request, all", to which
the IO-device responds.

To modify the station name or IP address, change the corresponding fields
in the list of scanned devices, and the click "Set name and IP".

The IO-controller sends a "Set request" DCP message
with suboptions "Name of station" and "IP parameter". After sending a
response, the IO-device will change IP address and station name. It will
also send a LLDP message with the new values. Then the
IO-controller sends a "Ident request, all", to which the IO-device responds.

There is functionality to flash a LED on an IO-device. Select your device in
the list of scanned devices, and click the "Blink LED" button. The button
remains activated until you click it again.

LED-blinking is done by sending the "Set request" DCP message with suboption
"Signal" once every 5 seconds as long as the corresponding button is activated.

In order the read Identification & Maintenance (I&M) data, the device needs to
be present as an IO-device connected to the IO-controller in the left side menu.
Select the device in the list of scanned devices, and click the "I&M" button.

Reading I&M data is done by the IO-controller by sending four "Read implicit"
request DCP messages, one for each of I&M0 to I&M3.
