Getting started using rt-kernel
===============================

Evaluation board details
------------------------
This example uses the Infineon evaluation board "XMC4800 Relax EtherCAT Kit".
Details are found on
https://www.infineon.com/cms/en/product/evaluation-boards/kit_xmc48_relax_ecat_v1/

You can use other evaluation boards supported by rt-kernel, but you need to
adapt paths etc accordingly.


Install tools on a Linux Laptop
-------------------------------
Install (see description elsewhere):

* Workbench (Version 2017.1 or later)
* Compiler
* rt-kernel, use variant ``rt-kernel-xmc4``
* Segger J-link
* Run ``pip install pyyaml``
* cmake (version 3.13 or later)


Copy the rt-kernel sources
--------------------------
Run (adapt paths)::

    cp -r /opt/rt-tools/rt-kernel-xmc4 /home/jonas/projects/profinetstack/rtkernelcopy/

Patch the source (in the root folder of the rt-kernel directory)::

   patch -p1 < PATH_TO/profinet_lwip.patch
   patch -p1 < PATH_TO/profinet_bsp.patch

Change IP settings in ``rt-kernel-xmc4/bsp/xmc48relax/include/config.h``.
Modify the respective lines to::

   #undef CFG_LWIP_ADDRESS_DYNAMIC

   #define CFG_LWIP_IPADDR()       IP4_ADDR (&ipaddr, 192, 168, 137, 4)

   #define CFG_LWIP_GATEWAY()      IP4_ADDR (&gw, 192, 168, 137, 1)

In the root folder of the rt-kernel directory::

    source setup.sh xmc48relax
    make clean
    make -j


Download and compile p-net
--------------------------
Clone the source::

    mkdir profinet
    cd profinet
    git clone https://github.com/rtlabs-com/p-net.git

Out-of-tree builds are recommended. Create a build directory and run the
following commands from that directory. In the following instructions, the
root folder for the repo is assumed to be an absolute or relative path in an
environment variable named repo.

Create a build directory (on same level as p-net directory) and run cmake::

    mkdir build
    cd build
    export COMPILERS=/opt/rt-tools/compilers
    export RTK=<PATH TO YOUR MODIFIED>/rt-kernel-xmc4/
    export BSP=xmc48relax

The cmake executable is assumed to be in your path. Run::

    cmake ../p-net -DCMAKE_TOOLCHAIN_FILE=../p-net/cmake/toolchain/rt-kernel-xmc4.cmake -DLOG_LEVEL=DEBUG -DBUILD_TESTING=OFF -DCMAKE_ECLIPSE_EXECUTABLE=/opt/rt-tools/workbench/Workbench -DCMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT=TRUE -G "Eclipse CDT4 - Unix Makefiles"

Depending on how you installed cmake, you might need to run ``snap run cmake``
instead of ``cmake``.

After running ``cmake`` you can run ``ccmake`` or ``cmake-gui`` to change settings.

Start Workbench::

    /opt/rt-tools/workbench/Workbench

Two Workbench projects have been created by cmake. One project is for building
the code, the other is for the p-net source code. You need at least to
import the project for building the code.

Use the menu "File > Import". Select "General > Existing Projects". Click
"Browse" and select the ``build`` directory that was created earlier.

Use the menu "Project > Build All" to build it.


More Workbench settings
-----------------------
If you intend to edit the p-net source code in the Workbench tool, you also
need to import the p-net source project. The files ``.project`` and
``.cproject`` have been created in the p-net repo by cmake.

Use the menu "File > Import". Select "General > Existing Projects". Click
"Browse" and select the ``p-net`` repo directory.


Run on target
-------------
First start Segger J-link GDB debug server::

    JLinkGDBServerExe

Select "USB" and target device "XMC4800-2048" in the GUI. Use "Little Endian",
Target interface "SWD" and speed "Auto Selection".

Run the compiled code on target by right-clicking the Profinet build project,
and selecting "Debug as > Hardware debugging". Select J-Link.
On the "Startup" tab enter ``monitor reset 0`` in the "Run commands".
Click Apply and Close. Select ``pn_dev.elf`` and click OK.
The download progress pop-up window should appear.

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
the target run continuously. The you should be able to use the on-target shell
via the serial console. To view a list of available commands, use::

   help

To start the sample application on target, type this command in the on-target
shell::

   pnio_run


Adjust log level
----------------
See the "Getting started on Linux" page.


Standalone rt-kernel project
----------------------------
This creates standalone makefiles.

Use::

    user@host:~/build$ cmake $repo \
        -DCMAKE_TOOLCHAIN_FILE=$repo/cmake/toolchain/rt-kernel-arm9e.cmake \
        -G "Unix Makefiles"
    user@host:~/build$ make all


Memory requirements for the tests
---------------------------------
Note that the tests require a stack of at least 6 kB. You may have to increase
CFG_MAIN_STACK_SIZE in your BSP ``include/config.h`` file.
