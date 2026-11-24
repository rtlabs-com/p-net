
/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2018 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#include "pf_includes.h"

#include <inttypes.h>

#ifdef UNIT_TEST
/* Background worker is disabled during unit tests.
 * Stack reinitialization during test recreates
 * the background task which cause problems.
 * Disabling is implemented using mock functions
 * - mock_pf_bg_worker_init
 * - mock_pf_bg_worker_start_job
 */
#endif

#undef BG_WORKER_LOG_JOBS

/* Events handled by bg worker task */

#define BG_JOB_EVENT_UPDATE_PORTS_STATUS                    BIT (0)
#define BG_JOB_EVENT_SAVE_ASE_NVM_DATA                      BIT (1)
#define BG_JOB_EVENT_SAVE_PDPORT_NVM_DATA                   BIT (2)
#define BG_JOB_EVENT_SAVE_SNMP_SYSCONTACT_NVM_DATA          BIT (3)
#define BG_JOB_EVENT_SAVE_SNMP_SYSNAME_NVM_DATA             BIT (4)
#define BG_JOB_EVENT_SAVE_SNMP_SYSLOCATION_NVM_DATA         BIT (5)
#define BG_JOB_EVENT_CLEAR_SNMP_SYSTEM_LOCATION_NVM_DATA    BIT (6)
#define BG_JOB_EVENT_CLEAR_IP_SETTING_NWM_DATA              BIT (7)
#define BG_JOB_EVENT_CLEAR_DIAGNOSTICS_NWM_DATA             BIT (8)
#define BG_JOB_EVENT_SAVE_SNMP_CONFIG                       BIT (9)
#define BG_JOB_EVENT_ACTIVATE_SNMP_CONFIG                   BIT (10)
#define BG_JOB_EVENT_SAVE_MRP_NVM_DATA                      BIT (11)
#define BG_JOB_EVENT_USER_CALLBACK                          BIT (12)

/* TODO: Move this to a header file.
 *
 * Note that pf_snmp.h(.c) is not the right place for this function.
 */
#if PNET_OPTION_SNMP && PNET_OPTION_SECURITY_LEVEL_1
extern int pf_snmp_activate_snmp_config (pnet_t * net);
#endif

static void bg_worker_task (void * arg);

void pf_bg_worker_init (pnet_t * net)
{
   net->pf_bg_worker.events = os_event_create();
   CC_ASSERT (net->pf_bg_worker.events != NULL);

   net->pf_bg_worker.user.events = os_event_create();
   CC_ASSERT (net->pf_bg_worker.user.events != NULL);

   os_thread_create (
      "p-net_bg_worker",
      net->fspm_cfg.bg_worker_thread.prio,
      net->fspm_cfg.bg_worker_thread.stack_size,
      bg_worker_task,
      (void *)net);
}

int pf_bg_worker_start_job (pnet_t * net, pf_bg_job_t job_id)
{
   if (net == NULL)
   {
      return -1;
   }

   switch (job_id)
   {
   case PF_BGJOB_UPDATE_PORTS_STATUS:
      os_event_set (net->pf_bg_worker.events, BG_JOB_EVENT_UPDATE_PORTS_STATUS);
      break;
   case PF_BGJOB_SAVE_ASE_NVM_DATA:
      os_event_set (net->pf_bg_worker.events, BG_JOB_EVENT_SAVE_ASE_NVM_DATA);
      break;
   case PF_BGJOB_SAVE_PDPORT_NVM_DATA:
      os_event_set (net->pf_bg_worker.events, BG_JOB_EVENT_SAVE_PDPORT_NVM_DATA);
      break;
   case PF_BGJOB_SAVE_SNMP_SYSTEM_CONTACT_NVM_DATA:
      os_event_set (
         net->pf_bg_worker.events,
         BG_JOB_EVENT_SAVE_SNMP_SYSCONTACT_NVM_DATA);
      break;
   case PF_BGJOB_SAVE_SNMP_SYSTEM_NAME_NVM_DATA:
      os_event_set (
         net->pf_bg_worker.events,
         BG_JOB_EVENT_SAVE_SNMP_SYSNAME_NVM_DATA);
      break;
   case PF_BGJOB_SAVE_SNMP_SYSTEM_LOCATION_NVM_DATA:
      os_event_set (
         net->pf_bg_worker.events,
         BG_JOB_EVENT_SAVE_SNMP_SYSLOCATION_NVM_DATA);
      break;
   case PF_BGJOB_CLEAR_SNMP_SYSTEM_LOCATION_NVM_DATA:
      os_event_set (
         net->pf_bg_worker.events,
         BG_JOB_EVENT_CLEAR_SNMP_SYSTEM_LOCATION_NVM_DATA);
      break;
   case PF_BGJOB_CLEAR_IP_SETTINGS_FILE:
      os_event_set (
         net->pf_bg_worker.events,
         BG_JOB_EVENT_CLEAR_IP_SETTING_NWM_DATA);
      break;
   case PF_BGJOB_CLEAR_DIAGNOSTICS_FILE:
      os_event_set (
         net->pf_bg_worker.events,
         BG_JOB_EVENT_CLEAR_DIAGNOSTICS_NWM_DATA);
      break;
   case PF_BGJOB_SAVE_SNMP_CONFIG:
      os_event_set (net->pf_bg_worker.events, BG_JOB_EVENT_SAVE_SNMP_CONFIG);
      break;
   case PF_BGJOB_ACTIVATE_SNMP_CONFIG:
      os_event_set (net->pf_bg_worker.events, BG_JOB_EVENT_ACTIVATE_SNMP_CONFIG);
      break;
   case PF_BGJOB_SAVE_MRP_DATA:
      os_event_set (net->pf_bg_worker.events, BG_JOB_EVENT_SAVE_MRP_NVM_DATA);
      break;
   case PF_BGJOB_USER_CALLBACK:
      os_event_set (net->pf_bg_worker.events, BG_JOB_EVENT_USER_CALLBACK);
      break;
   default:
      LOG_ERROR (
         PNET_LOG,
         "BGW(%d): Unsupported job %d\n",
         __LINE__,
         (int)job_id);
      return -1;
   }

   return 0;
}

void pf_bg_worker_set_user_events (pnet_t * net, uint32_t flags)
{
   os_event_set (net->pf_bg_worker.user.events, flags);
}

static void pf_bg_worker_clear_file (pnet_t * net, char * filename)
{
   pf_file_clear (pf_cmina_get_file_directory (net), filename);
}

/**
 * Event handling loop for background thread
 *
 * @param arg              InOut: Thread argument, must be of type pnet_t *
 */
static void bg_worker_task (void * arg)
{
   pnet_t * net = (pnet_t *)arg;
   uint32_t mask =
      BG_JOB_EVENT_UPDATE_PORTS_STATUS | BG_JOB_EVENT_SAVE_ASE_NVM_DATA |
      BG_JOB_EVENT_SAVE_PDPORT_NVM_DATA | BG_JOB_EVENT_SAVE_MRP_NVM_DATA |
      BG_JOB_EVENT_SAVE_SNMP_SYSCONTACT_NVM_DATA |
      BG_JOB_EVENT_SAVE_SNMP_SYSNAME_NVM_DATA |
      BG_JOB_EVENT_SAVE_SNMP_SYSLOCATION_NVM_DATA |
      BG_JOB_EVENT_CLEAR_IP_SETTING_NWM_DATA |
      BG_JOB_EVENT_CLEAR_DIAGNOSTICS_NWM_DATA | BG_JOB_EVENT_SAVE_SNMP_CONFIG |
      BG_JOB_EVENT_ACTIVATE_SNMP_CONFIG | BG_JOB_EVENT_USER_CALLBACK;
   uint32_t flags = 0;

#ifdef BG_WORKER_LOG_JOBS
   bool log_job_complete_msg = false;
   os_tick_t start, stop;
#endif

   for (;;)
   {
#ifdef BG_WORKER_LOG_JOBS
      if (log_job_complete_msg)
      {
         log_job_complete_msg = false;
         stop = os_tick_current();
         LOG_DEBUG (
            PNET_LOG,
            "BGW(%d): Job completed in %" PRIu32 " ticks\n",
            __LINE__,
            (uint32_t)stop - start);
      }
#endif

      os_event_wait (net->pf_bg_worker.events, mask, &flags, OS_WAIT_FOREVER);

#ifdef BG_WORKER_LOG_JOBS
      if (flags != BG_JOB_EVENT_UPDATE_PORTS_STATUS)
      {
         LOG_DEBUG (
            PNET_LOG,
            "BGW(%d): Job start (flags = 0x%" PRIx32 ")\n",
            __LINE__,
            flags);
         start = os_tick_current();
         log_job_complete_msg = true;
      }
#endif

      /* Several jobs may be queued. Run jobs that clear files first */
      if (flags & BG_JOB_EVENT_CLEAR_IP_SETTING_NWM_DATA)
      {
         os_event_clr (
            net->pf_bg_worker.events,
            BG_JOB_EVENT_CLEAR_IP_SETTING_NWM_DATA);

         pf_bg_worker_clear_file (net, PF_FILENAME_IP);
      }

      if (flags & BG_JOB_EVENT_CLEAR_DIAGNOSTICS_NWM_DATA)
      {
         os_event_clr (
            net->pf_bg_worker.events,
            BG_JOB_EVENT_CLEAR_DIAGNOSTICS_NWM_DATA);

         pf_bg_worker_clear_file (net, PF_FILENAME_DIAGNOSTICS);
      }
      if (flags & BG_JOB_EVENT_CLEAR_SNMP_SYSTEM_LOCATION_NVM_DATA)
      {
         os_event_clr (
            net->pf_bg_worker.events,
            BG_JOB_EVENT_CLEAR_SNMP_SYSTEM_LOCATION_NVM_DATA);

#if PNET_OPTION_SNMP
         pf_bg_worker_clear_file (net, PF_FILENAME_SNMP_SYSLOCATION);
#endif
      }
      if (flags & BG_JOB_EVENT_SAVE_ASE_NVM_DATA)
      {
         os_event_clr (net->pf_bg_worker.events, BG_JOB_EVENT_SAVE_ASE_NVM_DATA);

         pf_cmina_save_ase (net, &net->cmina_nonvolatile_dcp_ase);
      }
      if (flags & BG_JOB_EVENT_SAVE_PDPORT_NVM_DATA)
      {
         os_event_clr (
            net->pf_bg_worker.events,
            BG_JOB_EVENT_SAVE_PDPORT_NVM_DATA);

         (void)pf_pdport_save_all (net);
      }
      if (flags & BG_JOB_EVENT_UPDATE_PORTS_STATUS)
      {
         os_event_clr (
            net->pf_bg_worker.events,
            BG_JOB_EVENT_UPDATE_PORTS_STATUS);

         pf_pdport_update_eth_status (net);
      }
      if (flags & BG_JOB_EVENT_SAVE_SNMP_SYSCONTACT_NVM_DATA)
      {
         os_event_clr (
            net->pf_bg_worker.events,
            BG_JOB_EVENT_SAVE_SNMP_SYSCONTACT_NVM_DATA);

#if PNET_OPTION_SNMP
         pf_snmp_save_system_contact (net);
#endif
      }
      if (flags & BG_JOB_EVENT_SAVE_SNMP_SYSNAME_NVM_DATA)
      {
         os_event_clr (
            net->pf_bg_worker.events,
            BG_JOB_EVENT_SAVE_SNMP_SYSNAME_NVM_DATA);

#if PNET_OPTION_SNMP
         pf_snmp_save_system_name (net);
#endif
      }
      if (flags & BG_JOB_EVENT_SAVE_SNMP_SYSLOCATION_NVM_DATA)
      {
         os_event_clr (
            net->pf_bg_worker.events,
            BG_JOB_EVENT_SAVE_SNMP_SYSLOCATION_NVM_DATA);

#if PNET_OPTION_SNMP
         pf_snmp_save_system_location (net);
#endif
      }
      if (flags & BG_JOB_EVENT_SAVE_SNMP_CONFIG)
      {
         os_event_clr (net->pf_bg_worker.events, BG_JOB_EVENT_SAVE_SNMP_CONFIG);
#if PNET_OPTION_SNMP && PNET_OPTION_SECURITY_LEVEL_1
         pf_snmp_save_config (net);
#endif
      }
      if (flags & BG_JOB_EVENT_ACTIVATE_SNMP_CONFIG)
      {
         os_event_clr (
            net->pf_bg_worker.events,
            BG_JOB_EVENT_ACTIVATE_SNMP_CONFIG);
#if PNET_OPTION_SNMP && PNET_OPTION_SECURITY_LEVEL_1
         pf_snmp_activate_snmp_config (net);
#endif
      }
      if (flags & BG_JOB_EVENT_SAVE_MRP_NVM_DATA)
      {
         os_event_clr (net->pf_bg_worker.events, BG_JOB_EVENT_SAVE_MRP_NVM_DATA);
#if PNET_OPTION_MRP
         pf_mrp_save_config (net);
#endif
      }
      if (flags & BG_JOB_EVENT_USER_CALLBACK)
      {
         uint32_t user_flags = 0;

         os_event_clr (net->pf_bg_worker.events, BG_JOB_EVENT_USER_CALLBACK);
         os_event_wait (
            net->pf_bg_worker.user.events,
            UINT32_MAX,
            &user_flags,
            0);
         if (user_flags != 0)
         {
            os_event_clr (net->pf_bg_worker.user.events, user_flags);
            if (net->fspm_cfg.bg_job_cb != NULL)
            {
               net->fspm_cfg.bg_job_cb (net, net->fspm_cfg.cb_arg, user_flags);
            }
         }
      }
   }
}
