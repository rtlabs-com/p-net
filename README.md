p-net Profinet device stack
===========================

Web resources
-------------

* Source repository: [https://github.com/rtlabs-com/p-net](https://github.com/rtlabs-com/p-net)
* Documentation: [https://rt-labs.com/docs/p-net](https://rt-labs.com/docs/p-net)
* Continuous integration: [https://travis-ci.org/rtlabs-com/p-net](https://travis-ci.org/rtlabs-com/p-net)
* rt-labs: [https://rt-labs.com](https://rt-labs.com)

[![Build Status](https://travis-ci.org/rtlabs-com/p-net.svg?branch=master)](https://travis-ci.org/rtlabs-com/p-net)

p-net
-----
The rt-labs Profinet stack p-net is used for Profinet device
implementations. It is easy to use and provides a small footprint. It
is especially well suited for embedded systems where resources are
limited and efficiency is crucial.

It is written in C and can be run on bare-metal hardware, an RTOS such
as rt-kernel, or on Linux. The main requirement is that the
platform can send and receive raw Ethernet Layer 2 frames. The
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

Limitations:

* IPv4 only
* Only a single Ethernet interface (no media redundancy)
* Only one stack instance

This software is dual-licensed, with GPL version 3 and a commercial license.
See LICENSE.md for more details.


Requirements
------------
The platform must be able to send and receive raw Ethernet Layer 2 frames,
and the Ethernet driver must be able to handle full size frames. It
should also avoid copying data, for performance reasons.

* cmake 3.13 or later

For Linux:

* gcc 4.6 or later
* See the "Real-time properties of Linux" page in the documentation on how to
  improve Linux timing

For rt-kernel:

* Workbench 2017.1 or later


Contributions
--------------
Contributions are welcome. If you want to contribute you will need to
sign a Contributor License Agreement and send it to us either by
e-mail or by physical mail. More information is available
on [https://rt-labs.com/contribution](https://rt-labs.com/contribution).
