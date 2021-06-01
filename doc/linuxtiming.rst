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
Select the FIFO Linux kernel scheduling option. This is done by passing
``-DUSE_SCHED_FIFO=ON`` command line argument to cmake.

Display real time properties of a process (should typically be ``SCHED_FIFO``
for best result)::

   pi@pndevice-pi:~$ chrt -p $(pidof pn_dev)
   pid 438's current scheduling policy: SCHED_OTHER
   pid 438's current scheduling priority: 0

To show all threads, with scheduling mechanism::

   ps -efLc

or for the sample application::

   ps -efLc | grep pn_dev

The column "NLWP" shows number of threads, and the column "LWP" shows the thread ID.
In the column "CLS" is the thread scheduling policy shown. It can be for example
``TS`` for time sharing, ``RR`` for round robin or ``FF`` for FIFO scheduling.

To show the names of the threads, use custom output of the ``ps`` command::

   pi@raspberrypi:~$ ps H -o 'pid ppid nlwp tid lwp comm cls pri psr time' $(pidof pn_dev)
   PID  PPID NLWP   TID   LWP COMMAND         CLS PRI PSR     TIME
   1018     1    8  1018  1018 pn_dev           TS  19   2 00:00:00
   1018     1    8  1020  1020 os_eth_task      FF  50   0 00:00:00
   1018     1    8  1021  1021 os_eth_task      FF  50   0 00:00:00
   1018     1    8  1022  1022 os_eth_task      FF  50   0 00:00:00
   1018     1    8  1023  1023 p-net_bg_worker  FF  45   0 00:00:08
   1018     1    8  1029  1029 pn_snmp          FF  41   1 00:00:00
   1018     1    8  1030  1030 os_timer         FF  70   3 00:01:37
   1018     1    8  1031  1031 pn_dev           FF  55   2 00:01:54

The "PSR" column shows which processor (core) the thread is running on.


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

Display which CPU core a process is running on::

   pi@pndevice-pi:~$ taskset -c -p $(pidof pn_dev)
   pid 443's current affinity list: 2

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


Network interface hardware
--------------------------
If your Ethernet network interface is connected via USB, there can be an
additional latency. This can affect the frame-to-frame interval for
transmitted Profinet frames.

For example the Ethernet interface in a Raspberry Pi 3 is connected via
an USB bus internally.
