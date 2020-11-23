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

static void pnal_snmp_thread (void * arg)
{
   struct pnet * pnet = arg;

   /* make us an agentx client. */
   netsnmp_enable_subagent();

   /* initialize the agent library */
   init_agent ("lldpMIB");

   /* init lldpMIB mib code */
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
