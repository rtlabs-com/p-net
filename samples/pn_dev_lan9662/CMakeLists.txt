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

target_include_directories(pn_lan9662
  PRIVATE
  ${PROFINET_SOURCE_DIR}/src
  ${PROFINET_SOURCE_DIR}/src/drivers/lan9662
  ${PROFINET_BINARY_DIR}/src
  )


target_link_libraries (pn_lan9662 PUBLIC profinet mera)

install (TARGETS pn_lan9662 DESTINATION bin)
