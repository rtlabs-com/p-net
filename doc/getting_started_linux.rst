Getting started on Linux
========================
Running the IO-device application on Linux


Install dependencies
--------------------
You need to have cmake version at least 3.13 installed. It is available in
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


Download and compile source
---------------------------
Clone the source::

    mkdir profinet
    cd profinet
    git clone https://github.com/rtlabs-com/p-net.git

Compile::

    cd p-net
    mkdir build
    cd build
    cmake ..
    make all

or maybe ::

    cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=ON -DUSE_SCHED_FIFO=ON

Use the ``-j`` flag to ``make`` to enable parallel build.

Depending on how you installed cmake, you might need to run ``snap run cmake``
instead of ``cmake``.


Run the Linux demo application
------------------------------
If you need to set a static IP-address (adapt IP address and network adapter name)::

   sudo ifconfig eth0 192.168.137.4 netmask 255.255.255.0 up

Usage of the demo IO-device application:

.. code-block:: none

    $ ./pn_dev --help

    Demo application for p-net Profinet device stack.

    Wait for connection from IO-controller.
    Then read buttons (input) and send to controller.
    Listen for LED output (from controller) and set LED state.
    It will also send a counter value (useful also without
    buttons and LED).
    Button1 value is sent in the periodic data. Button2 triggers an alarm.

    The LED is controlled by writing '1' or '0' to the control file,
    for example /sys/class/gpio/gpio17/value
    A pressed button should be indicated by that the first character in
    the control file is '1'.
    Make sure to activate the appropriate GPIO files by exporting them.
    If no hardware-controlling files in /sys are available, you can
    still try the functionality by using plain text files.

    Assumes the default gateway is found on .1 on same subnet as the IP address.

    Optional arguments:
        --help       Show this help text and exit
        -h           Show this help text and exit
        -v           Incresase verbosity
        -i INTERF    Set Ethernet interface name. Defaults to eth0
        -s NAME      Set station name. Defaults to rt-labs-dev
        -l FILE      Path to control LED. Defaults to not control any LED.
        -b FILE      Path to read button1. Defaults to not read button1.
        -d FILE      Path to read button2. Defaults to not read button2.

Run the sample application::

    cd build
    make pn_dev
    sudo ifconfig eth0 192.168.137.4 netmask 255.255.255.0 up
    sudo ./pn_dev -v

On Raspberry Pi::

    sudo build/pn_dev -v -l /sys/class/gpio/gpio17/value -b /sys/class/gpio/gpio22/value -d /sys/class/gpio/gpio27/value

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

    cd build
    make check

Run a single test file::

    cd build
    test/pf_test --gtest_filter=CmrpcTest.CmrpcConnectReleaseTest

Create Doxygen documentation::

    cd build
    make docs

The Doxygen documentation ends up in ``build/html/index.html``

The clang static analyzer can also be used if installed. From a clean
build directory, run::

   scan-build cmake ..
   scan-build make
