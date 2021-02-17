p-net Profinet device stack
===========================

Web resources
-------------

* Source repository: [https://github.com/rtlabs-com/p-net](https://github.com/rtlabs-com/p-net)
* Documentation: [https://rt-labs.com/docs/p-net](https://rt-labs.com/docs/p-net)
* Continuous integration: [https://github.com/rtlabs-com/p-net/actions](https://github.com/rtlabs-com/p-net/actions)
* RT-Labs: [https://rt-labs.com](https://rt-labs.com)


[![Build Status](https://github.com/rtlabs-com/p-net/workflows/build/badge.svg?branch=master)](https://github.com/rtlabs-com/p-net/actions?workflow=build)

p-net
-----
Profinet device stack implementation. Key features:
* Profinet v2.4
  * Conformance Class A and B
  * Real Time Class 1
* Easy to use
  * Extensive documentation and instructions on how to get started.
  * Build and run sample application on Raspberry Pi in 30 minutes.
* Portable
  * Written in C.
  * Linux, RTOS or bare metal.
  * Sources for supported port layers provided.

The RT-Labs Profinet stack p-net is used for Profinet device
implementations. It is easy to use and provides a small footprint. It
is especially well suited for embedded systems where resources are
limited and efficiency is crucial.
The stack is supplied with full sources including porting
layers and a sample application.

Also C++ (any version) is supported for application development.

The main requirement on the platform
is that it can send and receive raw Ethernet Layer 2 frames.

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
 * Supports I&M0 - I&M4. The I&M data is supported for the device, but not for individual modules.

Limitations or not yet implemented:

* This is a device stack, which means that the IO-controller/master/PLC side is not supported.
* Only a single Ethernet port (no media redundancy)
* No startup mode legacy
* No support for RT_CLASS_UDP
* No support for DHCP
* No fast start-up
* No MC multicast device-to-device
* No support of shared device (connection to multiple controllers)
* No iPar (parameter server) support
* No support for time synchronization
* No UDP frames at alarm (just the default alarm mechanism is implemented)
* No ProfiDrive or ProfiSafe profiles.

This software is dual-licensed, with GPL version 3 and a commercial license.
See LICENSE.md for more details.


Requirements
------------
The platform must be able to send and receive raw Ethernet Layer 2 frames,
and the Ethernet driver must be able to handle full size frames. It
should also avoid copying data, for performance reasons.

* cmake 3.14 or later

For Linux:

* gcc 4.6 or later
* See the "Real-time properties of Linux" page in the documentation on how to
  improve Linux timing

For rt-kernel:

* Workbench 2020.1 or later

An example of microcontroller we have been using is the Infineon XMC4800,
which has an ARM Cortex-M4 running at 144 MHz, with 2 MB Flash and 352 kB RAM.
It runs rt-kernel, and we have tested it with 9 Profinet slots each
having 8 digital inputs and 8 digital outputs (one bit each). The values are
sent and received each millisecond (PLC watchdog setting 3 ms).


Dependencies
------------
The p-net stack contains no third party components. Its external dependencies are:

* C-library
* An operating system (if used)

Tools used for building and documentation (not shipped in the resulting binaries):

* cmake (BSD 3-clause License)
* Sphinx (BSD license)


Contributions
--------------
Contributions are welcome. If you want to contribute you will need to
sign a Contributor License Agreement and send it to us either by
e-mail or by physical mail. More information is available
on [https://rt-labs.com/contribution](https://rt-labs.com/contribution).
