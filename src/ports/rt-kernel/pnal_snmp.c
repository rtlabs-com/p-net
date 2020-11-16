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

#include "options.h"
#include "pnet_api.h"
#include "osal.h"
#include "osal_log.h"
#include "pnal.h"
#include "pf_types.h"
#include "pnal_snmp.h"

#include "lldp-mib.h"
#include "lldp-ext-pno-mib.h"
#include "lldp-ext-dot3-mib.h"

#include <lwip/apps/snmp.h>
#include <lwip/apps/snmp_mib2.h>
#include <lwip/apps/snmp_threadsync.h>

/* Global variable for SNMP server state */
pnal_snmp_t pnal_snmp;

/**
 * Called right after writing a variable
 *
 * This is a callback function for SNMP in lwip. It conforms to
 * the type snmp_write_callback_fct.
 *
 * @param oid              In:    Object ID array, e.g. 1.3.6.1.2.1.1.5.0 for
 *                                sysName.
 * @param oid_len          In:    Number of elements in array.
 * @param callback_arg     InOut: Argument configured in call to
 *                                snmp_set_write_callback().
 */
static void pnal_snmp_write_callback (
   const u32_t * oid,
   u8_t oid_len,
   void * callback_arg)
{
   (void)pf_snmp_set_system_name (pnal_snmp.net, &pnal_snmp.sysname);
   (void)pf_snmp_set_system_contact (pnal_snmp.net, &pnal_snmp.syscontact);
   (void)pf_snmp_set_system_location (pnal_snmp.net, &pnal_snmp.syslocation);
}

/* Configure standard SNMP variables (MIB-II) */
static void pnal_snmp_configure_mib2 (void)
{
   static const struct snmp_obj_id enterprise_oid = {
      .len = SNMP_DEVICE_ENTERPRISE_OID_LEN,
      .id = {1, 3, 6, 1, 4, 1, 24686}, /* Profinet */
   };

   snmp_threadsync_init (&snmp_mib2_lwip_locks, snmp_mib2_lwip_synchronizer);

   /* Set buffers to hold the writable variables */
   snmp_mib2_set_sysname (
      (u8_t *)pnal_snmp.sysname.string,
      NULL,
      sizeof (pnal_snmp.sysname.string) - 1);
   snmp_mib2_set_syscontact (
      (u8_t *)pnal_snmp.syscontact.string,
      NULL,
      sizeof (pnal_snmp.syscontact.string) - 1);
   snmp_mib2_set_syslocation (
      (u8_t *)pnal_snmp.syslocation.string,
      NULL,
      sizeof (pnal_snmp.syslocation.string) - 1);

   snmp_mib2_set_sysdescr ((const u8_t *)pnal_snmp.sysdescr.string, NULL);

   snmp_set_device_enterprise_oid (&enterprise_oid);
}

int os_snmp_init (pnet_t * net)
{
   static const struct snmp_mib * mibs[] = {
      &mib2,
      &lldpmib,
      &lldpxpnomib,
      &lldpxdot3mib,
   };

   /* Store reference to Profinet device instance */
   pnal_snmp.net = net;

   /* Get values to use for system variables in MIB-II */
   pf_snmp_get_system_name (net, &pnal_snmp.sysname);
   pf_snmp_get_system_contact (net, &pnal_snmp.syscontact);
   pf_snmp_get_system_location (net, &pnal_snmp.syslocation);
   pf_snmp_get_system_description (net, &pnal_snmp.sysdescr);

   pnal_snmp_configure_mib2();
   snmp_set_mibs (mibs, LWIP_ARRAYSIZE (mibs));
   snmp_set_write_callback (pnal_snmp_write_callback, net);

   /* Start the SNMP server task */
   snmp_init();

   /* Success */
   return 0;
}
