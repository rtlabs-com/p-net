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

#ifdef UNIT_TEST

#endif

#include "pf_includes.h"

#define PF_CMIO_TIMER_PERIOD           (100*1000)  /* us */

static const char          *cmio_sched_name = "cmio";

/**
 * @internal
 * Return a string representation of the CMIO state.
 * @param state            In:   The state.
 * @return  A string representation of the CMIO state.
 */
static const char *pf_cmio_state_to_string(
   pf_cmio_state_values_t  state)
{
   const char *s = "<unknown>";

   switch (state)
   {
   case PF_CMIO_STATE_IDLE:
      s = "PF_CMIO_STATE_IDLE";
      break;
   case PF_CMIO_STATE_STARTUP:
      s = "PF_CMIO_STATE_STARTUP";
      break;
   case PF_CMIO_STATE_WDATA:
      s = "PF_CMIO_STATE_WDATA";
      break;
   case PF_CMIO_STATE_DATA:
      s = "PF_CMIO_STATE_DATA";
      break;
   }

   return s;
}

void pf_cmio_show(
   pf_ar_t                 *p_ar)
{
   printf("CMIO state            = %s\n", pf_cmio_state_to_string(p_ar->cmio_state));
   printf("     timer            = %u\n", (unsigned)p_ar->cmio_timer);
   printf("     timer run        = %u\n", (unsigned)p_ar->cmio_timer_run);
}

/**
 * @internal
 * @param p_ar             In:   The AR instance.
 * @param state            In:   The new CMIO state.
 */
static void pf_cmio_set_state(
   pf_ar_t                 *p_ar,
   pf_cmio_state_values_t  state)
{
   if (state != p_ar->cmio_state)
   {
      LOG_INFO(PNET_LOG, "CMIO(%d): New state %s\n", __LINE__, pf_cmio_state_to_string(state));
      p_ar->cmio_state = state;
   }
}

/**
 * @internal
 * Poll each output IOCR (CPM) if they have received data from the controller.
 *
 * This is a callback for the scheduler. Arguments should fulfill pf_scheduler_timeout_ftn_t
 *
 * When in state PF_CMIO_STATE_WDATA this function polls (every 100ms) all CPM
 * to see if data has arrived from the controller.
 * If CPM data has arrived in state PF_CMIO_STATE_WDATA then notify CMDEV, else
 * schedule another call in 100ms.
 * @param net              InOut: The p-net stack instance
 * @param arg              In:   The AR instance.
 * @param current_time     In:   The current time.
 */
static void pf_cmio_timer_expired(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                current_time)
{
   pf_ar_t                 *p_ar = (pf_ar_t *)arg;
   uint16_t                crep;
   bool                    data_possible = true;

   if ((p_ar->cmio_state == PF_CMIO_STATE_WDATA) &&
       (p_ar->cmio_timer_run == true))
   {
      for (crep = 0; crep < p_ar->nbr_iocrs; crep++)
      {
         if ((p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_OUTPUT) ||
             (p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_MC_CONSUMER))
         {
            if (p_ar->iocrs[crep].cpm.cmio_start == false)
            {
               data_possible = false;
            }
         }
      }
      pf_cmdev_cmio_info_ind(net, p_ar, data_possible);
      pf_scheduler_add(net, PF_CMIO_TIMER_PERIOD,
         cmio_sched_name, pf_cmio_timer_expired, p_ar, &p_ar->cmio_timer);
   }
   else
   {
      p_ar->cmio_timer_run = false;
      p_ar->cmio_timer = UINT32_MAX;
   }
}

int pf_cmio_cmdev_state_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_event_values_t     event)
{
   uint16_t                crep;

   switch (p_ar->cmio_state)
   {
   case PF_CMIO_STATE_IDLE:
      if (event == PNET_EVENT_ABORT)
      {
         /* Ignore */
         p_ar->cmio_timer_run = false;
         p_ar->cmio_timer = UINT32_MAX;

         pf_cmio_set_state(p_ar, PF_CMIO_STATE_STARTUP);
      }
      else if (event == PNET_EVENT_STARTUP)
      {
         /*
          * CMDEV has received a connect request.
          * Start supervision of the data flow.
          */
         for (crep = 0; crep < p_ar->nbr_iocrs; crep++)
         {
            if ((p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_OUTPUT) ||
                (p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_MC_CONSUMER))
            {
               p_ar->iocrs[crep].cpm.cmio_start = false;
            }
         }
         p_ar->cmio_timer_run = false;
         p_ar->cmio_timer = UINT32_MAX;

         pf_cmio_set_state(p_ar, PF_CMIO_STATE_STARTUP);
      }
      break;
   case PF_CMIO_STATE_STARTUP:
      if (event == PNET_EVENT_ABORT)
      {
         p_ar->cmio_timer_run = false;
         p_ar->cmio_timer = UINT32_MAX;

         pf_cmio_set_state(p_ar, PF_CMIO_STATE_IDLE);
      }
      else if (event == PNET_EVENT_STARTUP)
      {
         for (crep = 0; crep < p_ar->nbr_iocrs; crep++)
         {
            if ((p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_OUTPUT) ||
                (p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_MC_CONSUMER))
            {
               p_ar->iocrs[crep].cpm.cmio_start = false;
            }
         }
         pf_cmio_set_state(p_ar, PF_CMIO_STATE_WDATA);
      }
      else if (event == PNET_EVENT_PRMEND)
      {
         p_ar->cmio_timer_run = true;
         pf_scheduler_add(net, PF_CMIO_TIMER_PERIOD,
            cmio_sched_name, pf_cmio_timer_expired, p_ar, &p_ar->cmio_timer);

         pf_cmio_set_state(p_ar, PF_CMIO_STATE_WDATA);
      }
      break;
   case PF_CMIO_STATE_WDATA:
      if (event == PNET_EVENT_ABORT)
      {
         p_ar->cmio_timer_run = false;
         p_ar->cmio_timer = UINT32_MAX;

         pf_cmio_set_state(p_ar, PF_CMIO_STATE_IDLE);
      }
      else if (event == PNET_EVENT_DATA)
      {
         p_ar->cmio_timer_run = false;
         p_ar->cmio_timer = UINT32_MAX;

         pf_cmio_set_state(p_ar, PF_CMIO_STATE_DATA);
      }
      break;
   case PF_CMIO_STATE_DATA:
      if (event == PNET_EVENT_ABORT)
      {
         p_ar->cmio_timer_run = false;
         p_ar->cmio_timer = UINT32_MAX;

         pf_cmio_set_state(p_ar, PF_CMIO_STATE_IDLE);
      }
      break;
   }

   return 0;
}

int pf_cmio_cpm_state_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint16_t                crep,
   bool                    start)
{
   int                     ret = 0;

   switch (p_ar->cmio_state)
   {
   case PF_CMIO_STATE_IDLE:
   case PF_CMIO_STATE_STARTUP:
      /* Ignore */
      break;
   case PF_CMIO_STATE_WDATA:
      if (crep < p_ar->nbr_iocrs)
      {
         if (start != p_ar->iocrs[crep].cpm.cmio_start)
         {
            LOG_INFO(PNET_LOG, "CMIO(%d): CPM State Ind (crep=%u) start=%s\n", __LINE__, crep, start?"true":"false");
         }
         p_ar->iocrs[crep].cpm.cmio_start = start;
      }
      else
      {
         ret = -1;
      }
      break;
   case PF_CMIO_STATE_DATA:
      if (start == false)
      {
         LOG_INFO(PNET_LOG, "CMIO(%d): CPM State Ind (crep=%u) start=%s\n", __LINE__, crep, start?"true":"false");

         /* if (crep != crep.mcpm) not possible - handled elsewhere */
         (void)pf_cmdev_state_ind(net, p_ar, PNET_EVENT_ABORT);
      }
      break;
   }

   return ret;
}

int pf_cmio_cpm_new_data_ind(
   pf_ar_t                 *p_ar,
   uint16_t                crep,
   bool                    new_data)
{
   int                     ret = 0;

   switch (p_ar->cmio_state)
   {
   case PF_CMIO_STATE_IDLE:
   case PF_CMIO_STATE_STARTUP:
   case PF_CMIO_STATE_DATA:
      /* Ignore */
      break;
   case PF_CMIO_STATE_WDATA:
      if (crep < p_ar->nbr_iocrs)
      {
         if (new_data != p_ar->iocrs[crep].cpm.cmio_start)
         {
            LOG_INFO(PNET_LOG, "CMIO(%d): CPM New Data Ind (crep=%u) start=%s\n", __LINE__, crep, new_data?"true":"false");
         }

         p_ar->iocrs[crep].cpm.cmio_start = new_data;
      }
      else
      {
         ret = -1;
      }
      break;
   }

   return ret;
}
