/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2023 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

/**
 * @file
 * @brief Implements the Plug State Machine (PLUGSM)
 *
 * This is a partial implementation of PLUGSM, to handle the release of
 * submodules when more than one AR is allowed, i.e. when shared device is
 * supported.
 */

#include "pf_includes.h"

void pf_plugsm_released_alarm_req (
   pnet_t * net,
   pf_ar_t * ar,
   uint32_t api,
   uint16_t slot_number,
   uint16_t subslot_number,
   uint32_t module_ident,
   uint32_t submodule_ident)
{
   pf_ar_plug_t * plug;
   uint16_t i;

   plug = &ar->plug_info;
   if (plug->count >= PF_AR_PLUG_MAX_QUEUE)
   {
      LOG_ERROR (
         PNET_LOG,
         "PLUGSM(%d): Queue is full for AREP %d\n",
         __LINE__,
         ar->arep);
   }
   else
   {
      i = plug->current + plug->count;
      if (i >= PF_AR_PLUG_MAX_QUEUE)
      {
         i -= PF_AR_PLUG_MAX_QUEUE;
      }
      plug->queue[i].api = api;
      plug->queue[i].slot_number = slot_number;
      plug->queue[i].subslot_number = subslot_number;
      plug->queue[i].module_ident = module_ident;
      plug->queue[i].submodule_ident = submodule_ident;
      ++plug->count;

      LOG_DEBUG (
         PNET_LOG,
         "PLUGSM(%d): Queued released alarm.\n"
         "  AREP: %u, API: %u, Slot: 0x%x, Subslot: 0x%x\n",
         __LINE__,
         ar->arep,
         api,
         slot_number,
         subslot_number);
   }
   if (plug->state == PF_PLUGSM_STATE_IDLE)
   {
      plug->state = PF_PLUGSM_STATE_WFPRMIND;

      LOG_DEBUG (
         PNET_LOG,
         "PLUGSM(%d): Sending released alarm to controller.\n"
         "  AREP: %u, API: %u, Slot: 0x%x, Subslot: 0x%x\n",
         __LINE__,
         ar->arep,
         plug->queue[plug->current].api,
         plug->queue[plug->current].slot_number,
         plug->queue[plug->current].subslot_number);

      pf_alarm_send_released (
         net,
         ar,
         plug->queue[plug->current].api,
         plug->queue[plug->current].slot_number,
         plug->queue[plug->current].subslot_number,
         plug->queue[plug->current].module_ident,
         plug->queue[plug->current].submodule_ident);
   }
}

int pf_plugsm_prmend_ind (pnet_t * net, pf_ar_t * ar, pnet_result_t * result)
{
   int ret;
   pf_ar_plug_t * plug;

   ret = 0;
   plug = &ar->plug_info;
   if (plug->state == PF_PLUGSM_STATE_WFPRMIND)
   {
      if (net->fspm_cfg.sm_released_cb != NULL)
      {
         LOG_DEBUG (
            PNET_LOG,
            "PLUGSM(%d): Triggering sm_released callback for AREP %u\n",
            __LINE__,
            ar->arep);

         ret = net->fspm_cfg.sm_released_cb (
            net,
            net->fspm_cfg.cb_arg,
            ar->arep,
            plug->queue[plug->current].api,
            plug->queue[plug->current].slot_number,
            plug->queue[plug->current].subslot_number,
            result);
      }
      else
      {
         LOG_DEBUG (
            PNET_LOG,
            "FSPM(%d): No user callback for sm_released implemented.\n",
            __LINE__);
      }

      if (ret == 0)
      {
         plug->state = PF_PLUGSM_STATE_WFDATAUPDATE;
      }
      else
      {
         plug->state = PF_PLUGSM_STATE_WFAPLRDYCNF;
         pf_plugsm_application_ready_cnf (net, ar);
      }
   }

   return ret;
}

void pf_plugsm_application_ready_req (pnet_t * net, pf_ar_t * ar)
{
   pf_ar_plug_t * plug;

   plug = &ar->plug_info;
   if (plug->state == PF_PLUGSM_STATE_WFDATAUPDATE)
   {
      plug->state = PF_PLUGSM_STATE_WFAPLRDYCNF;

      LOG_DEBUG (
         PNET_LOG,
         "PLUGSM(%d): Sending application ready to controller.\n"
         "  AREP: %u, API: %u, Slot: 0x%x, Subslot: 0x%x\n",
         __LINE__,
         ar->arep,
         plug->queue[plug->current].api,
         plug->queue[plug->current].slot_number,
         plug->queue[plug->current].subslot_number);

      pf_cmdev_generate_submodule_diff (
         net,
         ar,
         plug->queue[plug->current].api,
         plug->queue[plug->current].slot_number,
         plug->queue[plug->current].subslot_number);

      pf_cmrpc_rm_ccontrol_req (net, ar, PF_BT_APPRDY_PLUG_ALARM_REQ);
   }
}

int pf_plugsm_application_ready_cnf (pnet_t * net, pf_ar_t * ar)
{
   pf_ar_plug_t * plug;

   plug = &ar->plug_info;
   if (plug->state == PF_PLUGSM_STATE_WFAPLRDYCNF)
   {
      LOG_DEBUG (
         PNET_LOG,
         "PLUGSM(%d): Application ready confirmed by controller.\n"
         "  AREP: %u, API: %u, Slot: 0x%x, Subslot: 0x%x\n",
         __LINE__,
         ar->arep,
         plug->queue[plug->current].api,
         plug->queue[plug->current].slot_number,
         plug->queue[plug->current].subslot_number);

      --plug->count;
      ++plug->current;
      if (plug->current >= PF_AR_PLUG_MAX_QUEUE)
      {
         plug->current = 0;
      }
      if (plug->count > 0)
      {
         plug->state = PF_PLUGSM_STATE_WFPRMIND;

         LOG_DEBUG (
            PNET_LOG,
            "PLUGSM(%d): Sending released alarm to controller.\n"
            "  AREP: %u, API: %u, Slot: 0x%x, Subslot: 0x%x\n",
            __LINE__,
            ar->arep,
            plug->queue[plug->current].api,
            plug->queue[plug->current].slot_number,
            plug->queue[plug->current].subslot_number);

         pf_alarm_send_released (
            net,
            ar,
            plug->queue[plug->current].api,
            plug->queue[plug->current].slot_number,
            plug->queue[plug->current].subslot_number,
            plug->queue[plug->current].module_ident,
            plug->queue[plug->current].submodule_ident);
      }
      else
      {
         plug->state = PF_PLUGSM_STATE_IDLE;
      }
   }
   return 0;
}
