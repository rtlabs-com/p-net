Additional Linux Details
========================
The tutorial gives most information required for getting started on a Linux
machine. Some additional information is given here.


Detailed dependency installation info
-------------------------------------
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
        clang-format-10 \
        doxygen \
        graphviz


Run tests and generate documentation
------------------------------------
Run tests (if you told cmake to configure it)::

    cmake --build build --target check

Run a single test file::

    build/pf_test --gtest_filter=CmrpcTest.CmrpcConnectReleaseTest

Create Doxygen documentation::

    cmake --build build --target docs

The Doxygen documentation ends up in ``build/html/index.html``

See the "Writing documentation" page if you would like to install
the toolchain to build the entire Sphinx documentation.

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
Conformance class B requires SNMP support. Linux uses net-snmp as agent,
see http://www.net-snmp.org/. P-Net for Linux implements
a Net-SNMP subagent that handles the Profinet mandatory MIB:s. Also
see :ref:`network-topology-detection` for information regarding SNMP.

Enable SNMP by setting ``PNET_OPTION_SNMP`` to ``ON``. Net-SNMP also needs to
be installed. On Ubuntu you can install the required packages using::

  sudo apt install -y snmpd libsnmp-dev

The p-net SNMP subagent will handle the system objects so the default
SNMP system module should be disabled by adding the snmpd argument
``-I -system_mib``. On Ubuntu Linux you should change
``/lib/systemd/system/snmpd.service`` to read::

  [Unit]
  Description=Simple Network Management Protocol (SNMP) Daemon.
  After=network.target
  ConditionPathExists=/etc/snmp/snmpd.conf

  [Service]
  Type=simple
  ExecStartPre=/bin/mkdir -p /var/run/agentx
  ExecStart=/usr/sbin/snmpd -LOw -u Debian-snmp -g Debian-snmp -I -system_mib,smux,mteTrigger,mteTriggerConf -f -p /run/snmpd.pid
  ExecReload=/bin/kill -HUP $MAINPID

  [Install]
  WantedBy=multi-user.target

To see the status of the service::

   systemctl status snmpd.service
   journalctl -u snmpd.service -f

To restart the service after modification::

   sudo systemctl daemon-reload
   sudo systemctl restart snmpd.service

The file snmpd.conf controls access to the snmp agent. It should be
set to listen on all interfaces and allow read-write access to the
Profinet MIB:s. On Ubuntu Linux you should change
``/etc/snmp/snmpd.conf`` to read::

   master  agentx
   agentaddress  0.0.0.0,[::1]
   view   systemonly  included   .1.3.6.1.2.1.1
   view   systemonly  included   .1.0.8802.1.1.2
   rocommunity  public  default -V systemonly
   rwcommunity  private default -V systemonly

To verify the SNMP capabilities, first use ``ping`` to make sure you have a
connection to the device, and then use ``snmpwalk``::

   ping 192.168.0.50
   snmpwalk -v1 -c public 192.168.0.50 1
   snmpget -v1 -c public 192.168.0.50 1.3.6.1.2.1.1.4.0
   snmpset -v1 -c private 192.168.0.50 1.3.6.1.2.1.1.4.0 s "My new sys contact"

See :ref:`network-topology-detection` for more details on SNMP.


snmpd in a Yocto build
----------------------
In an embedded Linux Yocto build, you would include the ``snmpd`` daemon by
using the ``net-snmp`` recipe.
