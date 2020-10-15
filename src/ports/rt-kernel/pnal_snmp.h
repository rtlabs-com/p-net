/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2020 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

/**
 * @file
 * @brief SNMP server for rt-kernel
 *
 * Uses the SNMP server implementation supplied by lwip.
 * Access to Profinet stack is done through the platform-independent internal
 * API, pf_snmp.h. Supported MIBs:
 * - MIB-II,
 * - LLDP-MIB,
 * - LLDP-EXT-DOT3-MIB (TODO),
 * - LLDP-EXT-PNO-MIB (TODO).
 *
 * Note that the rt-kernel tree needs to be patched to support SNMP etc.
 * See supplied patch file.
 */

#ifndef PNAL_SNMP_H
#define PNAL_SNMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pf_snmp.h"

/**
 * SNMP server state
 */
typedef struct pnal_snmp
{
   /* The p-net stack instance.
    *
    * Used for accessing variables in the stack, such as LLDP variables.
    */
   pnet_t * net;

   pf_snmp_system_name_t sysname;
   pf_snmp_system_contact_t syscontact;
   pf_snmp_system_location_t syslocation;
   pf_snmp_system_description_t sysdescr;
} pnal_snmp_t;

/** Global variable containing SNMP server state */
extern pnal_snmp_t pnal_snmp;

#ifdef __cplusplus
}
#endif

#endif /* PNAL_SNMP_H */
