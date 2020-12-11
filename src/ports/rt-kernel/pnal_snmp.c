/*
-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

p-net Profinet device stack
===========================

Copyright 2018 rt-labs AB

This instance of p-net is licensed for use in a specific product, as
follows:

Company: Bender GmbH & Co. KG
Product: COMTRAXX
Product description: COMTRAXX products provided by Bender GmbH & Co. KG

Use of this instance of p-net in other products is not allowed.

You are hereby given a non-exclusive, non-transferable license to use
the SOFTWARE and documentation according to the SOFTWARE LICENSE
AGREEMENT document shipped together with this instance of the
SOFTWARE. The SOFTWARE LICENSE AGREEMENT prohibits copying and
redistribution of the SOFTWARE.  It prohibits reverse-engineering
activities.  It limits warranty and liability on behalf of rt-labs.
The details of these terms are explained in subsequent chapters of the
SOFTWARE LICENSE AGREEMENT.
-----BEGIN PGP SIGNATURE-----

iQEzBAEBCgAdFiEE1e4U+gPgGJXLsxxLZlG0bopmiXIFAl/TMvQACgkQZlG0bopm
iXJE1QgAlEKblKkIrLF+hVRm6I1IfbGV6VWnBx+9IPwaHZmgQZAVTOlp4RHiNAre
Il1l1xheENWpuihbVlvz5hMCVto4fkT13IUXxOo9Va9MK8lm6JOu2lYpl1HgVIwV
R6I6dsWLeNBSBaA38DDBHIfjP4LBZC/HVYO2ISc3/OKutqasz/WIp+AjAURiKRsf
yaWGay88FITHymmqJpGaWzB+jAOvRFs8hW/GsOyX/r17AK5F5N51O+UqUTKAWNPT
f6M8QF0b07Hw+eOuB7A/0D5nvSl/rOd9LvYkiUhIUmQdHqERNkF9EJdroIjxTMXi
lmoOkPEqyGV58fh7JjMYlI60sPlGBw==
=TaN+
-----END PGP SIGNATURE-----
*/

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

int pnal_snmp_init (pnet_t * net)
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
