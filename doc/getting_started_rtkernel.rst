Getting started using rt-kernel
===============================

Evaluation board details
------------------------
This example uses the Infineon evaluation board "XMC4800 Relax EtherCAT Kit".
It has a Cortex®-M4 running at 144 MHz, and it has 352 kB RAM and 2 MB flash.
Details are found on
https://www.infineon.com/cms/en/product/evaluation-boards/kit_xmc48_relax_ecat_v1/

You can use other evaluation boards supported by rt-kernel, but you need to
adapt paths etc accordingly.


Install tools on a Linux Laptop
-------------------------------
Install (see description elsewhere):

* Workbench (Version 2020.1 or later)
* Compiler
* rt-kernel, use variant ``rt-kernel-xmc4``
* Segger J-link
* Run ``pip install pyyaml``
* cmake (version 3.14 or later)


Copy the rt-kernel sources
--------------------------
Run (adapt paths)::

    cp -r /opt/rt-tools/rt-kernel-xmc4 /home/jonas/projects/profinetstack/rtkernelcopy/

Patch the source by using the patch file available in the p-net repository.
Verify that you have the version of rt-kernel that the patch file is intended for.
Run this in the root folder of the rt-kernel directory::

   patch -p1 < PATH_TO_PNET/src/ports/rt-kernel/0001-rtkernel-for-Profinet.patch

You may want to change IP settings in ``rt-kernel-xmc4/bsp/xmc48relax/include/config.h``.

In the root folder of the rt-kernel directory::

    source setup.sh xmc48relax
    make clean
    make -j


Download and compile p-net
--------------------------
Clone the source::

    git clone --recurse-submodules https://github.com/rtlabs-com/p-net.git

This will clone the repository with submodules. If you already cloned
the repository without the ``--recurse-submodules`` flag then run this
in the ``p-net`` folder::

    git submodule update --init --recursive

The CMake build files for rt-kernel need to know which BSP you want to
use, as well as the path to your rt-kernel source tree. Run the
following to create and configure the build::

    RTK=<PATH TO YOUR MODIFIED>/rt-kernel-xmc4 \
    BSP=xmc48relax \
    cmake -B build -S p-net \
    -DCMAKE_TOOLCHAIN_FILE=cmake/tools/toolchain/rt-kernel.cmake \
    -DCMAKE_ECLIPSE_EXECUTABLE=/opt/rt-tools/workbench/Workbench \
    -DCMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT=TRUE -G "Eclipse CDT4 - Unix Makefiles"

If you are using Windows, start the program "Command line" found in
the rt-labs rt-collab Toolbox installation (adapt paths to your setup)::

    cd /c/Users/rtlfrm/Documents/Profinet

    RTK=/c/Users/rtlfrm/Documents/Profinet/rt-kernel-xmc4 \
    BSP=xmc48relax \
    /c/Program\ Files/CMake/bin/cmake.exe -B build -S p-net \
    -DCMAKE_TOOLCHAIN_FILE=cmake/tools/toolchain/rt-kernel.cmake \
    -DCMAKE_ECLIPSE_EXECUTABLE=/opt/rt-tools/workbench/Workbench \
    -DCMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT=TRUE -G "Eclipse CDT4 - Unix Makefiles"

Depending on how you installed cmake, you might need to run ``snap run
cmake`` instead of ``cmake``. This creates build files in a folder
named ``build``. You can choose any name for the build folder, for
instance if you want to build for multiple targets.

After running ``cmake`` you can run ``ccmake build`` or ``cmake-gui``
in the build folder to change settings.

Start Workbench::

    /opt/rt-tools/workbench/Workbench

Two Workbench projects have been created by cmake. One project is for building
the code, the other is for the p-net source code. You need at least to
import the project for building the code.

Use the menu "File > Import". Select "General > Existing Projects". Click
"Browse" and select the ``build`` directory that was created earlier.

Use the menu "Project > Build All" to build it.

An OSAL layer hosted in a separate directory is used by p-net, and is installed
automatically during setup. For details see the tutorial page
in this documentation.


More Workbench settings
-----------------------
If you intend to edit the p-net source code in the Workbench tool, you
should also import the p-net source project. The files ``.project``
and ``.cproject`` have been created in the p-net repo by cmake.

Use the menu "File > Import". Select "General > Existing Projects". Click
"Browse" and select the ``p-net`` repo directory.

After importing, right-click on the project and choose "New > Convert
to a C/C++ project". This will setup the project so that the indexer
works correctly and the Workbench revision control tools can be used.

Run on target
-------------
Install J-link from https://www.segger.com/
Start Segger J-link GDB debug server::

    JLinkGDBServerExe

Select "USB" and target device "XMC4800-2048" in the GUI. Use "Little Endian",
Target interface "SWD" and speed "Auto Selection".

Run the compiled code on target by right-clicking the Profinet build project,
and selecting "Debug as > Hardware debugging". Select J-Link.
On the "Startup" tab enter ``monitor reset 0`` in the "Run commands".
Click Apply and Close. Select ``pn_dev.elf`` and click OK.
The download progress pop-up window should appear.

The resulting ``.elf`` file contains the sample application, the p-net stack,
the rt-kernel, lwip and drivers.

If you need to adjust debugger settings later, right-click the Profinet build
project, and select "Debug as > Debug configurations". Select the "Profinet... "
node. You might need to double click "Hardware Debugging" if the child node
does not appear. Typically these values have been automatically entered:

* Tab "Main" C/C++ application: ``pn_dev.elf``.
* Tab "Debugger". Debugger type J-Link. GDB command:
  ``${COMPILERS}/arm-eabi/bin/arm-eabi-gdb``.

To be able to view register content, use the MMR tab in the debug view. Select
core "XMC4800".

Open a terminal to view the debug output from the target, which will appear as
for example ``/dev/ttyACM0``. An example of a terminal program is picocom
(add yourself to the ``dialout`` user group to avoid using sudo)::

    sudo picocom -b 115200 /dev/ttyACM0

You can step-debug in the Workbench GUI. Press the small "Resume" icon to have
the target run continuously.


Adjust log level
----------------
In order to learn the Profinet communication model, it is very informative to
adjust the log level to see the incoming and outgoing messages. See the
tutorial page for details on how to adjust the log level.

However note that printing out log strings is slow, so you probably need
to decrease the cyclic data frequency (see PLC timing settings below).
It is recommended to use log level ERROR when running with short cycle times
on a microcontroller, in order not to interfere with the real-time
requirements of the Profinet communication.


Standalone rt-kernel project
----------------------------
This creates standalone makefiles.

Configure the build::

    RTK=<PATH TO YOUR MODIFIED>/rt-kernel-xmc4 \
    BSP=xmc48relax \
    cmake -B build -S p-net \
    -DCMAKE_TOOLCHAIN_FILE=cmake/tools/toolchain/rt-kernel.cmake \
    -G "Unix Makefiles"

Build the code::

    cmake --build build


Serial port baud rate
---------------------
If you like to increase the baud rate of the serial port, change the value in
the file ``bsp/xmc48relax/src/xmc48relax.c``. For example change
``.baudrate = 115200,`` to ``.baudrate = 460800,``.

To be able to run debug logging via serial cable, you need to increase the
baudrate to 460800 bits/s.


PLC timing settings
-------------------
The send clock is 1 ms in the GSDML file.

If you do lots of printouts (which are slow) from the application on the
XMC4800 board, you might need to increase the reduction ratio in the PLC
settings to avoid timeout errors.

In case of problems, increase the reduction ratio (and timeout) value a lot,
and then gradually reduce it to find the smallest usable value.


Using the built-in rt-kernel shell
----------------------------------
Press Enter key to enter the built-in rt-kernel shell via the serial console.
To view a list of available commands, use::

   help

Example commands::

   ls /disk1
   hexdump /disk1/pnet_data_ip.bin
   rm /disk1/pnet_data_ip.bin
   pnio_factory_reset
   pnio_remove_files
   pnio_show


Memory requirements for the tests
---------------------------------
Note that the tests require a stack of at least 6 kB. You may have to increase
``CFG_MAIN_STACK_SIZE`` in your BSP ``include/config.h`` file.


Examining flash and RAM usage
-----------------------------
The flash and RAM usage is shown by the tool ``arm-eabi-size``.
In this example we use::

   CMAKE_BUILD_TYPE Release
   LOG_LEVEL Warning
   PNET_MAX_AR 1
   PNET_MAX_SLOTS 5
   PNET_MAX_SUBSLOTS 3

To estimate the binary size, link partially (without standard
libraries). This example is for cortex-m4f MCU:s, such as XMC4800::

   build$ make all
   build$ /opt/rt-tools/compilers/arm-eabi/bin/arm-eabi-gcc -O3 -DNDEBUG -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 CMakeFiles/pn_dev.dir/samples/pn_dev/sampleapp_common.o CMakeFiles/pn_dev.dir/src/ports/rt-kernel/sampleapp_main.o -o pn_dev.elf libprofinet.a -nostartfiles -nostdlib -r

Study the resulting executable::

   build$ arm-eabi-size pn_dev.elf
      text   data    bss     dec      hex  filename
    127421     16   1388  128825    1f739  pn_dev.elf

Values in bytes (including the rt-kernel RTOS).

* text: code in flash
* data: Memory, statically initialized
* bss: Memory, zero-initialized. For example the stack.
* dec = text + data + bss
* hex = text + data + bss (in hexadecimal)

The flash usage is text + data, as the RAM initialization values are stored in flash.


Run tests on XMC4800 target
---------------------------
In order to compile the test code, make sure to use ``BUILD_TESTING`` and that
``TEST_DEBUG`` is disabled. Reduce ``PNET_MAX_FILENAME_SIZE`` to 30 bytes.
This is done via ccmake, which should be started in the build directory::

    ccmake .

Set ``CFG_MAIN_STACK_SIZE`` to at least 8192 in ``rt-kernel-xmc4/bsp/xmc48relax/include/config.h``

The resulting file after compiling is named ``pf_test.elf``

Add a new hardware debugging configuration, where the C/C++ application on the
"Main" tab is set to ``pn_dev.elf``.

The test will run on the target board when starting hardware debugging.
You might need to press the Play button in the Workbench if you have enabled
breakpoints.


Run tests on the QEMU emulator
------------------------------
On a Linux laptop, install the package ``rt-collab-qemu``.

Patch the rt-kernel. Use the BSP "integrator", which is intended for emulation.
You need to increase the main stack size in ``rt-kernel/bsp/integrator/include/config.h``.
Modify ``CFG_MAIN_STACK_SIZE``.

In the root folder of the rt-kernel directory::

    source setup.sh integrator
    make clean
    make -j

In the parent directory of ``p-net``, configure a new build directory::

    RTK=<PATH TO YOUR MODIFIED>/rt-kernel \
    BSP=integrator \
    cmake -B build.integrator -S p-net \
    -DCMAKE_TOOLCHAIN_FILE=cmake/tools/toolchain/rt-kernel.cmake \
    -G "Unix Makefiles"

If necessary adjust the settings::

    ccmake build.integrator/

Build only the ``pf_test`` binary::

    cmake --build build.integrator/ -j --target pf_test

Start the emulator::

    /opt/rt-tools/qemu/bin/qemu-system-arm -M integratorcp -nographic -semihosting -kernel build.integrator/pf_test.elf

If you add ``-s`` it it possible to connect with ``gdb`` to port 1234 from
Workbench. By adding ``-S`` qemu will wait for gdb to connect.

To send a command line argument to the gtest binary, add ``--append "<gtest_command"``.
For example ``--append "--gtest_filter=AlarmUnitTest*"`` or
``--append "--gtest_filter=CmrpcTest.CmrpcConnectReleaseTest"``.

Exit QEMU with CTRL-A X (not CTRL-A CTRL-X).


SNMP
----
To enable SNMP support, set the ``PNET_OPTION_SNMP`` value to ``ON``.

See :ref:`network-topology-detection` for more details on SNMP and how to
verify the SNMP communication to the p-net stack.


IP-stack LWIP
-------------
The rt-kernel uses the "lwip" IP stack.

To enable logging in lwip, modify the file
``rt-kernel-xmc4/lwip/src/include/lwip/lwipopts.h``.

Make sure general logging is enabled::

   #define LWIP_DEBUG 1
   #define LWIP_DBG_MIN_LEVEL          LWIP_DBG_LEVEL_ALL
   #define LWIP_DBG_TYPES_ON           LWIP_DBG_ON

And enable debug logging of the modules you are interested in::

   #define PBUF_DEBUG                  LWIP_DBG_OFF
   #define IP_DEBUG                    LWIP_DBG_ON
   #define IGMP_DEBUG                  LWIP_DBG_ON
   #define TCPIP_DEBUG                 LWIP_DBG_ON

Rebuild rt-kernel.


Increase LWIP resources
-----------------------
In order to handle incoming data, you might need to increase buffer sizes for
the lwip IP stack.

In the file ``lwip/src/include/lwip/lwipopts.h`` or in
``lwip/src/include/lwip/opt.h`` (which holds the default values), increase the
values for ``MEMP_NUM_NETBUF`` and ``PBUF_POOL_SIZE``.

It can also be beneficial to increase the values ``eth_cfg.rx_buffers``
and ``eth_cfg.rx_task_prio`` found in the ``bsp/xmc48relax/src/lwip.c`` file.

For debugging you can enable ``LWIP_STATS_DISPLAY`` in the ``lwipopts.h`` file,
and then trigger the ``stats_display()`` function.
