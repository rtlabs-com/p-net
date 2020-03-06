Real-time properties of Linux
=============================
Regular Linux is not a real-time operating system. Profinet have rather strict
timing constraints. For example a Simatic PLC (with default settings) will
send an alarm if no incoming Profinet frame is received for 7-8 ms. So it is
important that Linux is responsive enough.

There are a few methods that can be used to improve the Linux responsiveness.
The most important is to use FIFO scheduling and to isolate the application on
a separate CPU core.


Use USE_SCHED_FIFO option
-------------------------
Select the FIFO linux kernel scheduling option.  This is done by passing
``-DUSE_SCHED_FIFO=ON`` command line argument to cmake.


Run the application on a separate processor core
------------------------------------------------
It is possible to tell the Linux kernel not to put any processes on a specific
processor core. This assumes that you have more than one core in your CPU.
Check it using::

   cat /proc/cpuinfo

By setting the ``isolcpus=2`` Linux boot command, the kernel will not put any
processes on CPU core number 2 automatically. This option is typically set from
the boot loader.

The best way to check which CPU cores that are currently isolated::

   cat /sys/devices/system/cpu/isolated

To see which kernel command line options that have been used::

   cat /proc/cmdline

Put your Profinet application on the isolated CPU core. It is done using::

   taskset -c 2 pn_dev

where ``-c 2`` tells which CPU core to use.


Real-time patches
-----------------
By applying the real-time patches (PREEMPT_RT) the real-time properties can
be improved.

For more details, see:

* https://wiki.linuxfoundation.org/realtime/start
* https://rt.wiki.kernel.org/index.php/Main_Page

It is important to write you application so that it can use the benefits of
the real-time patches. This includes running in a separate thread and setting
the priorities properly.

For the real-time patches to have an effect on p-net, set the ``USE_SCHED_FIFO``
cmake option.


Increase application cycle time
-------------------------------
For testing, you can increase the cycle time from the PLC in order to reduce
the time-out problems. Also the allowed number of missed frames can be
increased in the PLC settings.
