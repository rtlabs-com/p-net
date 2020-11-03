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


Required features of the hardware
---------------------------------

* It should be possible to send and receive raw (layer 2) Ethernet frames
* It should be possible to store data between runs, in a file system or some nonvolatile memory
