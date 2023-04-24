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

add_executable(pn_setpoint_from_button "")
set_target_properties(profinet pn_setpoint_from_button
        PROPERTIES
        C_STANDARD 99
        )

target_include_directories(pn_setpoint_from_button
        PRIVATE
        .
        ${PROFINET_SOURCE_DIR}/src/ports/linux
        ${PROFINET_SOURCE_DIR}/src
        ${PROFINET_BINARY_DIR}/src
        )

target_sources(pn_setpoint_from_button
        PRIVATE
        app_data.c
        app_gsdml.c
        app_log.c
        app_utils.c
        sampleapp_common.c
        utils.c
        ${PROFINET_SOURCE_DIR}/src/ports/linux/sampleapp_main.c
        )

target_compile_options(pn_setpoint_from_button
        PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wno-unused-parameter
        -ffunction-sections
        -fdata-sections
        )

target_link_options(pn_setpoint_from_button
        PRIVATE
        -Wl,--gc-sections
        )

# TODO not sure if this one is required
file(COPY ${PROFINET_SOURCE_DIR}/src/ports/linux/set_network_parameters DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)

target_link_libraries(pn_setpoint_from_button PUBLIC profinet)

install(TARGETS pn_setpoint_from_button DESTINATION bin)