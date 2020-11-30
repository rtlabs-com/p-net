Getting started on Linux
========================
Running the IO-device application on Linux


Install dependencies
--------------------
You need to have cmake version at least 3.14 installed. It is available in
Ubuntu 19.04 and later.

To install on Ubuntu 16.04 or 18.04, follow the instructions here:
https://blog.kitware.com/ubuntu-cmake-repository-now-available/

It is also possible to install it via snap::

    sudo snap install cmake --classic

When using it, you might need to replace ``cmake`` with ``snap run cmake``.
If uninstalling the default ``cmake``, the snap-installed version should be
used by default after reboot.

Optional dependencies (for development and documentation of the p-net stack)::

    sudo apt install -y \
        clang-tools \
        doxygen \
        graphviz


Download and compile p-net
---------------------------
Clone the source::

    git clone --recurse-submodules https://github.com/rtlabs-com/p-net.git

This will clone the repository with submodules. If you already cloned
the repository without the ``--recurse-submodules`` flag then run this
in the ``p-net`` folder::

    git submodule update --init --recursive

Then create and configure the build::

    cmake -B build -S p-net

or maybe ::

    cmake -B build -S p-net -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=ON -DUSE_SCHED_FIFO=ON

You can choose any name for the build folder, for instance if you want
to build different configurations.

Build the code::

    cmake --build build --target install

Use the ``-j`` flag to ``make`` to enable parallel build.

Depending on how you installed cmake, you might need to run ``snap run cmake``
instead of ``cmake``.

Use the ``install`` target to install scripts for manipulating IP
settings, control LEDs etc.


Run the Linux demo application
------------------------------
If you need to set a static IP-address (adapt IP address and network adapter name)::

   sudo ifconfig eth0 192.168.0.50 netmask 255.255.255.0 up

Usage of the demo IO-device application:

.. code-block:: none

    $ ./pn_dev --help

    Demo application for p-net Profinet device stack.

    Wait for connection from IO-controller.
    Then read buttons (input) and send to controller.
    Listen for application LED output (from controller) and set application LED state.
    It will also send a counter value (useful also without buttons and LED).
    Button1 value is sent in the periodic data. Button2 triggers an alarm.

    Also the mandatory Profinet signal LED is controlled by this application.

    The LEDs are controlled by the script set_profinet_leds_linux
    located in the same directory as the application binary.
    A version for Raspberry Pi is available, and also a version writing
    to plain text files (useful for demo if no LEDs are available).

    Assumes the default gateway is found on .1 on same subnet as the IP address.

    Optional arguments:
        --help       Show this help text and exit
        -h           Show this help text and exit
        -v           Incresase verbosity
        -f           Reset to factory settings, and store to file. Exit.
        -r           Remove stored files and exit.
        -g           Show stack details and exit. Repeat for more details.
        -i INTERF    Name of Ethernet interface to use. Defaults to eth0
        -s NAME      Set station name. Defaults to rt-labs-dev  Only used
                     if not already available in storage file.
        -b FILE      Path (absolute or relative) to read button1. Defaults to not read button1.
        -d FILE      Path (absolute or relative) to read button2. Defaults to not read button2.
        -p PATH      Absolute path to storage directory. Defaults to use current directory.

Run the sample application::

    sudo ifconfig eth0 192.168.0.50 netmask 255.255.255.0 up
    sudo build/pn_dev -v

On Raspberry Pi::

    sudo build/pn_dev -v -b /sys/class/gpio/gpio22/value -d /sys/class/gpio/gpio27/value

Note that you must set up the GPIO files properly first (see the Raspberry Pi
page).


Adjust log level
----------------
If you would like to change the log level, run ``ccmake .`` in the ``build``
directory. It will start a menu program. Move to the LOG_LEVEL entry, and
press Enter to change to DEBUG. Press c to save and q to exit.

You need to re-build the project for the changes to take effect.


Run tests and generate documentation
------------------------------------
Run tests (if you told cmake to configure it)::

    cmake --build build --target check

Run a single test file::

    build/pf_test --gtest_filter=CmrpcTest.CmrpcConnectReleaseTest

Create Doxygen documentation::

    cmake --build build --target docs

The Doxygen documentation ends up in ``build/html/index.html``

The clang static analyzer can also be used if installed. Create a new
build directory by running::

   scan-build cmake -B build.scan-build -S p-net
   scan-build cmake --build build


Setting Linux ephemeral port range
----------------------------------
This is the range of random source ports used when sending UDP messages.
Profinet requires that the UDP source port should be >= 0xC000, which is 49152
in decimal numbers.

To change the ephemeral port range::

    echo "49152 60999" > /proc/sys/net/ipv4/ip_local_port_range

This should typically be done at system start up.


File size and memory usage on Linux
-----------------------------------
The resulting file size of the sample application binary is heavily dependent
on the compile time options, for example whether to include debug information.
In this example we use::

   BUILD_SHARED_LIBS ON
   CMAKE_BUILD_TYPE Release
   LOG_LEVEL Warning
   PNET_MAX_AR 2
   PNET_MAX_SLOTS 5
   PNET_MAX_SUBSLOTS 3

To get an estimate of the binary size, partially link it (use release, without
standard libraries)::

   p-net/build$ make all
   p-net/build$ /usr/bin/cc -O3 -DNDEBUG CMakeFiles/pn_dev.dir/sample_app/sampleapp_common.o CMakeFiles/pn_dev.dir/src/ports/linux/sampleapp_main.o -o pn_dev libprofinet.a -nostdlib -r

Resulting size::

   p-net/build$ size pn_dev
      text	   data	    bss	    dec	    hex	filename
   244481	     72	      8	 244561	  3bb51	pn_dev

See https://linux.die.net/man/1/size for information on how to use the command.
Also the rt-kernel page in this documentation has some description on how to
interpret the output.

The size of the p-net stack can be estimated from the size of libprofinet,
built with the options given above::

   p-net/build$ size libprofinet.so
      text	   data	    bss	    dec	    hex	filename
   230888	   3304	      8	 234200	  392d8	libprofinet.so

An estimate of the p-net RAM usage can be made from the size of the pnet_t struct.
The sample application has a command line option to show this value, for the used
compile time options (for example the maximum number of modules allowed).


Debug intermittent segmentation faults during tests on Linux
------------------------------------------------------------

Enable core dumps::

    ulimit -c unlimited

Run a test case until the problem occurs (in the build directory)::

    while ./pf_test --gtest_filter=DiagTest.DiagRunTest; do :; done

Study the resulting core::

    gdb pf_test core

SNMP (Conformance class B)
--------------------------

Conformance class B requires SNMP support. P-Net for Linux implements
a Net-SNMP subagent that handles the Profinet mandatory MIB:s. Also
see :ref:`network-topology-detection` for information regarding SNMP.

Enable SNMP by setting PNET_OPTION_SMP to ON. Net-SNMP also needs to
be installed. On Ubuntu you can install the required packages using::

    sudo apt install -y snmpd libsnmp-dev

The file snmpd.conf controls access to the snmp agent. The following
needs to be set::

    master  agentx
    agentaddress  0.0.0.0,[::1]
    view   systemonly  included   .1.3.6.1.2.1.1
    view   systemonly  included   .1.3.6.1.2.1.25.1
    view   systemonly  included   .1.0.8802.1.1.
    rocommunity  public default -V systemonly

This enables agentx support, listens on all interfaces and allows
read-only access to the Profinet MIB:s.
