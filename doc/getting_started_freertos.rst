Getting started using STM32Cube / FreeRTOS / lwIP
=================================================

This tutorial uses the STM32F769i discovery kit, see
https://www.st.com/en/evaluation-tools/32f769idiscovery.html

Preparations
------------
Run the build on a Linux x86 machine. The build chain uses an ARM cross compiler. Install it::

   sudo apt install gcc-arm-none-eabi

Install ST-link on your Linux machine to have access to the serial port of the device.


Cloning STM32Cube firmware
--------------------------

First, clone the relevant STM32Cube repo for your platform. For
instance, to build for STM32F7 (fetching a specific tag only)::

    git clone https://github.com/STMicroelectronics/STM32CubeF7.git \
        -b v1.16.1 --single-branch --depth 1

Download and compile p-net
--------------------------
Clone the source::

    git clone --recurse-submodules https://github.com/rtlabs-com/p-net.git

This will clone the repository with submodules. If you already cloned
the repository without the ``--recurse-submodules`` flag then run this
in the ``p-net`` folder::

    git submodule update --init --recursive

Then, when configuring specify CPU, board and path to the cloned git
repository::

    CPU=cortex-m7fd BOARD=STM32F769I-DISCO CUBE_DIR=/path/to/cube cmake \
        -B build.cube \
        -DCMAKE_TOOLCHAIN_FILE=cmake/tools/toolchain/stm32cube.cmake \
        -G "Unix Makefiles"
    cmake --build build.cube

When the build completes you can find the sample-app binary in
``build.cube/pn_dev.bin``. Flash this binary to your board by
uploading it to the USB disk as usual for STM32 boards.

A serial port will also be available via the USB cable.
