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

#include "pnal.h"
#include "pnal_options.h"
#include "osal.h"
#include "options.h"
#include "osal_log.h"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-features.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "mib/lldpLocalSystemData.h"
#include "mib/lldpLocPortTable.h"
#include "mib/lldpConfigManAddrTable.h"
#include "mib/lldpLocManAddrTable.h"
#include "mib/lldpRemTable.h"
#include "mib/lldpXdot3LocPortTable.h"
#include "mib/lldpXdot3RemPortTable.h"
#include "mib/lldpXPnoLocTable.h"
#include "mib/lldpXPnoRemTable.h"
#include "mib/system_mib.h"

static void pnal_snmp_thread (void * arg)
{
   struct pnet * pnet = arg;

   snmp_disable_log();

   /* make us an agentx client. */
   netsnmp_enable_subagent();

   /* initialize the agent library */
   init_agent ("lldpMIB");

   /* init mib code */
   init_system_mib (pnet);
   init_lldpLocalSystemData (pnet);
   init_lldpLocPortTable (pnet);
   init_lldpConfigManAddrTable (pnet);
   init_lldpLocManAddrTable (pnet);
   init_lldpRemTable (pnet);
   init_lldpXdot3LocPortTable (pnet);
   init_lldpXdot3RemPortTable (pnet);
   init_lldpXPnoLocTable (pnet);
   init_lldpXPnoRemTable (pnet);

   /* read lldpMIB.conf files. */
   init_snmp ("lldpMIB");

   for (;;)
   {
      agent_check_and_process (1); /* 0 == don't block */
   }
}

int pnal_snmp_init (struct pnet * pnet)
{
   os_thread_create (
      "pn_snmp",
      PNET_SNMP_PRIO,
      PNET_SNMP_STACK_SIZE,
      pnal_snmp_thread,
      pnet);
   return 0;
}
