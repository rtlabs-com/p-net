Sample app details
==================

Cyclic data timing
------------------
The sample application GSDML file has the element ``<TimingProperties>``, which
has an attribute ``SendClock="32"`` in units of 1/32 milliseconds.
It corresponds to 1 ms, and thus the IO-device will send cyclic
data every millisecond. Note that the sample application does not read
the GSDML file, the file just describes the hard coded value used
in the sample application.

The sample application timer uses ``APP_TICK_INTERVAL_US`` which is
1000 microseconds = 1 millisecond. To be able to tell this information to
others, the value ``32`` is given in the ``min_device_interval`` field of the
device configuration.

In the PLC you can set the reduction ratio to slow down the data exchange,
and this information is sent to the IO-device on startup.
For example a reduction ratio of 4 will cause the cyclic data to
be sent with 4 times longer interval (i.e. every 4 ms) from the IO-device.


Using more modules
------------------
If necessary, increase the values for ``PNET_MAX_SLOTS`` and
``PNET_MAX_SUBSLOTS`` in ``include/pnet_api.h``.

In the sample app, the input data is written to all input modules ("8 bit in +
8 bit out" and "8 bit in"). The LED is controlled by the output module ("8 bit
in + 8 bit out" or "8 bit out") with lowest slot number.

The alarm triggered by button 2 is sent from the input module with lowest slot
number (if any).

In order to handle larger incoming DCE/RPC messages (split in several frames),
you might need to increase PNET_MAX_SESSION_BUFFER_SIZE. Note that this will
increase the memory consumption.


Sample app data payload
-----------------------
This example is for when using the “8 bit in + 8 bit out” module in slot 1.
The periodic data sent from the sample application IO-device to IO-controller
is one byte:

* Lowest 7 bits: A counter which has its value updated every 10 ms
* Most significant bit: Button1

When looking in Wireshark, look at the "Profinet IO Cyclic Service Data Unit",
which is 40 bytes. The relevant byte it the fourth byte from left in this
block.

Details of the 40 bytes from IO-device to IO-controller:

* IOPS from slot 0, subslot 1
* IOPS from slot 0, subslot 0x8000
* IOPS from slot 0, subslot 0x8001
* IO data from slot 1, subslot 1 (input part of the module)
* IOPS from slot 1, subslot 1 (input part of the module)
* IOCS from slot 1, subslot 1 (output part of the module)
* (Then 34 bytes of padding, which is 0)

From the IO-controller to IO-device is one data byte sent:

* Most significant bit: LED

When looking in Wireshark, look at the "Profinet IO Cyclic Service Data Unit",
which is 40 bytes. The relevant byte is the fifth byte from left in this
block. The value toggles between 0x80 and 0x00.

Details of the 40 bytes from IO-controller to IO-device:

* IOCS to slot 0, subslot 1
* IOCS to slot 0, subslot 0x8000
* IOCS to slot 0, subslot 0x8001
* IOCS to slot 1, subslot 1 (input part of the module)
* IO data to slot 1, subslot 1 (output part of the module)
* IOPS to slot 1, subslot 1 (output part of the module)
* (Then 34 bytes of padding, which is 0)

Note that Wireshark can help parsing the cyclic data by using the GSDML file.
See the Wireshark page in this document.


Ethernet frames sent during start-up
------------------------------------
For this example, the IO-controller is started first, and then the IO-device.

+---------------------------+----------+----------------------------------------------------------------------------------------------------------------------+
| Sender                    | Protocol | Content                                                                                                              |
+===========================+==========+======================================================================================================================+
| IO-controller (broadcast) | LLDP     | Name, MAC, IP address, port name (sent every 5 seconds)                                                              |
+---------------------------+----------+----------------------------------------------------------------------------------------------------------------------+
| IO-controller (broadcast) | PN-DCP   | "Ident req". Looking for "rt-labs-dev" (sent every 2.5 seconds)                                                      |
+---------------------------+----------+----------------------------------------------------------------------------------------------------------------------+
| IO-controller (broadcast) | ARP      | Is someone else using my IP?                                                                                         |
+---------------------------+----------+----------------------------------------------------------------------------------------------------------------------+
| IO-device (broadcast)     | LLDP     | Name, MAC, IP address, port name (sent every 5 seconds??)                                                            |
+---------------------------+----------+----------------------------------------------------------------------------------------------------------------------+
| IO-device (broadcast)     | PN-DCP   | "Hello req". Station name "rt-labs-dev", IP address, gateway, vendor, device (sent every 3 seconds)                  |
+---------------------------+----------+----------------------------------------------------------------------------------------------------------------------+
| IO-device                 | PN-DCP   | "Ident OK" Identify response. Station name "rt-labs-dev", IP address, netmask, gateway, VendorID, DeviceID, options  |
+---------------------------+----------+----------------------------------------------------------------------------------------------------------------------+
| IO-controller             | PN-DCP   | "Set Req" Set IP request. Use IP address and gateway.                                                                |
+---------------------------+----------+----------------------------------------------------------------------------------------------------------------------+
| IO-device                 | PN-DCP   | "Set OK" Status.                                                                                                     |
+---------------------------+----------+----------------------------------------------------------------------------------------------------------------------+
| IO-controller (broadcast) | ARP      | Who has <IO-device IP address>?                                                                                      |
+---------------------------+----------+----------------------------------------------------------------------------------------------------------------------+
| IO-device                 | ARP      | IP <IO-device IP address> is at <IO-device MAC address>                                                              |
+---------------------------+----------+----------------------------------------------------------------------------------------------------------------------+
| IO-controller             | PNIO-CM  | "Connect request" Controller MAC, timeout, input + output data (CR), modules + submodules in slots                   |
+---------------------------+----------+----------------------------------------------------------------------------------------------------------------------+
| IO-device                 | PNIO-CM  | "Connect response" MAC address, UDP port, input + output + alarm CR, station name                                    |
+---------------------------+----------+----------------------------------------------------------------------------------------------------------------------+
| IO-device                 | PNIO-PS  | FrameID 0x8001. Cycle counter, provider stopped. 40 bytes data.                                                      |
+---------------------------+----------+----------------------------------------------------------------------------------------------------------------------+
| IO-controller             | PNIO-PS  | FrameID 0x8000. Cycle counter, provider running. 40 bytes data.                                                      |
+---------------------------+----------+----------------------------------------------------------------------------------------------------------------------+
| IO-controller             | PNIO-CM  | "Write request" API, slot, subslot, data.                                                                            |
+---------------------------+----------+----------------------------------------------------------------------------------------------------------------------+
| IO-device                 | PNIO-CM  | "Write response" API, slot, subslot, status.                                                                         |
+---------------------------+----------+----------------------------------------------------------------------------------------------------------------------+
| IO-controller             | PNIO-CM  | "Control request" (DControl). Command: ParameterEnd.                                                                 |
+---------------------------+----------+----------------------------------------------------------------------------------------------------------------------+
| IO-device                 | PNIO-CM  | "Control response" Command: Done                                                                                     |
+---------------------------+----------+----------------------------------------------------------------------------------------------------------------------+
| IO-device                 | PNIO-PS  | FrameID 0x8001. Cycle counter, provider running. 40 bytes data.                                                      |
+---------------------------+----------+----------------------------------------------------------------------------------------------------------------------+
| IO-device                 | PNIO-CM  | "Control request" (CControl). Command: ApplicationReady                                                              |
+---------------------------+----------+----------------------------------------------------------------------------------------------------------------------+
| IO-controller             | PNIO-CM  | "Control response" Command: ApplicationReadyDone                                                                     |
+---------------------------+----------+----------------------------------------------------------------------------------------------------------------------+

The order of the PNIO-PS frames is somewhat random in relation to PNIO-CM frames.

+------------+---------------------------------+
| Protocol   | Description                     |
+============+=================================+
| LLDP       |                                 |
+------------+---------------------------------+
| ARP        |                                 |
+------------+---------------------------------+
| PN-DCP     | Acyclic real-time data          |
+------------+---------------------------------+
| PNIO-PS    | Cyclic real-time data           |
+------------+---------------------------------+
| PNIO-AL    | Acyclic real-time alarm         |
+------------+---------------------------------+
| PNIO-CM    | UDP, port 34964 = 0x8892        |
+------------+---------------------------------+


Ethernet frames sent at alarm
-----------------------------
Frames sent when pressing button 2.

+---------------+----------+----------------------------------------------------------------------------------------+
| Sender        | Protocol | Content                                                                                |
+===============+==========+========================================================================================+
| IO-device     | PN-AL    | Alarm notification high, slot, subslot, module, submodule, sequence, 1 byte user data  |
+---------------+----------+----------------------------------------------------------------------------------------+
| IO-controller | PN-AL    | ACK-RTA-PDU                                                                            |
+---------------+----------+----------------------------------------------------------------------------------------+
| IO-controller | PN-AL    | Alarm ack high, alarm type Process, slot, subslot, Status OK                           |
+---------------+----------+----------------------------------------------------------------------------------------+
| IO-device     | PN-AL    | ACK-RTA-PDU                                                                            |
+---------------+----------+----------------------------------------------------------------------------------------+


Cyclic data for the different slots
-----------------------------------
This is an example if you populate slot 1 to 3 with different modules.

+------+---------+--------------------------------------------+-----------------------------------------+--------------------------------------------+
| Slot | Subslot | Description                                | | Contents of Input CR                  | | Contents of Output CR                    |
|      |         |                                            | | (to IO-controller)                    | | (from IO-controller)                     |
+======+=========+============================================+=========================================+============================================+
| 0    | 1       | The IO-Device itself                       | (data) + IOPS                           | IOCS                                       |
+------+---------+--------------------------------------------+-----------------------------------------+--------------------------------------------+
| 0    | 0x8000  | Interface 1                                | (data) + IOPS                           | IOCS                                       |
+------+---------+--------------------------------------------+-----------------------------------------+--------------------------------------------+
| 0    | 0x8001  | Port 0 of interface 1                      | (data) + IOPS                           | IOCS                                       |
+------+---------+--------------------------------------------+-----------------------------------------+--------------------------------------------+
| 1    | 1       | Input part of "8-bit in 8-bit out" module  | data (1 byte) + IOPS                    | IOCS                                       |
|      |         +--------------------------------------------+-----------------------------------------+--------------------------------------------+
|      |         | Output part of "8-bit in 8-bit out" module | IOCS                                    | data (1 byte) + IOPS                       |
+------+---------+--------------------------------------------+-----------------------------------------+--------------------------------------------+
| 2    | 1       | "8-bit in" module                          | data (1 byte) + IOPS                    | IOCS                                       |
+------+---------+--------------------------------------------+-----------------------------------------+--------------------------------------------+
| 3    | 1       | "8-bit out" module                         | IOCS                                    | data (1 byte) + IOPS                       |
+------+---------+--------------------------------------------+-----------------------------------------+--------------------------------------------+

Note that the submodules (in subslots) in slot 0 do not send any cyclic data,
but they behave as inputs (as they send cyclic IOPS).

Output (from PLC)::

   +----------------+                +-----------+
   | IO-device      |                | PLC       |
   |  +----------+  |    DATA        |           |
   |  |          |  |  <-----------  |           |
   |  |  CPM     |  |                |           |
   |  |          |  |    IOPS        |           |
   |  |          |  |  <-----------  |           |
   |  +----------+  |                |           |
   |                |                |           |
   |  +----------+  |                |           |
   |  |          |  |                |           |
   |  |  PPM     |  |                |           |
   |  |          |  |    IOCS        |           |
   |  |          |  |  ----------->  |           |
   |  +----------+  |                |           |
   |                |                |           |
   +----------------+                +-----------+


Input (to PLC)::

   +----------------+                +-----------+
   | IO-device      |                | PLC       |
   |  +----------+  |    IOCS        |           |
   |  |          |  |  <-----------  |           |
   |  |  CPM     |  |                |           |
   |  |          |  |                |           |
   |  |          |  |                |           |
   |  +----------+  |                |           |
   |                |                |           |
   |  +----------+  |    DATA        |           |
   |  |          |  |  ----------->  |           |
   |  |  PPM     |  |                |           |
   |  |          |  |    IOPS        |           |
   |  |          |  |  ----------->  |           |
   |  +----------+  |                |           |
   |                |                |           |
   +----------------+                +-----------+
