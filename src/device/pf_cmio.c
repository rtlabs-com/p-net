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

/**
 * @file
 * @brief Implements the Context Management Input Output protocol machine (CMIO)
 *
 * This monitors several CPM during startup, to know when there is incoming
 * cyclic data.
 *
 * States are IDLE, STARTUP, WDATA and DATA.
 *
 * While waiting for data, it polls the CPMs every 100 ms to check if data has
 * arrived, and tells CMDEV the result.
 *
 */

#ifdef UNIT_TEST

#endif

#include "pf_includes.h"

#define PF_CMIO_TIMER_PERIOD (100 * 1000) /* us */

static const char * cmio_sched_name = "cmio";

/*************** Diagnostic strings *****************************************/

/**
 * @internal
 * Return a string representation of the CMIO state.
 * @param state            In:   The state.
 * @return  A string representation of the CMIO state.
 */
static const char * pf_cmio_state_to_string (pf_cmio_state_values_t state)
{
   const char * s = "<unknown>";

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

void pf_cmio_show (const pf_ar_t * p_ar)
{
   printf (
      "CMIO state            = %s\n",
      pf_cmio_state_to_string (p_ar->cmio_state));
   printf ("     timer            = %u\n", (unsigned)p_ar->cmio_timer);
   printf ("     timer run        = %u\n", (unsigned)p_ar->cmio_timer_run);
}

/****************************************************************************/

/**
 * @internal
 * @param p_ar             InOut: The AR instance.
 * @param state            In:    The new CMIO state.
 */
static void pf_cmio_set_state (pf_ar_t * p_ar, pf_cmio_state_values_t state)
{
   if (state != p_ar->cmio_state)
   {
      LOG_DEBUG (
         PNET_LOG,
         "CMIO(%d): New state %s\n",
         __LINE__,
         pf_cmio_state_to_string (state));
      p_ar->cmio_state = state;
   }
}

/**
 * @internal
 * Poll each output IOCR (CPM) if they have received data from the controller.
 *
 * This is a callback for the scheduler. Arguments should fulfill
 * pf_scheduler_timeout_ftn_t
 *
 * When in state PF_CMIO_STATE_WDATA this function polls (every 100ms) all CPM
 * to see if data has arrived from the controller.
 * If CPM data has arrived in state PF_CMIO_STATE_WDATA then notify CMDEV, else
 * schedule another call in 100ms.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              In:    The AR instance.
 * @param current_time     In:    The current system time, in microseconds,
 *                                when the scheduler is started to execute
 * stored tasks.
 */
static void pf_cmio_timer_expired (
   pnet_t * net,
   void * arg,
   uint32_t current_time)
{
   pf_ar_t * p_ar = (pf_ar_t *)arg;
   uint16_t crep;
   bool data_possible = true;

   if ((p_ar->cmio_state == PF_CMIO_STATE_WDATA) && (p_ar->cmio_timer_run == true))
   {
      for (crep = 0; crep < p_ar->nbr_iocrs; crep++)
      {
         if (
            (p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_OUTPUT) ||
            (p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_MC_CONSUMER))
         {
            if (p_ar->iocrs[crep].cpm.cmio_start == false)
            {
               data_possible = false;
            }
         }
      }
      pf_cmdev_cmio_info_ind (net, p_ar, data_possible);
      pf_scheduler_add (
         net,
         PF_CMIO_TIMER_PERIOD,
         cmio_sched_name,
         pf_cmio_timer_expired,
         p_ar,
         &p_ar->cmio_timer);
   }
   else
   {
      p_ar->cmio_timer_run = false;
      p_ar->cmio_timer = UINT32_MAX;
   }
}

int pf_cmio_cmdev_state_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   pnet_event_values_t event)
{
   uint16_t crep;

   LOG_DEBUG (
      PNET_LOG,
      "CMIO(%d): Received event %s from CMDEV. Our state %s.\n",
      __LINE__,
      pf_cmdev_event_to_string (event),
      pf_cmio_state_to_string (p_ar->cmio_state));

   switch (p_ar->cmio_state)
   {
   case PF_CMIO_STATE_IDLE:
      if (event == PNET_EVENT_ABORT)
      {
         /* Ignore */
         p_ar->cmio_timer_run = false;
         p_ar->cmio_timer = UINT32_MAX;

         pf_cmio_set_state (p_ar, PF_CMIO_STATE_STARTUP);
      }
      else if (event == PNET_EVENT_STARTUP)
      {
         /*
          * CMDEV has received a connect request.
          * Start supervision of the data flow.
          */
         for (crep = 0; crep < p_ar->nbr_iocrs; crep++)
         {
            if (
               (p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_OUTPUT) ||
               (p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_MC_CONSUMER))
            {
               p_ar->iocrs[crep].cpm.cmio_start = false;
            }
         }
         p_ar->cmio_timer_run = false;
         p_ar->cmio_timer = UINT32_MAX;

         pf_cmio_set_state (p_ar, PF_CMIO_STATE_STARTUP);
      }
      break;
   case PF_CMIO_STATE_STARTUP:
      if (event == PNET_EVENT_ABORT)
      {
         p_ar->cmio_timer_run = false;
         p_ar->cmio_timer = UINT32_MAX;

         pf_cmio_set_state (p_ar, PF_CMIO_STATE_IDLE);
      }
      else if (event == PNET_EVENT_STARTUP)
      {
         for (crep = 0; crep < p_ar->nbr_iocrs; crep++)
         {
            if (
               (p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_OUTPUT) ||
               (p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_MC_CONSUMER))
            {
               p_ar->iocrs[crep].cpm.cmio_start = false;
            }
         }
         pf_cmio_set_state (p_ar, PF_CMIO_STATE_WDATA);
      }
      else if (event == PNET_EVENT_PRMEND)
      {
         p_ar->cmio_timer_run = true;
         pf_scheduler_add (
            net,
            PF_CMIO_TIMER_PERIOD,
            cmio_sched_name,
            pf_cmio_timer_expired,
            p_ar,
            &p_ar->cmio_timer);

         pf_cmio_set_state (p_ar, PF_CMIO_STATE_WDATA);
      }
      break;
   case PF_CMIO_STATE_WDATA:
      if (event == PNET_EVENT_ABORT)
      {
         p_ar->cmio_timer_run = false;
         p_ar->cmio_timer = UINT32_MAX;

         pf_cmio_set_state (p_ar, PF_CMIO_STATE_IDLE);
      }
      else if (event == PNET_EVENT_DATA)
      {
         p_ar->cmio_timer_run = false;
         p_ar->cmio_timer = UINT32_MAX;

         pf_cmio_set_state (p_ar, PF_CMIO_STATE_DATA);
      }
      break;
   case PF_CMIO_STATE_DATA:
      if (event == PNET_EVENT_ABORT)
      {
         p_ar->cmio_timer_run = false;
         p_ar->cmio_timer = UINT32_MAX;

         pf_cmio_set_state (p_ar, PF_CMIO_STATE_IDLE);
      }
      break;
   }

   return 0;
}

int pf_cmio_cpm_state_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint16_t crep,
   bool start)
{
   int ret = 0;
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
            LOG_INFO (
               PNET_LOG,
               "CMIO(%d): CPM state change. CMIO state is WDATA. crep: %u, CPM "
               "start=%s\n",
               __LINE__,
               crep,
               start ? "true" : "false");
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
         LOG_INFO (
            PNET_LOG,
            "CMIO(%d): CPM is stopping. CMIO state is DATA. Aborting. crep: "
            "%u\n",
            __LINE__,
            crep);

         /* if (crep != crep.mcpm) not possible - handled elsewhere */
         (void)pf_cmdev_state_ind (net, p_ar, PNET_EVENT_ABORT);
      }
      break;
   }

   return ret;
}

int pf_cmio_cpm_new_data_ind (pf_ar_t * p_ar, uint16_t crep, bool new_data)
{
   int ret = 0;

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
            LOG_DEBUG (
               PNET_LOG,
               "CMIO(%d): New data ind from CPM. (crep=%u) new_data=%s\n",
               __LINE__,
               crep,
               new_data ? "true" : "false");
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
