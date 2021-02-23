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

#include "mib/mib2_system.h"
#include "mib/lldp-mib.h"
#include "mib/lldp-ext-pno-mib.h"
#include "mib/lldp-ext-dot3-mib.h"
#include <lwip/apps/snmp.h>
#include <lwip/apps/snmp_mib2.h>
#include <lwip/apps/snmp_threadsync.h>

/* Global variable for SNMP server state */
pnal_snmp_t pnal_snmp;

/* Configure standard SNMP variables (MIB-II)
 *
 * NOTE: This uses a patched version of lwip where callback functions are
 * called when the MIB-II system variables are read or written.
 */
static void pnal_snmp_configure_mib2 (void)
{
   static const struct snmp_obj_id enterprise_oid = {
      .len = SNMP_DEVICE_ENTERPRISE_OID_LEN,
      .id = {1, 3, 6, 1, 4, 1, 24686}, /* Profinet */
   };

   snmp_threadsync_init (&snmp_mib2_lwip_locks, snmp_mib2_lwip_synchronizer);
   snmp_set_device_enterprise_oid (&enterprise_oid);
   snmp_mib2_system_set_callbacks (
      mib2_system_get_value,
      mib2_system_test_set_value,
      mib2_system_set_value);
}

int pnal_snmp_init (pnet_t * net, const pnal_cfg_t * pnal_cfg)
{
   static const struct snmp_mib * mibs[] = {
      &mib2,
      &lldpmib,
      &lldpxpnomib,
      &lldpxdot3mib,
   };

   /* Store reference to Profinet device instance */
   pnal_snmp.net = net;

   pnal_snmp_configure_mib2();
   snmp_set_mibs (mibs, LWIP_ARRAYSIZE (mibs));

   /* Start the SNMP server task */
   snmp_init();

   /* Success */
   return 0;
}
