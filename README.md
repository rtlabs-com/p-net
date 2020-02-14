p-net: PROFINET device stack
============================
[![Build Status](https://travis-ci.org/rtlabs-com/p-net.svg?branch=master)](https://travis-ci.org/rtlabs-com/p-net)

The rt-labs PROFINET stack p-net is used for PROFINET device
implementations. It is easy to use and provides a small footprint. It
is especially well suited for embedded systems where resources are
limited and efficiency is crucial.

It is written in C and can be run on bare-metal hardware, an RTOS such
as rt-kernel, or on Linux. The main requirement is that the
platform can send and receive RAW Ethernet Layer 2 frames. The
p-net stack is supplied with full sources including a porting
layer.

rt-labs p-net is developed according to specification 2.3:

 * Conformance Class A (Class B upon request)
 * Real Time Class 1

Features:

 * TCP/IP
 * RT (real-time)
 * Address resolution
 * Parameterization
 * Process IO data exchange
 * Alarm handling
 * Configurable number of modules and sub-modules
 * Bare-metal or OS
 * Porting layer provided

The stack includes a comprehensive set of unit-tests.

Prerequisites for all platforms
===============================

 * CMake 3.13 or later

Out-of-tree builds are recommended. Create a build directory and run
the following commands from that directory. In the following
instructions, the root folder for the repo is assumed to be an
absolute or relative path in an environment variable named *repo*.

The cmake executable is assumed to be in your path. After running
cmake you can run ccmake or cmake-gui to change settings.


Linux
=====

 * GCC 4.6 or later

```console
user@host:~/build$ cmake $repo
user@host:~/build$ make all check
```

The clang static analyzer can also be used if installed. From a clean
build directory, run:

```console
user@host:~/build$ scan-build cmake $repo
user@host:~/build$ scan-build make
```

rt-kernel
=========

 * Workbench 2017.1 or later

Set the following environment variables. You should use a bash shell,
such as for instance the Command Line in your Toolbox installation.

```console
user@host:~/build$ export COMPILERS=/opt/rt-tools/compilers
user@host:~/build$ export RTK=/path/to/rt-kernel
user@host:~/build$ export BSP=integrator
```

Standalone project
------------------

This creates standalone makefiles.

```
user@host:~/build$ cmake $repo \
    -DCMAKE_TOOLCHAIN_FILE=$repo/cmake/toolchain/rt-kernel-arm9e.cmake \
    -G "Unix Makefiles"
user@host:~/build$ make all
```

Workbench project
-----------------

This creates a Makefile project that can be imported to Workbench. The
project will be created in the build directory.

```
user@host:~/build$ cmake $repo \
    -DCMAKE_TOOLCHAIN_FILE=$repo/cmake/toolchain/rt-kernel-arm9e.cmake \
    -DCMAKE_ECLIPSE_EXECUTABLE=/opt/rt-tools/workbench/Workbench \
    -DCMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT=TRUE \
    -G "Eclipse CDT4 - Unix Makefiles"
```

A source project will also be created in the $repo folder. This
project can also be imported to Workbench. After importing,
right-click on the project and choose *New* -> *Convert to a C/C++
project*. This will setup the project so that the indexer works
correctly and the Workbench revision control tools can be used.

For both types of projects, substitute BSP and toolchain file as
appropriate.

The library and the unit tests will be built. Note that the tests
require a stack of at least 6 kB. You may have to increase
CFG_MAIN_STACK_SIZE in your bsp include/config.h file.

Contributions
=============

Contributions are welcome. If you want to contribute you will need to
sign a Contributor License Agreement and send it to us either by
e-mail or by physical mail. More information is available
[here](https://rt-labs.com/contribution).
