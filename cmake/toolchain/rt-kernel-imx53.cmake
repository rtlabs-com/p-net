#********************************************************************
#        _       _         _
#  _ __ | |_  _ | |  __ _ | |__   ___
# | '__|| __|(_)| | / _` || '_ \ / __|
# | |   | |_  _ | || (_| || |_) |\__ \
# |_|    \__|(_)|_| \__,_||_.__/ |___/
#
# www.rt-labs.com
# Copyright 2018 rt-labs AB, Sweden.
#
# This software is dual-licensed under GPLv3 and a commercial
# license. See the file LICENSE.md distributed with this software for
# full license information.
#*******************************************************************/

# the name of the target operating system
set(CMAKE_SYSTEM_NAME rt-kernel)

# which compiler to use
set(CMAKE_C_COMPILER $ENV{COMPILERS}/arm-eabi/bin/arm-eabi-gcc)
set(CMAKE_CXX_COMPILER $ENV{COMPILERS}/arm-eabi/bin/arm-eabi-gcc)
if(CMAKE_HOST_WIN32)
  set(CMAKE_C_COMPILER ${CMAKE_C_COMPILER}.exe)
  set(CMAKE_CXX_COMPILER ${CMAKE_CXX_COMPILER}.exe)
endif(CMAKE_HOST_WIN32)

set(ARCH imx53)
set(CPU cortex-a8)

add_definitions(-mfpu=neon -mcpu=cortex-a8)
