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

set(OSAL_SOURCES
  ${PROFINET_SOURCE_DIR}/src/osal/windows/osal.c
  ${PROFINET_SOURCE_DIR}/src/osal/windows/osal_eth.c
  ${PROFINET_SOURCE_DIR}/src/osal/windows/osal_udp.c
  )
set(OSAL_INCLUDES
  ${PROFINET_SOURCE_DIR}/src/osal/windows
  )
set(OSAL_LIBS
  ws2_32
  )

set(GOOGLE_TEST_INDIVIDUAL TRUE)

# Common options. /wd4200 should disable irrelevant warning about
# zero-sized array in frame.h but it doesn't seem to work.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D_CRT_SECURE_NO_WARNINGS")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4200 /D_CRT_SECURE_NO_WARNINGS /wd4204 /wd4100 /wd4127 /wd4201")

# GTest wants /MT
 set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        )
foreach(CompilerFlag ${CompilerFlags})
  string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
endforeach()

#set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
#set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
