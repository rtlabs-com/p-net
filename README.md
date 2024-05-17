# P-net

Profinet device stack for embedded systems

## Web resources

- Source repository: [https://github.com/rtlabs-com/p-net](https://github.com/rtlabs-com/p-net)
- Documentation: [https://rt-labs.com/docs/p-net](https://rt-labs.com/docs/p-net)
- Continuous integration: [https://github.com/rtlabs-com/p-net/actions](https://github.com/rtlabs-com/p-net/actions)
- RT-Labs (stack integration, certification services and training): [https://rt-labs.com](https://rt-labs.com)

[![Build Status](https://github.com/rtlabs-com/p-net/workflows/Build/badge.svg?branch=master)](https://github.com/rtlabs-com/p-net/actions?workflow=Build)
[![CodeQL](https://github.com/rtlabs-com/p-net/workflows/CodeQL/badge.svg?branch=master)](https://github.com/rtlabs-com/p-net/actions?workflow=CodeQL)

## Key features

- Profinet v2.43
    - Conformance Class A and B
    - Real Time Class 1
    - Multiple Ethernet ports


- Easy to use
  - Extensive documentation and instructions on how to get started.
  - Build and run sample application on Raspberry Pi in 30 minutes.
- Portable
  - Written in C.
  - Linux, RTOS or bare metal.
  - Sources for supported port layers provided.

The RT-Labs Profinet stack p-net is used for Profinet device
implementations. It is easy to use and provides a small footprint. It
is especially well suited for embedded systems where resources are
limited and efficiency is crucial.
The stack is supplied with full sources including porting
layers and a sample application.

Also C++ (any version) is supported for application development.

The main requirement on the platform
is that it can send and receive raw Ethernet Layer 2 frames.

This software is dual-licensed, with GPL version 3 and a commercial license.
If you intend to use this stack in a commercial product, you likely need to
buy a license. See LICENSE.md for more details.

## Getting started

See the tutorial in the documentation: [https://docs.rt-labs.com/p-net/running_sample_app.html](https://docs.rt-labs.com/p-net/running_sample_app.html)

Note that you need to include submodules when cloning:

```
git clone --recurse-submodules https://github.com/rtlabs-com/p-net.git
```

## Features

- Multiple Ethernet ports (for Linux only, so far)
- TCP/IP
- LLDP
- SNMP
- RT (real-time)
- Address resolution
- Parameterization
- Process IO data exchange
- Alarm handling
- Configurable number of modules and sub-modules
- Bare-metal or OS
- Porting layer provided
- Supports I&M0 - I&M4. The I&M data is supported for the device, but not for
  individual modules.
- Shared device (connection to multiple controllers)

## Limitations

- This is a device stack, which means that the IO-controller/master/PLC side is
  not supported.
- No media redundancy (No MRP support)
- Legacy startup mode is not fully implemented
- No support for RT_CLASS_UDP
- No support for DHCP
- No fast start-up
- No MC multicast device-to-device
- No support for shared inputs
- Supports only full connections, not the limited "DeviceAccess" connection type.
- No iPar (parameter server) support
- No support for time synchronization
- No UDP frames at alarm (just the default alarm mechanism is implemented)
- No ProfiDrive or ProfiSafe profiles.

## Requirements

The platform must be able to send and receive raw Ethernet Layer 2 frames,
and the Ethernet driver must be able to handle full size frames. It
should also avoid copying data, for performance reasons.

- cmake 3.14 or later

For Linux:

- gcc 4.6 or later
- See the "Real-time properties of Linux" page in the documentation on how to
  improve Linux timing

For rt-kernel:

- Workbench 2020.1 or later

An example of microcontroller we have been using is the Infineon XMC4800,
which has an ARM Cortex-M4 running at 144 MHz, with 2 MB Flash and 352 kB RAM.
It runs rt-kernel, and we have tested it with 9 Profinet slots each
having 8 digital inputs and 8 digital outputs (one bit each). The values are
sent and received each millisecond (PLC watchdog setting 3 ms).

## Dependencies

Some of the platform-dependent parts are located in the OSAL repository and the
cmake-tools repository.

- [https://github.com/rtlabs-com/osal](https://github.com/rtlabs-com/osal)
- [https://github.com/rtlabs-com/cmake-tools](https://github.com/rtlabs-com/cmake-tools)

Those are downloaded automatically during install.

The p-net stack contains no third party components. Its external dependencies are:

- C-library
- An operating system (if used)
- For conformance class B you need an SNMP implementation. On Linux is
  net-snmp (BSD License) used [http://www.net-snmp.org](http://www.net-snmp.org)

Tools used for building, testing and documentation (not shipped in the resulting binaries):

- cmake (BSD 3-clause License) [https://cmake.org](https://cmake.org)
- gtest (BSD-3-Clause License) [https://github.com/google/googletest](https://github.com/google/googletest)
- Sphinx (BSD license) [https://www.sphinx-doc.org](https://www.sphinx-doc.org)
- Doxygen (GPL v2) [https://www.doxygen.nl](https://www.doxygen.nl/index.html)
- clang-format (Apache License 2.0) [https://clang.llvm.org](https://clang.llvm.org/docs/ClangFormat.html)

## Contributions

Contributions are welcome. If you want to contribute you will need to
sign a Contributor License Agreement and send it to us either by
e-mail or by physical mail. More information is available
on [https://rt-labs.com/contribution](https://rt-labs.com/contribution).
