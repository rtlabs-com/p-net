#********************************************************************
#        _       _         _
#  _ __ | |_  _ | |  __ _ | |__   ___
# | '__|| __|(_)| | / _` || '_ \ / __|
# | |   | |_  _ | || (_| || |_) |\__ \
# |_|    \__|(_)|_| \__,_||_.__/ |___/
#
# www.rt-labs.com
# Copyright 2020 rt-labs AB, Sweden.
#
# This software is licensed under the terms of the BSD 3-clause
# license. See the file LICENSE distributed with this software for
# full license information.
#*******************************************************************/


cmake_minimum_required(VERSION 3.14)

# Attempt to find externally built mera library
find_package(mera QUIET)

if (NOT mera_FOUND)
  # Download and build mera locally as a static library
  # Todo: this is a private repo. Switch to public repo before release
  message(STATUS "Fetch mera from github")
  include(FetchContent)
  FetchContent_Declare(
    mera
    GIT_REPOSITORY      https://github.com/microchip-ung/rtlabs-mera
    GIT_TAG             b9d43d5
    )

  FetchContent_GetProperties(mera)
  if(NOT mera_POPULATED)
    FetchContent_Populate(mera)
    set(BUILD_SHARED_LIBS_OLD ${BUILD_SHARED_LIBS})
    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
    add_subdirectory(${mera_SOURCE_DIR} ${mera_BINARY_DIR} EXCLUDE_FROM_ALL)
    set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_OLD} CACHE BOOL "" FORCE)
  endif()

endif()
