Creating applications and porting to new hardware
=================================================

Create your own application
---------------------------
If you prefer not to implement some of the callbacks, set the corresponding
fields in the configuration struct to NULL instead of a function pointer.
See the API documentation on which callbacks that are optional.

To read output data from the PLC, use ``pnet_output_get_data_and_iops()``.
To write data to the PLC (which is input data in Profinet terminology), use
``pnet_input_set_data_and_iops()``.


Adapt to other hardware
-----------------------
You need to adapt the implementation to your specific hardware.

This includes for example:

* For Linux, adjust the networking and LED scripts in PNAL
* Implement the callback to control the "Signal" LED.


Required features of the hardware or operating system
-----------------------------------------------------

* It should be possible to send and receive raw (layer 2) Ethernet frames
* It should be possible to store data between runs, in a file system or some nonvolatile memory
* For conformance class B, there needs to be an SNMP implementation that
  the p-net stack can use.


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


Interacting with other parts of your application software
---------------------------------------------------------
When the PLC sends a "factory reset" command, not only the p-net stack but
also the rest of your application should be reset to factory settings.

If there is a method to do a factory reset of your application, it should also
reset the p-net stack.

It is possible to set the IP-address, netmask and gateway address via Profinet
from a PLC. Consider how this affects other aspects of your application.

Serial number, software version, model name etc should be available for the
p-net stack to use.


Minimum cycle time for your application and hardware
----------------------------------------------------
At PLC configuration, there are two adjustable parameters regarding the
cyclic data timing for an IO-device. One basically controls the
frame-to-frame transmission interval (given as a multiple of the cycle time),
and the other one controls the PLC watchdog timeout setting.
The watchdog triggers an alarm when a number of consecutive cyclic data
frames from the IO-device are missing.

During conformance testing, the PLC watchdog timeout setting is 3 x
frame-to-frame interval times, ie the PLC will trigger an alarm if three
consecutive cyclic data frames are missing. Thus the smallest allowed
frame-to-frame interval given in the GSDML file must take the smallest
useful PLC watchdog timeout setting into account.

There can be different factors limiting the ability to send Profinet frames at
a regular pace:

* The initial work at Profinet communication startup should not cause
  delays in the cyclic data transmission once started.
* The hardware needs to keep up with the steady-state sending and receiving
  of Profinet cyclic data.
* Other work done by the Profinet application, for example work done in
  callbacks, might slow down the communication.
* Operating system jitter, which basically is that the operating system is
  busy doing other work, will cause frame delays.
* Using a network interface connected via USB can cause additional frame
  delays.

For real-time operating systems (RTOS) for example rt-kernel, the
frame-to-frame transmission will have little jitter.

For non-real-time operating systems, for example Linux, the limiting factor
is typically the jitter caused by the operating system.
See the page on Linux timing in this documentation for ideas on improving
Linux real-time properties.


Measure Profinet frame transmission jitter
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Set the PLC watchdog time to 100 x frame interval, to avoid any alarms
from the PLC.
Use Wireshark to record cyclic data transmission for a few minutes.
Plot the maximum frame-to-frame transmission interval (see the page on
capturing Ethernet packets in this documentation).
This will show which PLC watchdog setting you need in order not to trigger
"missing frame" alarms.

For example if the transmission interval is 1 ms but there sometimes is a 10 ms
delay between the frames, then you need to use the next standard PLC timeout
value which is 12 ms. In order to handle that with a watchdog setting of 3 x
frame interval, you need to use a minimum cycle time of 4 ms (as
will be stated in your GSDML file).


Example measurement results
^^^^^^^^^^^^^^^^^^^^^^^^^^^
Measurements with p-net on a XMC4800 board running rt-kernel shows that the
hardware is able to send frames with a periodicity of 1 ms with little jitter.
The liming factor is that the PLC watchdog timeout value needs to be 12 ms,
due to the time it takes for the first cyclic data frame to be sent from
the IO-device.
A GSDML file should state that the smallest allowed cycle time is 4 ms.

Similar measurements on a Raspberry Pi 3B+ show that it is possible to send
frames with a cycle time of 1 ms. However there is sometimes a frame-to-frame
interval of up to 40 ms, resulting in that a GSDML file should state that the
minimum frame interval is 16 ms.
