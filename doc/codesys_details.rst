Codesys Details
===============
This page contains additional information on how to use the Codesys soft PLC.


Adjust PLC timing settings
--------------------------
It is possible to adjust the cycle time that the IO-controller (PLC) is using
for cyclic data communication with an IO-device.

In the left menu, double-click the "P_Net_Sample_App", and open the "General"
tab. The "Communication" section shows the send clock in milliseconds, as read
from the GSDML file. By using the "Reduction ratio" you can slow down the
communication, by multiplying the cycle time by the factor given in the
dropdown.

It is also possible to increase the watchdog time, after which the PLC will set
an alarm for missing incoming cyclic data. The watchdog will also shut down the
communication, and trigger a subsequent restart of communication.

In case of problems, increase the reduction ratio (and timeout) value a lot,
and then gradually reduce it to find the smallest usable value.


Displaying errors
-----------------
Click on the IO-device in the tree structure in the left part of the screen.
Use the "Log" tab to display errors.


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
Change the IP address by double-clicking the "P_Net_Sample_App" node
in the left menu, using the "General" tab. Set the IP-address to new value.

The IO-controller will send the new IP address in a "DCP Set" message to the
IO-device having the given station name. Then it will use ARP messages to
the IO-device to find its MAC address, and to detect IP address collisions.


Scan for devices, assign IP address, reset devices and change station name
--------------------------------------------------------------------------

Scan
^^^^
In the left side menu, right-click the PN_Controller and select "Scan for
devices". The running IO-devices will show up, and it is possible to see which
modules are plugged into which slot.
Note that the texts in the columns "Device name" and "Device type" are from the
available GSDML files in Codesys. If a corresponding GSDML file not has been
loaded, the text "Attention: The device was not found in the repository" is
shown together with the Vendor-ID and Product-ID.

This is implemented in Codesys by sending the "Ident request, all" DCP
message from the IO-controller.
It works also if there are no IO-devices loaded in the left side hierarchy menu.
An IO-device will respond with the "Ident OK" DCP message. Then the IO-controller
will do a "Read implicit request" for "APIData", on which the IO device
responds with the APIs it supports. A similar request for
"RealIdentificationData for one API" is done by the IO-controller, on which the
IO-device responds which modules (and submodules) are plugged into which slots
(and subslots).

Factory reset
^^^^^^^^^^^^^
To factory reset a device, select it in the list of scanned devices and click
the "reset" button.

At a factory reset, the IO-controller sends a "Set request" DCP message
with suboption "Reset factory settings". After sending a response, the
IO-device will do the factory reset and also send a LLDP message with the
new values. Then the IO-controller sends a "Ident request, all", to which
the IO-device responds.

Set name and IP
^^^^^^^^^^^^^^^
To modify the station name or IP address, change the corresponding fields
in the list of scanned devices, and the click "Set name and IP".

The IO-controller sends a "Set request" DCP message
with suboptions "Name of station" and "IP parameter". After sending a
response, the IO-device will change IP address and station name. It will
also send a LLDP message with the new values. Then the
IO-controller sends a "Ident request, all", to which the IO-device responds.

Flash LED
^^^^^^^^^
There is functionality to flash a LED on an IO-device. Select your device in
the list of scanned devices, and click the "Blink LED" button. The button
remains activated until you click it again.

LED-blinking is done by sending the "Set request" DCP message with suboption
"Signal" once every 5 seconds as long as the corresponding button is activated.

Identification & Maintenance (I&M) data
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In order the read Identification & Maintenance (I&M) data, the device needs to
be present as an IO-device connected to the IO-controller in the left side menu.
Select the device in the list of scanned devices, and click the "I&M" button.

Reading I&M data is done by the IO-controller by sending four "Read implicit"
request DCP messages, one for each of I&M0 to I&M3.

When writing I&M data from Codesys, it will send a connect, write and release.


Writing and reading IO-device parameters
----------------------------------------
The parameters for different submodules are written as part of normal startup.

To adjust the values sent at startup, you must first be offline.
Then double-click the DIO_8xLogicLevel device in the left menu. Use the
General tab, and the Settings part of the page. Click the corresponding
value in the table, and adjust it.

While offline (with regards to the PLC, it is possible to manually trigger
parameter reading and writing. Right-click the parameter you would like
to send, and select  "Write to device". It is also possible to read a
parameter similarly.

To read all or write all parameters, use the corresponding buttons.
When clicking the "Write All Values" icon, one write request is sent per
parameter.


Forcing cyclic data values
--------------------------
To force an output value from the PLC, open the PLC_PRG window while online.
In the table on the top of the page, on the line you would like to force click the "Prepared value"
until it shows "TRUE". Use the line "out_pin_LED" if running the sample application.
Right-click on the line, and select "Force all values ...". The new value is sent from the PLC to the IO-device.
A red `F` symbol is shown.
To stop forcing the value, right-click it and select "Unforce all values ...".


Displaying alarms sent from IO-device
-------------------------------------
Incoming process alarms and diagnosis alarms appear on multiple places in
the Codesys desktop application.

* Codesys Raspberry Pi: The "Log" tab show process and diagnosis alarms.
* PN_Controller: The "Log" tab show process and diagnosis alarms.
* IO-device: The "Log" tab show process and diagnosis alarms. The
  “Status” tab shows alarms related to built-in (DAP) modules.
* Plugged module in IO-device: Process and diagnosis alarms are displayed on
  the “Status” tab.


Setting output producer status (IOPS)
-------------------------------------
Normally Codesys will set the Output PS to GOOD (0x80 = 128) when running.
Clicking the "Output PS" checkbox on the "IOxS" tab on the Profinet IO-device
sets the value to BAD (0).


Enabling checking of peer stationname and port ID
-------------------------------------------------
It is possible to have the IO-device verify that it is connected to the
correct neighbour (peer) by checking its station name and port ID (as sent
in LLDP frames by the neighbour).

Double-click the “P_Net_Sample_App” node in the left menu. On the "options"
tab in the resulting window, use the fields "Peer station" and "port". It
seems only possible to select station names from other devices or controllers
already available in the project.

During startup the PLC will send the given values to the IO-Device via a
write command. If the correct neighbour is not present, an alarm will be sent
by the IO-device to the PLC.


Writing PLC programs
--------------------
Documentation of available function blocks is found at
https://help.codesys.com/webapp/_pnio_f_profinet_io_configuration;product=core_ProfinetIO_Configuration_Editor


Using the Echo module
---------------------
The echo module will receive an integer and a float from the PLC, and multiply them with a constant
value before sending them back to the PLC. The multiplier is module parameter, and can be adjusted
at startup. The integer is an unsigned 32 bit integer, and the float is a single precision float
(32 bits).

To test it, unplug any existing modules, and plug one Echo module into slot 1.

Enter this program::

   PROGRAM PLC_PRG
   VAR
      out_echo_int: UDINT;
      out_echo_float: REAL;
      in_echo_int: UDINT;
      in_echo_float: REAL;
      temp_int: UDINT;
      temp_float: REAL;
   END_VAR

   out_echo_int := 1000001;
   out_echo_float := 1001.23456;

   temp_int := in_echo_int;
   temp_float := in_echo_float;

On the Echo_module page, use the "PNIO Module I/O Mapping" to connect the
four program variables to the corresponding channels. Connect "in_echo_float" to
"Input float to controller" etc.

Start the PLC, and go online to follow the values. You can force (described elsewhere)
the ``out_echo_int`` and ``out_echo_float`` values to study how the resulting
input values from the IO-device changes.
