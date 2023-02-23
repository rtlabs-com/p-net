#********************************************************************
#        _       _         _
#  _ __ | |_  _ | |  __ _ | |__   ___
# | '__|| __|(_)| | / _` || '_ \ / __|
# | |   | |_  _ | || (_| || |_) |\__ \
# |_|    \__|(_)|_| \__,_||_.__/ |___/
#
# http://www.rt-labs.com
# Copyright 2017 rt-labs AB, Sweden.
# See LICENSE file in the project root for full license information.
#*******************************************************************/


add_executable(pn_dev_counter "")
set_target_properties (profinet pn_dev_counter
        PROPERTIES
        C_STANDARD 99
        )

target_include_directories(pn_dev_counter
        PRIVATE
        .
        ${PROFINET_SOURCE_DIR}/src/ports/linux
        ${PROFINET_SOURCE_DIR}/src
        ${PROFINET_BINARY_DIR}/src
        )

target_sources(pn_dev_counter
        PRIVATE
        app_data.c
        app_gsdml.c
        app_log.c
        app_utils.c
        sampleapp_common.c
        ${PROFINET_SOURCE_DIR}/src/ports/linux/sampleapp_main.c
        )

target_compile_options(pn_dev_counter
        PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wno-unused-parameter
        -ffunction-sections
        -fdata-sections
        )

target_link_options(pn_dev_counter
        PRIVATE
        -Wl,--gc-sections
        )


target_link_libraries (pn_dev_counter PUBLIC profinet)

install (TARGETS pn_dev_counter DESTINATION bin)
