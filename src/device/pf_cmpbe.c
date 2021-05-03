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

/**
 * @file
 * @brief Implements the Parameter Begin End Protocol Machine (PBE)
 *
 * Disables and enables alarm sending.
 */

#ifdef UNIT_TEST

#endif

#include "pf_includes.h"

/*************** Diagnostic strings *****************************************/

/**
 * @internal
 * Return a string representation of the CMPBE state.
 * @param state            In:   The state.
 * @return  A string representation of the CMPBE state.
 */
static const char * pf_cmpbe_state_to_string (pf_cmpbe_state_values_t state)
{
   const char * s = "<unknown>";
   switch (state)
   {
   case PF_CMPBE_STATE_IDLE:
      s = "PF_CMPBE_STATE_IDLE";
      break;
   case PF_CMPBE_STATE_WFIND:
      s = "PF_CMPBE_STATE_WFIND";
      break;
   case PF_CMPBE_STATE_WFRSP:
      s = "PF_CMPBE_STATE_WFRSP";
      break;
   case PF_CMPBE_STATE_WFPEI:
      s = "PF_CMPBE_STATE_WFPEI";
      break;
   case PF_CMPBE_STATE_WFPER:
      s = "PF_CMPBE_STATE_WFPER";
      break;
   case PF_CMPBE_STATE_WFREQ:
      s = "PF_CMPBE_STATE_WFREQ";
      break;
   case PF_CMPBE_STATE_WFCNF:
      s = "PF_CMPBE_STATE_WFCNF";
      break;
   }

   return s;
}

void pf_cmpbe_show (const pf_ar_t * p_ar)
{
   const char * s = pf_cmpbe_state_to_string (p_ar->cmpbe_state);

   printf ("CMPBE state           = %s\n", s);
   printf ("      stored command  = %#x\n", p_ar->cmpbe_stored_command);
}

/****************************************************************************/

/**
 * @internal
 * Handle state changes in the CMPBE component.
 * @param p_ar             InOut: The AR instance.
 * @param state            In:    The new state.
 */
static void pf_cmpbe_set_state (pf_ar_t * p_ar, pf_cmpbe_state_values_t state)
{
   if (state != p_ar->cmpbe_state)
   {
      LOG_DEBUG (
         PNET_LOG,
         "CMPBE(%d): New state %s for AREP %u (was %s)\n",
         __LINE__,
         pf_cmpbe_state_to_string (state),
         p_ar->arep,
         pf_cmpbe_state_to_string (p_ar->cmpbe_state));
   }
   p_ar->cmpbe_state = state;
}

int pf_cmpbe_cmdev_state_ind (pf_ar_t * p_ar, pnet_event_values_t event)
{
   LOG_DEBUG (
      PNET_LOG,
      "CMPBE(%d): Received event %s from CMDEV. Initial state %s for AREP "
      "%u.\n",
      __LINE__,
      pf_cmdev_event_to_string (event),
      pf_cmpbe_state_to_string (p_ar->cmpbe_state),
      p_ar->arep);
   switch (p_ar->cmpbe_state)
   {
   case PF_CMPBE_STATE_IDLE:
      if (event == PNET_EVENT_DATA)
      {
         pf_cmpbe_set_state (p_ar, PF_CMPBE_STATE_WFIND);
      }
      else
      {
         /* Ignore and stay in state */
      }
      break;
   case PF_CMPBE_STATE_WFRSP:
   case PF_CMPBE_STATE_WFIND:
   case PF_CMPBE_STATE_WFPEI:
   case PF_CMPBE_STATE_WFPER:
   case PF_CMPBE_STATE_WFREQ:
   case PF_CMPBE_STATE_WFCNF:
      if (event == PNET_EVENT_ABORT)
      {
         pf_cmpbe_set_state (p_ar, PF_CMPBE_STATE_IDLE);
      }
      break;
   }

   return 0;
}

int pf_cmpbe_rm_dcontrol_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   pf_control_block_t * p_control_io,
   pnet_result_t * p_result)
{
   int ret = -1;
   uint16_t temp_control_command;

   LOG_DEBUG (
      PNET_LOG,
      "CMPBE(%d): Received DControl command bitfield 0x%04x for AREP %u. "
      "Initial state %s\n",
      __LINE__,
      p_control_io->control_command,
      p_ar->arep,
      pf_cmpbe_state_to_string (p_ar->cmpbe_state));
   switch (p_ar->cmpbe_state)
   {
   case PF_CMPBE_STATE_IDLE:
      ret = 0; /* Ignore */
      break;
   case PF_CMPBE_STATE_WFRSP:
      /* Is virtual state - see below! */
      break;
   case PF_CMPBE_STATE_WFIND:
      if (p_ar->cmpbe_stored_command != 0)
      {
         /* Save */
         temp_control_command = p_control_io->control_command;

         /* Issue stored control command */
         p_control_io->control_command = p_ar->cmpbe_stored_command;
         p_ar->cmpbe_stored_command = 0;

         /* Only stored command is PNET_CONTROL_COMMAND_PRM_BEGIN */
         if (
            pf_fspm_cm_dcontrol_ind (
               net,
               p_ar,
               PNET_CONTROL_COMMAND_PRM_BEGIN,
               p_result) == 0)
         {
            pf_cmpbe_set_state (p_ar, PF_CMPBE_STATE_WFRSP);
            /* cm_dcontrol_rsp() */
            ret = 0;
            pf_cmpbe_set_state (p_ar, PF_CMPBE_STATE_WFPEI);
         }
         /* Restore */
         p_control_io->control_command = temp_control_command;
      }
      else if (
         p_control_io->control_command ==
         BIT (PF_CONTROL_COMMAND_BIT_PRM_BEGIN))
      {
         pf_alarm_disable (p_ar);
         ret = pf_fspm_cm_dcontrol_ind (
            net,
            p_ar,
            PNET_CONTROL_COMMAND_PRM_BEGIN,
            p_result);
         pf_cmpbe_set_state (p_ar, PF_CMPBE_STATE_WFRSP);
      }
      break;
   case PF_CMPBE_STATE_WFPEI:
      if (p_control_io->control_command == BIT (PF_CONTROL_COMMAND_BIT_PRM_END))
      {
         pf_fspm_cm_dcontrol_ind (
            net,
            p_ar,
            PNET_CONTROL_COMMAND_PRM_END,
            p_result);
         pf_cmpbe_set_state (p_ar, PF_CMPBE_STATE_WFPER);
         /* cm_dcontrol_rsp() */
         ret = 0;
         pf_cmpbe_set_state (p_ar, PF_CMPBE_STATE_WFREQ);
      }
      break;
   case PF_CMPBE_STATE_WFPER:
      if (p_control_io->control_command == BIT (PF_CONTROL_COMMAND_BIT_PRM_END))
      {
         pf_cmpbe_set_state (p_ar, PF_CMPBE_STATE_WFREQ);
         ret = 0;
      }
      break;
   case PF_CMPBE_STATE_WFREQ:
      if (p_control_io->control_command == BIT (PF_CONTROL_COMMAND_BIT_PRM_BEGIN))
      {
         /* Note: PrmEnd is handled by CMRPC as re-run. */
         (void)pf_cmdev_state_ind (net, p_ar, PNET_EVENT_ABORT);
         pf_cmpbe_set_state (p_ar, PF_CMPBE_STATE_IDLE);
      }
      break;
   case PF_CMPBE_STATE_WFCNF:
      if (p_control_io->control_command == BIT (PF_CONTROL_COMMAND_BIT_PRM_BEGIN))
      {
         p_ar->cmpbe_stored_command = p_control_io->control_command;
      }
      break;
   }

   return ret;
}

int pf_cmpbe_rm_ccontrol_cnf (
   pf_ar_t * p_ar,
   const pf_control_block_t * p_control_io,
   pnet_result_t * p_result)
{
   int ret = -1;

   if (p_ar->cmpbe_state == PF_CMPBE_STATE_WFCNF)
   {
      /*
       * The only expected bit is the DONE bit.
       * Assume it is the confirmation to the APPL_RDY ccontrol_req.
       */
      if (p_control_io->control_command == BIT (PF_CONTROL_COMMAND_BIT_DONE))
      {
         pf_alarm_enable (p_ar);
         pf_cmpbe_set_state (p_ar, PF_CMPBE_STATE_WFIND);
         ret = 0;
      }
   }
   else
   {
      ret = 0; /* Ignore */
   }

   return ret;
}

/* Not used */
int pf_cmpbe_cm_ccontrol_req (pnet_t * net, pf_ar_t * p_ar)
{
   int ret = -1;

   LOG_DEBUG (
      PNET_LOG,
      "CMPBE(%d): ccontrol_req in state: %s\n",
      __LINE__,
      pf_cmpbe_state_to_string (p_ar->cmpbe_state));

   if (p_ar->cmpbe_state == PF_CMPBE_STATE_WFREQ)
   {
      /* Control block is always APPLRDY here */
      if (pf_alarm_pending (p_ar) == false)
      {
         ret = pf_cmrpc_rm_ccontrol_req (net, p_ar);
         pf_cmpbe_set_state (p_ar, PF_CMPBE_STATE_WFCNF);
      }
   }

   return ret;
}
