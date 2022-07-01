.. _using-codesys:

Using Codesys soft PLC
======================
We run the Codesys ("COntroller DEvelopment SYStem") runtime on a Raspberry Pi,
and the setup is done using a Windows-based software (Codesys Development
System). Your Windows laptop is used to make changes to the PLC (IO-controller)
settings, but it does not act as an IO-controller.


Download and installation of Codesys Development System on a Windows PC
-----------------------------------------------------------------------
The program can be downloaded from https://store.codesys.com/codesys.html
A trial version is available. Registration is required.

Download “CODESYS Development System V3", in the latest available version.
The file is named :file:`CODESYS 64 <VERSION>.exe`. Install it on a Windows
machine by double clicking the icon.

From within Codesys, install the "Codesys control for Raspberry Pi"
by using the menu Tools/"Codesys installer". In the "AddOns" section
use "Browse" and search for "raspberry". Select the relevant row in the
results, and click "Install". When completed, there should be an entry
"Update Raspberry Pi" available in the Tools menu.

Restart the program after the installation.


Scan network to find your PLC Raspberry Pi, and install the Codesys runtime onto it
-----------------------------------------------------------------------------------
Make sure your Windows machine and the Raspberry Pi are connected to the
same local network.

In Codesys on Windows, use the menu Tools -> "Update Raspberry Pi".
Click "Scan" to find the IP address.
In this tutorial the IP address of the PLC Raspberry Pi is ``192.168.0.100``.

It is possible to install the runtime on the PLC Raspberry Pi as long as you
know its IP address, even if it doesn't show up during the scan.

Click "Install" for the Codesys Runtime package. Use "Standard" runtime
in the pop-up window.

The runtime will be installed in the :file:`/opt/codesys` directory on the Raspberry Pi.


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
For the sample application the GSDML file is available in the
:file:`samples/pn_dev` folder in the repository that you have cloned, or
from https://github.com/rtlabs-com/p-net/tree/master/samples/pn_dev

* On the “Device (CODESY Control for Raspberry Pi SL)” in the left hand panel,
  right-click and select Add Device. Use “Ethernet adapter”, “Ethernet”.
* On the "Ethernet", right-click and select Add Device.
  Use "Profinet IO master", "PN-Controller".
* On the "PN_Controller", right-click and select Add Device. Use "P-Net Sample App".
* On the "P_Net_Sample_App", right-click and select Add Device. Use "DIO 8xLogicLevel".

Double-click the "Ethernet" node in the left menu. Select interface "eth0".
The IP address will be updated accordingly.

Double-click the "PN_controller" node in the left menu. Adjust the IP range
using "First IP" and "Last IP" to both have the existing IP-address of your
IO-device (for example a Linux laptop or embedded Linux board running the
sample_app). For this tutorial we use the “First IP” ``192.168.0.50``
and also the “Last IP” ``192.168.0.50``.

Double-click the "P_Net_Sample_App" node in the left menu. Set the
IP-address to the existing address of your IO-device.
In this tutorial we use ``192.168.0.50``.


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
        flashing: BOOL := TRUE;
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

On the "DIO_8xLogicLevel" node in the left-side menu,
right-click and select "Edit IO mapping".
Open the "Input 8 bits" row by clicking the small ``+`` sign.
Double-click the icon on the row that you would like the edit.
Map "Input Bit 7" to "in_pin_button_LED" (found via Application/PLC_PRG),
and "Output Bit 7" to "out_pin_LED".

In the "Application -> MainTask" select "Cyclic" with 4 ms.

In the "Application -> Profinet_CommunicationTask" select "Cyclic" with 10 ms.
Use priority 14.


Transfer controller application to (controller) Raspberry Pi
------------------------------------------------------------

* In the top menu, use Build -> "Generate Code".
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
Now it is time to run the application, so head back to the :ref:`tutorial` page.

Once the Codesys softplc running on the Raspberry Pi has been configured,
you can turn off the personal computer (running the Codesys desktop application)
used to configure it.
Remember that you need to power cycle the Raspberry Pi running the softplc every
two hours, if using the trial version.
