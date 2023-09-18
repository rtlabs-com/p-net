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
#include "osal.h"
#include "options.h"
#include "osal_log.h"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-features.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "mib/lldpConfigManAddrTable.h"
#include "mib/lldpLocalSystemData.h"
#include "mib/lldpLocManAddrTable.h"
#include "mib/lldpLocPortTable.h"
#include "mib/lldpRemManAddrTable.h"
#include "mib/lldpRemTable.h"
#include "mib/lldpXdot3LocPortTable.h"
#include "mib/lldpXdot3RemPortTable.h"
#include "mib/lldpXPnoLocTable.h"
#include "mib/lldpXPnoRemTable.h"
#include "mib/system_mib.h"

static void pnal_snmp_thread (void * arg)
{
   struct pnet * pnet = arg;

#ifndef USE_SYSLOG
   snmp_disable_log();
#endif

   /* make us an agentx client. */
   netsnmp_enable_subagent();

   if (pnet->snmp_agentx_socket)
   {
      netsnmp_ds_set_string(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_X_SOCKET,
			    pnet->snmp_agentx_socket);
   }

   /* initialize the agent library */
   init_agent ("lldpMIB");

   /* init mib code */
   init_system_mib (pnet);
   init_lldpLocalSystemData (pnet);
   init_lldpLocPortTable (pnet);
   init_lldpConfigManAddrTable (pnet);
   init_lldpLocManAddrTable (pnet);
   init_lldpRemManAddrTable (pnet);
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

int pnal_snmp_init (struct pnet * pnet, const pnal_cfg_t * pnal_cfg)
{
   pnet->snmp_agentx_socket = pnal_cfg->snmp_agentx_socket;
   os_thread_create (
      "pn_snmp",
      pnal_cfg->snmp_thread.prio,
      pnal_cfg->snmp_thread.stack_size,
      pnal_snmp_thread,
      pnet);
   return 0;
}
