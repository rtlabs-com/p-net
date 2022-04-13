Additional Linux Details
========================
The tutorial gives most information required for getting started on a Linux
machine. Some additional information is given here.


Detailed dependency installation info
-------------------------------------
You need to have cmake version at least 3.14 installed. It is available in
Ubuntu 19.04 and later.

To install on Ubuntu 16.04 or 18.04, follow the instructions here:
https://www.kitware.com/ubuntu-cmake-repository-now-available/

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
   p-net/build$ /usr/bin/cc -O3 -DNDEBUG CMakeFiles/pn_dev.dir/samples/pn_dev/sampleapp_common.o CMakeFiles/pn_dev.dir/src/ports/linux/sampleapp_main.o -o pn_dev libprofinet.a -nostdlib -r

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

An estimate of the p-net RAM usage can be made from the size of the ``pnet_t`` struct.
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
see http://www.net-snmp.org/. The name of the SNMP agent binary is ``snmpd``.

P-Net for Linux implements a Net-SNMP subagent that handles the Profinet
mandatory MIB:s. Also
see :ref:`network-topology-detection` for information regarding SNMP.

Enable SNMP by setting ``PNET_OPTION_SNMP`` to ``ON`` in the p-net compilation
options. Net-SNMP also needs to
be installed. On Ubuntu you can install the required packages using::

  sudo apt install -y snmpd libsnmp-dev

To show the installed version of ``snmpd``, use::

   snmpd -v


Change snmpd command line arguments
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The p-net SNMP subagent will handle the system objects, so the default
SNMP "system" module should be disabled by adding the snmpd argument
``-I -system_mib``. Otherwise the subagent will complain about
"registering pdu failed" at startup. If you use systemd init system (for
example on Ubuntu Linux) you should change
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

The command line arguments ``-u`` and ``-g`` are for the user id and group id
of the snmpd process will use after its initial startup.
They are not necessary to use snmpd.
However snmpd must be started with permissions to open relevant sockets,
typically root permissions.

If you use "system V init" instead of systemd, then snmpd is typically started
by a script file named ``/etc/init.d/snmpd``. Change the snmpd command line
arguments in the file, typically via ``SNMPDOPTS``. Stop and start the
service with::

   sudo /etc/init.d/snmpd stop
   sudo /etc/init.d/snmpd start


Configuration file for snmpd
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The file ``snmpd.conf`` controls access to the snmp agent. It should be
set to listen on all interfaces and allow read-write access to the
Profinet MIB:s. On Ubuntu Linux you should change
``/etc/snmp/snmpd.conf`` to read::

   master  agentx
   agentaddress  udp:161
   view   systemonly  included   .1.3.6.1.2.1.1
   view   systemonly  included   .1.3.6.1.2.1.2.2
   view   systemonly  included   .1.0.8802.1.1.2
   rocommunity  public  default -V systemonly
   rwcommunity  private default -V systemonly

If your Linux distribution does give a long description for ``ifDesc`` you can
override it by adding a line to the ``snmpd.conf`` file. Adapt the interface
index (last digit in OID) and the interface name::

   override 1.3.6.1.2.1.2.2.1.2.3 octet_str "enp0s31f6"

See :ref:`network-topology-detection` for more details on SNMP and how to
verify the SNMP communication to the p-net stack.


Start your application after snmpd
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
You probably would like your application to wait for the ``snmpd`` application to
be up and running. If you use systemd as init system, modify this line in
the ``[Unit]`` part of your ``.service`` file::

   After=network.target snmpd.service

You might also need to add this to the ``[Service]`` part of your
``.service`` file::

   ExecStartPre=/usr/bin/sleep 0.3

If the p-net application fails to connect to snmpd, a message "Warning: Failed
to connect to the agentx master agent" will be written to the p-net sample
app log file every 15 seconds.


Debugging snmpd settings
^^^^^^^^^^^^^^^^^^^^^^^^
The command line argument ``-LOw`` for snmpd sets the snmpd logging to
standard out with log level "warning".
To change to log level debug, use ``-LOd``. You should also use the ``-D`` flag
to select which debug messages you are interested in. Use the ``-Dagentx``
command line argument to debug the agentx communication between the snmpd and
the subagent in the p-net stack. A log example when asked for a single OID::

   Connection from UDP: [192.168.0.30]:43833->[192.168.0.50]:161
   agentx/master: agentx master handler starting, mode = 0xa0
   agentx/master: request for variable (iso.3.6.1.2.1.1.4.0)
   agentx/master: sending pdu (req=0x8,trans=0x7,sess=0x6)
   agentx_build: packet built okay
   agentx/master: got response errstat=0, (req=0x8,trans=0x7,sess=0x6)
   agentx/master: agentx_got_response() beginning...
   agentx/master:   handle_agentx_response: processing: iso.3.6.1.2.1.1.4.0
   agentx/master: handle_agentx_response() finishing...

Note that there might be a warning message "pcilib: Cannot open /proc/bus/pci"
in the snmpd log if you specify that it should use all interfaces.
That is because it will look also for (possibly non-existing) PCI interfaces.

To trouble-shoot snmpd issues, verify that no other snmpd instances are running::

   ps -ef | grep snmpd

and verify that no other process is using UDP port 161::

   sudo lsof -i udp -n -P


snmpd in a Yocto build
----------------------
In an embedded Linux Yocto build, you would include the ``snmpd`` daemon by
using the ``net-snmp`` recipe.


Persistent logs
---------------
To make the journalctl logs persistent between restarts::

   sudo mkdir -p /var/log/journal
   sudo systemd-tmpfiles --create --prefix /var/log/journal

Remove all contents of the journalctl logs::

   sudo journalctl --rotate
   sudo journalctl --vacuum-time=1s

The configurations for journalctl are located in ``/etc/systemd/journald.conf``.
If you do experiments with frequent reboots, it might be useful to change some
values::

   SyncIntervalSec=10s
   MaxRetentionSec=4h



Boot time optimization
----------------------
To improve the startup time of your Linux device, it is useful to study what
is delaying the start. If you use the "systemd" init system, you can use these
commands to analyze the startup::

   systemd-analyze
   systemd-analyze blame
   systemd-analyze critical-chain pnet-sampleapp.service

To decrease the startup time, disable services you don't use. On a Raspberry Pi
it might be for example::

   sudo systemctl disable triggerhappy triggerhappy.socket apt-daily.timer apt-daily-upgrade.timer logrotate.timer  rpi-display-backlight lightdm bluetooth hciuart rsync cups cups-browsed alsa-state avahi-daemon

Other applications that you might disable for experimentation::

   sudo systemctl disable snapd snapd.socket wpa_supplicant systemd-timesyncd dhcpcd


Debug the sample application on a Linux Laptop
----------------------------------------------
It can be convenient to be able to run the sample application and the p-net
stack in a debugger tool. It is easy using gdb and the Visual Studio Code
editor.

First make sure you can run the application from a terminal on your Linux
laptop.

Next step is to be able to run it from the terminal within Visual Studio Code.
In case of compilation error messages, you can click on the code line given
in the terminal (within Visual Studio Code) and the corresponding file will
be opened. Hold the CTRL key while clicking on the line.

To use debug features while running (for example breakpoints) you need to adapt
the settings file for Visual Studio Code. Click the "Run and Debug" icon
in the left side tool bar. Then click "Create a launch.json file". In the
"Select environment", use "C++ (GDB/LLDB)".

Modify the ``launch.json`` file to point at the correct executable, working
directory and to use correct command line arguments.

If you need to run the application with root permissions, you need to add a path in the
``"miDebuggerPath"`` field. It should point to a text file typically named
``gdb``, with this content::

   pkexec /usr/bin/gdb "$@"

Put the ``gdb`` file for example in the ``.vscode`` subdirectory within
the ``p-net`` directory. Set the executable flag::

   chmod +x gdb

An example of a ``launch.json`` file::

   {
      "version": "0.2.0",
      "configurations": [

         {
            "name": "(gdb) Launch",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/pn_dev",
            "args": ["-vv", "-i", "enp0s31f6"],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}/build/",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "miDebuggerPath": "${workspaceFolder}/.vscode/gdb",
            "setupCommands": [
               {
                  "description": "Enable pretty-printing for gdb",
                  "text": "-enable-pretty-printing",
                  "ignoreFailures": true
               }
            ]
         }

      ]
   }

The given ``"args"`` command line arguments in the example is for
increasing the verbosity level and to set the Ethernet interface name.
Adapt those and also paths to your particular setup.

Use the ``CMAKE_BUILD_TYPE`` setting as ``Debug`` when running the executable
via the debugger.

Start the debugging by clicking on the small green "Run" icon on the
"Run and Debug" page. It will stop at any breakpoint. Set a breakpoint in
any file by clicking on a line to the left of the line number.
