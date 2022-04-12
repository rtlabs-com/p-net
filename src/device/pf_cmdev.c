
/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 * www.rt-labs.com
 * Copyright 2018 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 * See LICENSE file in the project root for full license information.
 ********************************************************************/

/**
 * @file
 * @brief Implements the Context Management Protocol Machine Device (CMDEV)
 *
 * This handles connection establishment for IO-Devices.
 *
 * For example pulling and plugging modules and submodules in slots and
 * subslots are done in this file. Also implements handling connect, release,
 * ccontrol and dcontrol.
 *
 * Collect IOCS and IOPS information.
 *
 * Bookkeeping of diagnosis entries.
 *
 */

#ifdef UNIT_TEST

#endif

#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <inttypes.h>

#include "pf_includes.h"
#include "pf_block_reader.h"
#include "pf_block_writer.h"

/* Forward declaration */

static int pf_cmdev_get_exp_sub (
   pf_ar_t * p_ar,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   pf_exp_submodule_t ** pp_exp_sub);

/* ====================================================================== */

int pf_cmdev_cfg_traverse (
   pnet_t * net,
   pf_ftn_device_t p_ftn_dev,
   pf_ftn_api_t p_ftn_api,
   pf_ftn_slot_t p_ftn_slot,
   pf_ftn_subslot_t p_ftn_sub)
{
   int ret = -1;
   uint16_t api_ix;
   uint16_t slot_ix;
   uint16_t subslot_ix;

   if (p_ftn_dev != NULL)
   {
      if (p_ftn_dev (&net->cmdev_device) != 0)
      {
         ret = -1;
      }
      else if (p_ftn_api != NULL)
      {
         for (api_ix = 0; api_ix < NELEMENTS (net->cmdev_device.apis); api_ix++)
         {
            if (net->cmdev_device.apis[api_ix].in_use == true)
            {
               if (p_ftn_api (&net->cmdev_device.apis[api_ix]) != 0)
               {
                  ret = -1;
               }
               else if (p_ftn_slot != NULL)
               {
                  for (slot_ix = 0;
                       slot_ix <
                       NELEMENTS (net->cmdev_device.apis[api_ix].slots);
                       slot_ix++)
                  {
                     if (net->cmdev_device.apis[api_ix].slots[slot_ix].in_use == true)
                     {
                        if (
                           p_ftn_slot (
                              &net->cmdev_device.apis[api_ix].slots[slot_ix]) !=
                           0)
                        {
                           ret = -1;
                        }
                        else if (p_ftn_sub != NULL)
                        {
                           for (subslot_ix = 0;
                                subslot_ix <
                                NELEMENTS (net->cmdev_device.apis[api_ix]
                                              .slots[slot_ix]
                                              .subslots);
                                subslot_ix++)
                           {
                              if (
                                 net->cmdev_device.apis[api_ix]
                                    .slots[slot_ix]
                                    .subslots[subslot_ix]
                                    .in_use == true)
                              {
                                 if (
                                    p_ftn_sub (&net->cmdev_device.apis[api_ix]
                                                   .slots[slot_ix]
                                                   .subslots[subslot_ix]) != 0)
                                 {
                                    ret = -1;
                                 }
                              }
                           }
                        }
                     }
                  }
               }
            }
         }
      }
   }

   return ret;
}

/**************************** Getters *************************************/

int pf_cmdev_get_device (pnet_t * net, pf_device_t ** pp_device)
{
   *pp_device = &net->cmdev_device;

   return 0;
}

int pf_cmdev_get_api (pnet_t * net, uint32_t api_id, pf_api_t ** pp_api)
{
   int ret = -1;
   pf_api_t * p_api = NULL;
   uint16_t ix = 0;

   if (pp_api == NULL)
   {
      LOG_ERROR (PNET_LOG, "CMDEV(%d): NULL pointer\n", __LINE__);
   }
   else
   {
      while ((ix < PNET_MAX_API) &&
             ((net->cmdev_device.apis[ix].in_use == false) ||
              (api_id != net->cmdev_device.apis[ix].api_id)))
      {
         ix++;
      }

      if ((ix < PNET_MAX_API) && (net->cmdev_device.apis[ix].in_use == true))
      {
         p_api = &net->cmdev_device.apis[ix];
         ret = 0;
      }

      *pp_api = p_api;
   }

   return ret;
}

/**
 * @internal
 * Get an slot instance of an API.
 * @param p_api            InOut: The API instance.
 * @param slot_nbr         In:    The slot number (must be in_use)
 * @param pp_slot          Out:   The slot instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmdev_get_slot (
   pf_api_t * p_api,
   uint16_t slot_nbr,
   pf_slot_t ** pp_slot)
{
   int ret = -1;
   pf_slot_t * p_slot = NULL;
   uint16_t ix;

   if ((p_api == NULL) || (pp_slot == NULL))
   {
      LOG_ERROR (PNET_LOG, "CMDEV(%d): NULL pointer(s)\n", __LINE__);
   }
   else
   {
      ix = 0;
      while ((ix < PNET_MAX_SLOTS) && ((p_api->slots[ix].in_use == false) ||
                                       (slot_nbr != p_api->slots[ix].slot_nbr)))
      {
         ix++;
      }

      if (ix < PNET_MAX_SLOTS)
      {
         p_slot = &p_api->slots[ix];
         ret = 0;
      }

      *pp_slot = p_slot;
   }

   return ret;
}

/**
 * @internal
 * Get an sub-slot instance of a slot instance.
 * @param p_slot           InOut: The slot instance.
 * @param subslot_nbr      In:    The sub-slot number (must be in_use).
 * @param pp_subslot       Out:   The sub-slot instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmdev_get_subslot (
   pf_slot_t * p_slot,
   uint16_t subslot_nbr,
   pf_subslot_t ** pp_subslot)
{
   int ret = -1;
   pf_subslot_t * p_subslot = NULL;
   uint16_t ix;

   if ((p_slot == NULL) || (pp_subslot == NULL))
   {
      LOG_ERROR (PNET_LOG, "CMDEV(%d): NULL pointer(s)\n", __LINE__);
   }
   else
   {
      ix = 0;
      while ((ix < PNET_MAX_SUBSLOTS) &&
             ((p_slot->subslots[ix].in_use == false) ||
              (subslot_nbr != p_slot->subslots[ix].subslot_nbr)))
      {
         ix++;
      }

      if (ix < PNET_MAX_SUBSLOTS)
      {
         p_subslot = &p_slot->subslots[ix];
         ret = 0;
      }

      *pp_subslot = p_subslot;
   }

   return ret;
}

int pf_cmdev_get_subslot_full (
   pnet_t * net,
   uint16_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   pf_subslot_t ** pp_subslot)
{
   int ret = -1;
   pf_api_t * p_api = NULL;
   pf_slot_t * p_slot = NULL;

   if (pf_cmdev_get_api (net, api_id, &p_api) == 0)
   {
      if (pf_cmdev_get_slot (p_api, slot_nbr, &p_slot) == 0)
      {
         if (pf_cmdev_get_subslot (p_slot, subslot_nbr, pp_subslot) == 0)
         {
            ret = 0;
         }
      }
   }

   return ret;
}

int pf_cmdev_get_slot_full (
   pnet_t * net,
   uint16_t api_id,
   uint16_t slot_nbr,
   pf_slot_t ** pp_slot)
{
   int ret = -1;
   pf_api_t * p_api = NULL;

   if (pf_cmdev_get_api (net, api_id, &p_api) == 0)
   {
      if (pf_cmdev_get_slot (p_api, slot_nbr, pp_slot) == 0)
      {
         ret = 0;
      }
   }

   return ret;
}

int pf_cmdev_get_module_ident (
   pnet_t * net,
   uint16_t api_id,
   uint16_t slot_nbr,
   uint32_t * p_module_ident)
{
   pf_slot_t * p_slot = NULL;
   if (pf_cmdev_get_slot_full (net, api_id, slot_nbr, &p_slot) == 0)
   {
      *p_module_ident = p_slot->module_ident_number;
      return 0;
   }
   return -1;
}

int pf_cmdev_get_submodule_ident (
   pnet_t * net,
   uint16_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t * p_submodule_ident)
{
   pf_subslot_t * p_subslot = NULL;
   if (
      pf_cmdev_get_subslot_full (net, api_id, slot_nbr, subslot_nbr, &p_subslot) ==
      0)
   {
      *p_submodule_ident = p_subslot->submodule_ident_number;
      return 0;
   }
   return -1;
}

/******************** Diagnosis **********************************************/

int pf_cmdev_get_diag_item (
   pnet_t * net,
   uint16_t item_ix,
   pf_diag_item_t ** pp_item)
{
   int ret = -1;

   if (item_ix < NELEMENTS (net->cmdev_device.diag_items))
   {
      *pp_item = &net->cmdev_device.diag_items[item_ix];

      ret = 0;
   }
   else
   {
      *pp_item = NULL;
   }

   return ret;
}

int pf_cmdev_new_diag (pnet_t * net, uint16_t * p_item_ix)
{
   int ret = -1;

   /* Return an item from the free list (after removing it). */
   *p_item_ix = net->cmdev_device.diag_items_free;
   if (*p_item_ix != PF_DIAG_IX_NULL)
   {
      net->cmdev_device.diag_items_free =
         net->cmdev_device.diag_items[*p_item_ix].next;

      /* Clear the entry */
      memset (
         &net->cmdev_device.diag_items[*p_item_ix],
         0,
         sizeof (net->cmdev_device.diag_items[*p_item_ix]));
      net->cmdev_device.diag_items[*p_item_ix].in_use = true;

      ret = 0;
   }

   return ret;
}

void pf_cmdev_free_diag (pnet_t * net, uint16_t item_ix)
{
   if (item_ix < NELEMENTS (net->cmdev_device.diag_items))
   {
      /* Put it first in the free list. */
      net->cmdev_device.diag_items[item_ix].in_use = false;
      net->cmdev_device.diag_items[item_ix].next =
         net->cmdev_device.diag_items_free;
      net->cmdev_device.diag_items_free = item_ix;
   }
}

int pf_cmdev_get_next_diagnosis_usi (
   pnet_t * net,
   uint16_t list_head,
   uint16_t low_usi_limit,
   uint16_t * p_next_usi)
{
   int ret = -1;
   pf_diag_item_t * p_item = NULL;
   uint16_t resulting_value = UINT16_MAX;

   /* Walk the list to find next larger USI value */
   pf_cmdev_get_diag_item (net, list_head, &p_item);
   while (p_item != NULL)
   {
      if (p_item->usi > low_usi_limit)
      {
         if (p_item->usi < resulting_value)
         {
            resulting_value = p_item->usi;
         }
      }

      pf_cmdev_get_diag_item (net, p_item->next, &p_item);
   }

   if (resulting_value != UINT16_MAX)
   {
      *p_next_usi = resulting_value;
      ret = 0;
   }

   return ret;
}

/*****************************************************************************/

/**
 * @internal
 * Instantiate a new API structure.
 * If the API identifier already exists the the operation fails.
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:    The API identifier.
 * @param pp_api           Out:   The new API instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmdev_new_api (pnet_t * net, uint32_t api_id, pf_api_t ** pp_api)
{
   int ret = -1;
   pf_api_t * p_api = NULL;
   uint16_t ix;

   if (pp_api == NULL)
   {
      LOG_ERROR (PNET_LOG, "CMDEV(%d): NULL pointer(s)\n", __LINE__);
   }
   else if (pf_cmdev_get_api (net, api_id, &p_api) == 0)
   {
      LOG_ERROR (
         PNET_LOG,
         "CMDEV(%d): API %u already exists\n",
         __LINE__,
         (unsigned)api_id);
   }
   else
   {
      ix = 0;
      while ((ix < PNET_MAX_API) && (net->cmdev_device.apis[ix].in_use == true))
      {
         ix++;
      }

      if (ix < PNET_MAX_API)
      {
         p_api = &net->cmdev_device.apis[ix];

         memset (p_api, 0, sizeof (*p_api));
         p_api->api_id = api_id;
         p_api->in_use = true;

         ret = 0;
      }

      *pp_api = p_api;
   }

   return ret;
}

/******************* Slots, subslots, modules and submodules *****************/

/**
 * @internal
 * Instantiate a new slot structure.
 * If the slot number already exists the the operation fails.
 * @param p_api            InOut: The API instance.
 * @param slot_nbr         In:    The slot number.
 * @param pp_slot          Out:   The new slot instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmdev_new_slot (
   pf_api_t * p_api,
   uint16_t slot_nbr,
   pf_slot_t ** pp_slot)
{
   int ret = -1;
   pf_slot_t * p_slot = NULL;
   uint16_t ix;

   if ((p_api == NULL) || (pp_slot == NULL))
   {
      LOG_ERROR (PNET_LOG, "CMDEV(%d): NULL pointer(s)\n", __LINE__);
   }
   else if (pf_cmdev_get_slot (p_api, slot_nbr, &p_slot) == 0)
   {
      /* Slot already exists */
      LOG_ERROR (
         PNET_LOG,
         "CMDEV(%d): Slot %u already exists\n",
         __LINE__,
         (unsigned)slot_nbr);
   }
   else
   {
      ix = 0;
      while ((ix < PNET_MAX_SLOTS) && (p_api->slots[ix].in_use == true))
      {
         ix++;
      }

      if (ix < PNET_MAX_SLOTS)
      {
         p_slot = &p_api->slots[ix];

         memset (p_slot, 0, sizeof (*p_slot));
         p_slot->slot_nbr = slot_nbr;
         p_slot->in_use = true;

         ret = 0;
      }

      *pp_slot = p_slot;
   }

   return ret;
}

/**
 * @internal
 * Instantiate a new sub-slot structure.
 * If the sub-slot number already exists the the operation fails.
 * @param p_slot           InOut: The slot instance.
 * @param subslot_nbr      In:    The sub-slot number.
 * @param pp_subslot       Out:   The new sub-slot instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmdev_new_subslot (
   pf_slot_t * p_slot,
   uint16_t subslot_nbr,
   pf_subslot_t ** pp_subslot)
{
   int ret = -1;
   pf_subslot_t * p_subslot = NULL;
   uint16_t ix;

   if ((p_slot == NULL) || (pp_subslot == NULL))
   {
      LOG_ERROR (PNET_LOG, "CMDEV(%d): NULL pointer(s)\n", __LINE__);
   }
   else if (pf_cmdev_get_subslot (p_slot, subslot_nbr, &p_subslot) == 0)
   {
      /* Subslot already exists */
      LOG_ERROR (
         PNET_LOG,
         "CMDEV(%d): Subslot %u already exists\n",
         __LINE__,
         (unsigned)subslot_nbr);
   }
   else
   {
      ix = 0;
      while ((ix < PNET_MAX_SUBSLOTS) && (p_slot->subslots[ix].in_use == true))
      {
         ix++;
      }

      if (ix < PNET_MAX_SUBSLOTS)
      {
         p_subslot = &p_slot->subslots[ix];

         memset (p_subslot, 0, sizeof (*p_subslot));
         p_subslot->subslot_nbr = subslot_nbr;
         p_subslot->diag_list = PF_DIAG_IX_NULL;
         p_subslot->in_use = true;

         ret = 0;
      }

      *pp_subslot = p_subslot;
   }

   return ret;
}

int pf_cmdev_plug_module (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint32_t module_ident_nbr)
{
   int ret = -1;
   pf_api_t * p_api = NULL;
   pf_slot_t * p_slot = NULL;

   if (pf_cmdev_get_api (net, api_id, &p_api) != 0)
   {
      LOG_ERROR (
         PNET_LOG,
         "CMDEV(%d): API %u does not exist\n",
         __LINE__,
         (unsigned)api_id);
   }
   else if (pf_cmdev_get_slot (p_api, slot_nbr, &p_slot) == 0)
   {
      /* Slot already has a plugged module. Check ident numbers */
      if (p_slot->module_ident_number == module_ident_nbr)
      {
         /* Correct module already plugged */
         p_slot->plug_state = PF_MOD_PLUG_PROPER_MODULE;

         ret = 0;
      }
      else
      {
         LOG_DEBUG (
            PNET_LOG,
            "CMDEV(%d): Wrong plugged module ident %u in api %u slot %u\n",
            __LINE__,
            (unsigned)module_ident_nbr,
            (unsigned)api_id,
            (unsigned)slot_nbr);
         p_slot->plug_state = PF_MOD_PLUG_WRONG_MODULE;
      }
   }
   else if (pf_cmdev_new_slot (p_api, slot_nbr, &p_slot) != 0)
   {
      /* Out of slot resources */
      LOG_ERROR (
         PNET_LOG,
         "CMDEV(%d): Out of slot resources for api %u slot %u\n",
         __LINE__,
         (unsigned)api_id,
         (unsigned)slot_nbr);
   }
   else
   {
      /* Slot allocated */
      p_slot->module_ident_number = module_ident_nbr;
      p_slot->plug_state = PF_MOD_PLUG_PROPER_MODULE;

      /* Inherit AR from API */
      p_slot->p_ar = p_api->p_ar;

      ret = 0;
   }

   return ret;
}

int pf_cmdev_pull_submodule (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr)
{
   int ret = -1;
   pf_api_t * p_api = NULL;
   pf_slot_t * p_slot = NULL;
   pf_subslot_t * p_subslot = NULL;

   if (pf_cmdev_get_api (net, api_id, &p_api) != 0)
   {
      LOG_ERROR (
         PNET_LOG,
         "CMDEV(%d): API %u does not exist\n",
         __LINE__,
         (unsigned)api_id);
   }
   else if (pf_cmdev_get_slot (p_api, slot_nbr, &p_slot) != 0)
   {
      LOG_DEBUG (
         PNET_LOG,
         "CMDEV(%d): No module in slot %u\n",
         __LINE__,
         (unsigned)slot_nbr);
   }
   else if (pf_cmdev_get_subslot (p_slot, subslot_nbr, &p_subslot) != 0)
   {
      LOG_DEBUG (
         PNET_LOG,
         "CMDEV(%d): No submodule in api_id %u slot %u subslot %u\n",
         __LINE__,
         (unsigned)api_id,
         (unsigned)slot_nbr,
         (unsigned)subslot_nbr);
   }
   else
   {
      p_subslot->in_use = false;
      p_subslot->submodule_state.ident_info = PF_SUBMOD_PLUG_NO;

      ret = pf_alarm_send_pull (
         net,
         p_subslot->p_ar,
         api_id,
         slot_nbr,
         subslot_nbr);
   }

   return ret;
}

int pf_cmdev_plug_submodule (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t module_ident_nbr,
   uint32_t submod_ident_nbr,
   pnet_submodule_dir_t direction,
   uint16_t length_input,
   uint16_t length_output,
   bool update)
{
   int ret = -1;
   pf_api_t * p_api = NULL;
   pf_slot_t * p_slot = NULL;
   pf_subslot_t * p_subslot = NULL;
   pf_exp_submodule_t * p_exp_sub = NULL;
   bool already_plugged = false;

   if (pf_cmdev_get_api (net, api_id, &p_api) != 0)
   {
      LOG_ERROR (
         PNET_LOG,
         "CMDEV(%d): API %u does not exist\n",
         __LINE__,
         (unsigned)api_id);
   }
   else if (pf_cmdev_get_slot (p_api, slot_nbr, &p_slot) != 0)
   {
      /* Auto-plug the module */
      if (pf_cmdev_plug_module (net, api_id, slot_nbr, module_ident_nbr) != 0)
      {
         LOG_ERROR (
            PNET_LOG,
            "CMDEV(%d): Out of slot resources for api %u\n",
            __LINE__,
            (unsigned)api_id);
      }
   }
   else if (pf_cmdev_get_subslot (p_slot, subslot_nbr, &p_subslot) == 0)
   {
      if (update == true)
      {
         if (pf_cmdev_pull_submodule (net, api_id, slot_nbr, subslot_nbr) != 0)
         {
            LOG_ERROR (
               PNET_LOG,
               "CMDEV(%d): Could not pull submodule in api %u slot %u subslot "
               "%u\n",
               __LINE__,
               (unsigned)api_id,
               (unsigned)slot_nbr,
               (unsigned)subslot_nbr);
         }
         p_subslot->submodule_state.ident_info = PF_SUBMOD_PLUG_NO;
      }
      else if (p_subslot->submodule_ident_number == submod_ident_nbr)
      {
         already_plugged = true;
         p_subslot->submodule_state.ident_info = PF_SUBMOD_PLUG_OK;
      }
      else
      {
         LOG_DEBUG (
            PNET_LOG,
            "CMDEV(%d): Substitute submodule ident 0x%08x number in api %u "
            "slot %u subslot %u\n",
            __LINE__,
            (unsigned)submod_ident_nbr,
            (unsigned)api_id,
            (unsigned)slot_nbr,
            (unsigned)subslot_nbr);
         p_subslot->submodule_state.ident_info = PF_SUBMOD_PLUG_SUBSTITUTE;
      }
   }

   if (already_plugged == true)
   {
      LOG_INFO (
         PNET_LOG,
         "CMDEV(%d): Submodule ident 0x%08x number already plugged in api %u "
         "slot %u subslot %u\n",
         __LINE__,
         (unsigned)submod_ident_nbr,
         (unsigned)api_id,
         (unsigned)slot_nbr,
         (unsigned)subslot_nbr);
      p_subslot->submodule_state.ident_info = PF_SUBMOD_PLUG_OK;

      ret = 0;
   }
   else if (pf_cmdev_get_slot (p_api, slot_nbr, &p_slot) != 0)
   {
      LOG_ERROR (
         PNET_LOG,
         "CMDEV(%d): No module in slot %u\n",
         __LINE__,
         (unsigned)slot_nbr);
   }
   else if (pf_cmdev_new_subslot (p_slot, subslot_nbr, &p_subslot) == 0)
   {
      /* Sub-slot created */
      p_subslot->submodule_ident_number = submod_ident_nbr;

      /*
       * While plugging the DAP sub-modules there is no AR yet, so the
       * sub-modules are always right.
       */
      if (
         (p_slot->p_ar == NULL) || (pf_cmdev_get_exp_sub (
                                       p_slot->p_ar,
                                       api_id,
                                       slot_nbr,
                                       subslot_nbr,
                                       &p_exp_sub) != 0))
      {
         p_subslot->submodule_state.ident_info = PF_SUBMOD_PLUG_OK;

         ret = 0;
      }
      else
      {
         if (p_exp_sub->submodule_ident_number == submod_ident_nbr)
         {
            p_subslot->submodule_state.ident_info = PF_SUBMOD_PLUG_OK;

            ret = pf_alarm_send_plug (
               net,
               p_slot->p_ar,
               api_id,
               slot_nbr,
               subslot_nbr,
               module_ident_nbr,
               submod_ident_nbr);
         }
         else
         {
            p_subslot->submodule_state.ident_info = PF_SUBMOD_PLUG_SUBSTITUTE;

            ret = pf_alarm_send_plug_wrong (
               net,
               p_slot->p_ar,
               api_id,
               slot_nbr,
               subslot_nbr,
               module_ident_nbr,
               submod_ident_nbr);
         }
      }
      p_subslot->submodule_state.format_indicator = 1;

      p_subslot->direction = direction;
      p_subslot->length_input = length_input;
      p_subslot->length_output = length_output;

      /* Inherit AR from slot module */
      p_subslot->p_ar = p_slot->p_ar;

      LOG_DEBUG (
         PNET_LOG,
         "CMDEV(%d): Plugged submodule ident 0x%08x number in api %u "
         "slot %u subslot %u\n",
         __LINE__,
         (unsigned)submod_ident_nbr,
         (unsigned)api_id,
         (unsigned)slot_nbr,
         (unsigned)subslot_nbr);
   }
   else
   {
      /* Out of resources */
      LOG_ERROR (
         PNET_LOG,
         "CMDEV(%d): Out of subslot resources for api %u slot %u\n",
         __LINE__,
         (unsigned)api_id,
         (unsigned)slot_nbr);
   }

   return ret;
}

int pf_cmdev_pull_module (pnet_t * net, uint32_t api_id, uint16_t slot_nbr)
{
   int ret = -1;
   pf_api_t * p_api = NULL;
   pf_slot_t * p_slot = NULL;
   uint16_t ix;

   /* Pull all submodules. Then pull the module */
   if (pf_cmdev_get_api (net, api_id, &p_api) != 0)
   {
      LOG_ERROR (
         PNET_LOG,
         "CMDEV(%d): API %u does not exist\n",
         __LINE__,
         (unsigned)api_id);
   }
   else if (pf_cmdev_get_slot (p_api, slot_nbr, &p_slot) != 0)
   {
      LOG_DEBUG (
         PNET_LOG,
         "CMDEV(%d): No module in slot %u\n",
         __LINE__,
         (unsigned)slot_nbr);
   }
   else
   {
      ret = 0; /* Assume all OK */
      ix = 0;
      while ((ix < PNET_MAX_SUBSLOTS) && (ret == 0))
      {
         if (
            (p_slot->subslots[ix].in_use == true) &&
            (pf_cmdev_pull_submodule (
                net,
                api_id,
                slot_nbr,
                p_slot->subslots[ix].subslot_nbr) != 0))
         {
            ret = -1;
         }
         ix++;
      }

      if (ret == 0)
      {
         p_slot->in_use = false;
         p_slot->plug_state = PF_MOD_PLUG_NO_MODULE;
      }
      else
      {
         LOG_ERROR (
            PNET_LOG,
            "CMDEV(%d): Could not pull all submodules for api %u slot %u\n",
            __LINE__,
            (unsigned)api_id,
            (unsigned)slot_nbr);
      }
   }

   return ret;
}

/**
 * @internal
 * Remove all entries that refer to the AR.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR entries to remove.
 */
static void pf_device_clear (pnet_t * net, pf_ar_t * p_ar)
{
   uint16_t api_ix;
   uint16_t slot_ix;
   uint16_t sub_ix;

   for (api_ix = 0; api_ix < NELEMENTS (net->cmdev_device.apis); api_ix++)
   {
      for (slot_ix = 0;
           slot_ix < NELEMENTS (net->cmdev_device.apis[api_ix].slots);
           slot_ix++)
      {
         for (sub_ix = 0;
              sub_ix <
              NELEMENTS (
                 net->cmdev_device.apis[api_ix].slots[slot_ix].subslots);
              sub_ix++)
         {
            if (
               net->cmdev_device.apis[api_ix]
                  .slots[slot_ix]
                  .subslots[sub_ix]
                  .p_ar == p_ar)
            {
               net->cmdev_device.apis[api_ix]
                  .slots[slot_ix]
                  .subslots[sub_ix]
                  .p_ar = NULL;
            }
         }
         if (net->cmdev_device.apis[api_ix].slots[slot_ix].p_ar == p_ar)
         {
            net->cmdev_device.apis[api_ix].slots[slot_ix].p_ar = NULL;
         }
      }
      if (net->cmdev_device.apis[api_ix].p_ar == p_ar)
      {
         net->cmdev_device.apis[api_ix].p_ar = NULL;
      }
   }
}

/*************** Diagnostic strings ****************************************/

const char * pf_cmdev_submod_dir_to_string (pnet_submodule_dir_t direction)
{
   const char * s = "<error>";

   switch (direction)
   {
   case PNET_DIR_NO_IO:
      s = "NO_IO";
      break;
   case PNET_DIR_INPUT:
      s = "INPUT";
      break;
   case PNET_DIR_OUTPUT:
      s = "OUTPUT";
      break;
   case PNET_DIR_IO:
      s = "INPUT_OUTPUT";
      break;
   }

   return s;
}

const char * pf_cmdev_state_to_string (pf_cmdev_state_values_t state)
{
   const char * s = "unknown";

   switch (state)
   {
   case PF_CMDEV_STATE_POWER_ON:
      s = "PF_CMDEV_STATE_POWER_ON";
      break;
   case PF_CMDEV_STATE_W_CIND:
      s = "PF_CMDEV_STATE_W_CIND";
      break;
   case PF_CMDEV_STATE_W_CRES:
      s = "PF_CMDEV_STATE_W_CRES";
      break;
   case PF_CMDEV_STATE_W_SUCNF:
      s = "PF_CMDEV_STATE_W_SUCNF";
      break;
   case PF_CMDEV_STATE_W_PEIND:
      s = "PF_CMDEV_STATE_W_PEIND";
      break;
   case PF_CMDEV_STATE_W_PERES:
      s = "PF_CMDEV_STATE_W_PERES";
      break;
   case PF_CMDEV_STATE_W_ARDY:
      s = "PF_CMDEV_STATE_W_ARDY";
      break;
   case PF_CMDEV_STATE_W_ARDYCNF:
      s = "PF_CMDEV_STATE_W_ARDYCNF";
      break;
   case PF_CMDEV_STATE_WDATA:
      s = "PF_CMDEV_STATE_WDATA";
      break;
   case PF_CMDEV_STATE_DATA:
      s = "PF_CMDEV_STATE_DATA";
      break;
   case PF_CMDEV_STATE_ABORT:
      s = "PF_CMDEV_STATE_ABORT";
      break;
   default:
      break;
   }
   return s;
}

const char * pf_cmdev_event_to_string (pnet_event_values_t event)
{
   const char * s = "unknown";

   switch (event)
   {
   case PNET_EVENT_ABORT:
      s = "PNET_EVENT_ABORT";
      break;
   case PNET_EVENT_STARTUP:
      s = "PNET_EVENT_STARTUP";
      break;
   case PNET_EVENT_PRMEND:
      s = "PNET_EVENT_PRMEND";
      break;
   case PNET_EVENT_APPLRDY:
      s = "PNET_EVENT_APPLRDY";
      break;
   case PNET_EVENT_DATA:
      s = "PNET_EVENT_DATA";
      break;
   default:
      break;
   }
   return s;
}

/**
 * @internal
 * Return a string representation of the specified sub-module direction.
 * @param direction        In:    The direction
 * @return  a string representation of the sub-module direction.
 */
static const char * pf_cmdev_direction_to_string (pnet_submodule_dir_t direction)
{
   const char * s = "unknown";

   switch (direction)
   {
   case PNET_DIR_NO_IO:
      s = "PNET_DIR_NO_IO";
      break;
   case PNET_DIR_INPUT:
      s = "PNET_DIR_INPUT";
      break;
   case PNET_DIR_OUTPUT:
      s = "PNET_DIR_OUTPUT";
      break;
   case PNET_DIR_IO:
      s = "PNET_DIR_IO";
      break;
   default:
      break;
   }

   return s;
}

/**
 * @internal
 * Return a string representation of the specified module plug state.
 * @param plug_state       In:    The plug state
 * @return  a string representation of the module plug state.
 */
static const char * pf_cmdev_mod_plug_state_to_string (
   pf_mod_plug_state_t plug_state)
{
   const char * s = "unknown";

   switch (plug_state)
   {
   case PF_MOD_PLUG_NO_MODULE:
      s = "PF_MOD_PLUG_NO_MODULE";
      break;
   case PF_MOD_PLUG_WRONG_MODULE:
      s = "PF_MOD_PLUG_WRONG_MODULE";
      break;
   case PF_MOD_PLUG_PROPER_MODULE:
      s = "PF_MOD_PLUG_PROPER_MODULE";
      break;
   case PF_MOD_PLUG_SUBSTITUTE:
      s = "PF_MOD_PLUG_SUBSTITUTE";
      break;
   default:
      break;
   }

   return s;
}

/**
 * @internal
 * Return a string representation of the specified sub-module plug state.
 * @param plug_state       In:    The plug state
 * @return  a string representation of the sub-module plug state.
 */
static const char * pf_cmdev_submod_plug_state_to_string (
   pf_submod_plug_state_t plug_state)
{
   const char * s = "unknown";

   switch (plug_state)
   {
   case PF_SUBMOD_PLUG_OK:
      s = "PF_SUBMOD_PLUG_OK";
      break;
   case PF_SUBMOD_PLUG_SUBSTITUTE:
      s = "PF_SUBMOD_PLUG_SUBSTITUTE";
      break;
   case PF_SUBMOD_PLUG_WRONG:
      s = "PF_SUBMOD_PLUG_WRONG";
      break;
   case PF_SUBMOD_PLUG_NO:
      s = "PF_SUBMOD_PLUG_NO";
      break;
   default:
      break;
   }

   return s;
}

void pf_cmdev_ar_show (const pf_ar_t * p_ar)
{
   printf (
      "CMDEV state           = %s\n",
      pf_cmdev_state_to_string (p_ar->cmdev_state));
}

/**
 * @internal
 * Show everything about the device instance.
 * @param p_dev            In:    The device instance.
 * @return  0  Always
 */
static int pf_cmdev_cfg_dev_show (pf_device_t * p_dev)
{
   printf (">>>DEVICE<<<\n");
   printf (
      "The device can use max %u APIs, and %u diag items.\n",
      (unsigned)PNET_MAX_API,
      (unsigned)PNET_MAX_DIAG_ITEMS);
   printf ("First unused diag item: %u.\n", (unsigned)p_dev->diag_items_free);

   return 0;
}

/**
 * @internal
 * Show everything about the API instance.
 * @param p_api            In:    The API instance.
 * @return  0  Always
 */
static int pf_cmdev_cfg_api_show (pf_api_t * p_api)
{
   printf (">>>API<<<\n");
   printf ("device api_id         = %u\n", (unsigned)p_api->api_id);
   printf (
      "   Each API can use max %u slots (each with max %u subslots).\n",
      (unsigned)PNET_MAX_SLOTS,
      (unsigned)PNET_MAX_SUBSLOTS);

   return 0;
}

/**
 * @internal
 * Show everything about the slot instance.
 * @param p_slot           In:    The slot instance.
 * @return  0  Always
 */
static int pf_cmdev_cfg_slot_show (pf_slot_t * p_slot)
{
   printf ("   >>>SLOT<<<\n");
   printf ("   slot_nbr           = %u\n", (unsigned)p_slot->slot_nbr);
   printf ("   in_use             = %u\n", (unsigned)p_slot->in_use);
   printf (
      "   plug_state         = %s\n",
      pf_cmdev_mod_plug_state_to_string (p_slot->plug_state));
   printf ("   AR                 = %p\n", p_slot->p_ar);
   printf (
      "   module_ident       = %u\n",
      (unsigned)p_slot->module_ident_number);
   printf (
      "   exp_mod_ident      = %u\n",
      (unsigned)p_slot->exp_module_ident_number);

   return 0;
}

/**
 * @internal
 * Show everything about the subslot instance.
 * @param p_subslot        In:    The subslot instance.
 * @return  0  Always
 */
static int pf_cmdev_cfg_subslot_show (pf_subslot_t * p_subslot)
{
   printf ("      >>>SUBSLOT<<<\n");
   printf ("      subslot_nbr     = 0x%04X\n", (unsigned)p_subslot->subslot_nbr);
   printf ("      in_use          = %u\n", (unsigned)p_subslot->in_use);
   printf (
      "      plug_state      = %s\n",
      pf_cmdev_submod_plug_state_to_string (
         p_subslot->submodule_state.ident_info));
   printf ("      AR              = %p\n", p_subslot->p_ar);
   printf (
      "      submod_ident    = 0x%08X\n",
      (unsigned)p_subslot->submodule_ident_number);
   printf (
      "      exp_sub_ident   = 0x%08X\n",
      (unsigned)p_subslot->exp_submodule_ident_number);
   printf (
      "      direction       = %s\n",
      pf_cmdev_direction_to_string (p_subslot->direction));
   printf ("      length_input    = %u\n", (unsigned)p_subslot->length_input);
   printf ("      length_output   = %u\n", (unsigned)p_subslot->length_output);
   printf ("      diag list start = ");
   if (p_subslot->diag_list == PF_DIAG_IX_NULL)
   {
      printf ("PF_DIAG_IX_NULL (no diagnosis items)\n");
   }
   else
   {
      printf ("%u\n", (unsigned)p_subslot->diag_list);
   }

   return 0;
}

void pf_cmdev_device_show (pnet_t * net)
{
   printf ("\nCMDEV:\n");
   (void)pf_cmdev_cfg_traverse (
      net,
      pf_cmdev_cfg_dev_show,
      pf_cmdev_cfg_api_show,
      pf_cmdev_cfg_slot_show,
      pf_cmdev_cfg_subslot_show);
}

void pf_cmdev_diag_show (const pnet_t * net)
{
   uint16_t ix = 0;
   uint16_t total = 0;
   const pf_diag_item_t * p_diag;

   printf ("DIAGNOSIS\n");
   printf (
      "Max: %u items\n",
      (uint16_t)NELEMENTS (net->cmdev_device.diag_items));

   for (ix = 0; ix < NELEMENTS (net->cmdev_device.diag_items); ix++)
   {
      if (net->cmdev_device.diag_items[ix].in_use == true)
      {
         total++;
      }
   }
   printf ("Items in use: %u\n", total);

   for (ix = 0; ix < NELEMENTS (net->cmdev_device.diag_items); ix++)
   {
      p_diag = &net->cmdev_device.diag_items[ix];
      if (p_diag->in_use == true)
      {
         printf ("[%3u] USI: 0x%04X ", ix, p_diag->usi);
         if (p_diag->next == UINT16_MAX)
         {
            printf (" [Last]  ");
         }
         else
         {
            printf ("Next: %3u", p_diag->next);
         }

         if (p_diag->usi >= PF_USI_CHANNEL_DIAGNOSIS)
         {
            printf (
               "  Channel: 0x%04X  Ch.error: 0x%04X  Ext.error 0x%04X  "
               "Add.value 0x%08" PRIX32 " Qualifier 0x%08" PRIX32 "\n",
               p_diag->fmt.std.ch_nbr,
               p_diag->fmt.std.ch_error_type,
               p_diag->fmt.std.ext_ch_error_type,
               p_diag->fmt.std.ext_ch_add_value,
               p_diag->fmt.std.qual_ch_qualifier);
         }
         else
         {
            printf ("\n");
         }
      }
   }
   printf ("\n");
}

/********************** CMDEV init, exit and state ****************************/

void pf_cmdev_exit (pnet_t * net)
{
   if (net->cmdev_initialized == true)
   {
      (void)pf_diag_exit();
      os_mutex_destroy (net->cmdev_device.diag_mutex);
      memset (&net->cmdev_device, 0, sizeof (net->cmdev_device));
      net->cmdev_initialized = false;
   }
}

void pf_cmdev_init (pnet_t * net)
{
   uint16_t ix;
   pf_api_t * p_api = NULL;

   if (net->cmdev_initialized == false)
   {
      net->cmdev_initialized = true;

      memset (&net->cmdev_device, 0, sizeof (net->cmdev_device));
      net->cmdev_device.diag_mutex = os_mutex_create();

      /* Create a list of free diag items. */
      net->cmdev_device.diag_items_free = 0;
      for (ix = 0; ix < NELEMENTS (net->cmdev_device.diag_items) - 1; ix++)
      {
         net->cmdev_device.diag_items[ix].next = ix + 1;
      }
      net->cmdev_device.diag_items[NELEMENTS (net->cmdev_device.diag_items) - 1]
         .next = PF_DIAG_IX_NULL;

      (void)pf_diag_init();

      /* Create the default API */
      pf_cmdev_new_api (net, 0, &p_api);
   }
}

int pf_cmdev_get_state (const pf_ar_t * p_ar, pf_cmdev_state_values_t * p_state)
{
   int ret = -1;

   if ((p_ar != NULL) && (p_state != NULL))
   {
      *p_state = p_ar->cmdev_state;
      ret = 0;
   }

   return ret;
}

/**
 * @internal
 * Request a state transition of the specified AR.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param state            In:    New state. Use PF_CMDEV_STATE_xxx
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmdev_set_state (
   pnet_t * net,
   pf_ar_t * p_ar,
   pf_cmdev_state_values_t state)
{
   if (state != p_ar->cmdev_state)
   {
      LOG_DEBUG (
         PNET_LOG,
         "CMDEV(%d): New state: %s for AREP %u (was %s)\n",
         __LINE__,
         pf_cmdev_state_to_string (state),
         p_ar->arep,
         pf_cmdev_state_to_string (p_ar->cmdev_state));
   }
   p_ar->cmdev_state = state;

   switch (state)
   {
   case PF_CMDEV_STATE_ABORT:
      pf_cmdev_state_ind (net, p_ar, PNET_EVENT_ABORT);
      pf_device_clear (net, p_ar);
      break;
   default:
      /* Nothing (yet) */
      break;
   }

   return 0;
}

/************************ Local primitives *****************************/

int pf_cmdev_state_ind (pnet_t * net, pf_ar_t * p_ar, pnet_event_values_t event)
{
   if (p_ar != NULL)
   {
      /* If we move pf_fspm_state_ind(), which triggers a user callback, last in
         this list then it would be possible for users to invoke
         pnet_application_ready() directly in the callback and not to use
         some delay mechanism.

         However then the users could not use pnet_get_ar_error_codes() at
         ABORT, as the AR would already be gone when the callback is triggered.
       */
      LOG_DEBUG (
         PNET_LOG,
         "CMDEV(%d): Sending event %s for AREP %u. Current state %s\n",
         __LINE__,
         pf_cmdev_event_to_string (event),
         p_ar->arep,
         pf_cmdev_state_to_string (p_ar->cmdev_state));

      pf_fspm_state_ind (net, p_ar, event);
      pf_cmsu_cmdev_state_ind (net, p_ar, event);
      pf_cmio_cmdev_state_ind (net, p_ar, event);
      pf_cmwrr_cmdev_state_ind (net, p_ar, event);
      pf_cmsm_cmdev_state_ind (net, p_ar, event);
      pf_cmpbe_cmdev_state_ind (p_ar, event);
      pf_cmrpc_cmdev_state_ind (net, p_ar, event);
   }
   else
   {
      LOG_ERROR (
         PNET_LOG,
         "CMDEV(%d): pf_cmdev_state_ind: p_ar is NULL\n",
         __LINE__);
   }

   return 0;
}

int pf_cmdev_cmio_info_ind (pnet_t * net, pf_ar_t * p_ar, bool data_possible)
{
   if (data_possible != p_ar->ready_4_data)
   {
      LOG_DEBUG (
         PNET_LOG,
         "CMDEV(%d): Incoming DataPossible indication from CMIO. value = %s\n",
         __LINE__,
         data_possible ? "true" : "false");
   }
   switch (p_ar->cmdev_state)
   {
   case PF_CMDEV_STATE_W_ARDY:
   case PF_CMDEV_STATE_W_ARDYCNF:
      p_ar->ready_4_data = data_possible;
      break;
   case PF_CMDEV_STATE_WDATA:
      p_ar->ready_4_data = data_possible;
      if (data_possible == true)
      {
         pf_cmdev_state_ind (net, p_ar, PNET_EVENT_DATA);
         pf_cmdev_set_state (net, p_ar, PF_CMDEV_STATE_DATA);
      }
      break;
   default:
      /* Ignore */
      break;
   }

   return 0;
}

int pf_cmdev_cm_abort (pnet_t * net, pf_ar_t * p_ar)
{
   int res = -1;

   if (p_ar != NULL)
   {
      if (p_ar->ar_param.ar_properties.device_access == true)
      {
         switch (p_ar->cmdev_state)
         {
         case PF_CMDEV_STATE_W_CRES:
            pf_cmdev_set_state (net, p_ar, PF_CMDEV_STATE_ABORT);
            res = 0;
            break;
         case PF_CMDEV_STATE_DATA:
            pf_cmdev_set_state (net, p_ar, PF_CMDEV_STATE_ABORT);
            res = 0;
            break;
         default:
            /* Ignore */
            break;
         }
      }
      else /* CMDEV */
      {
         /* Any state */
         pf_cmdev_set_state (net, p_ar, PF_CMDEV_STATE_ABORT);
         res = 0;
      }
   }
   else
   {
      /* p_ar may be NULL when handling controller induced aborts */
      LOG_INFO (
         PNET_LOG,
         "CMDEV(%d): pf_cmdev_cm_abort_req: p_ar is NULL\n",
         __LINE__);
   }

   /* cm_abort_cnf */
   return res;
}

/**
 * @internal
 * ?
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR entries to remove.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_cm_init_req (pnet_t * net, pf_ar_t * p_ar)
{
   int res = -1;

   if (p_ar != NULL)
   {
      if (p_ar->cmdev_state == PF_CMDEV_STATE_POWER_ON)
      {
         pf_cmdev_set_state (net, p_ar, PF_CMDEV_STATE_W_CIND);
         res = 0;
      }
   }
   else
   {
      LOG_ERROR (
         PNET_LOG,
         "CMDEV(%d): pf_cmdev_cm_init_req: p_ar is NULL\n",
         __LINE__);
   }

   /* cm_init_cnf */
   return res;
}

/**
 * @internal
 * Check if a buffer contains only zero bytes.
 * @param p_start          In:    Start of buffer.
 * @param len              In:    Length of buffer.
 * @return  0  if all bytes are zero.
 *          -1 if at least one byte is non-zero.
 */
int pf_cmdev_check_zero (uint8_t * p_start, uint16_t len)
{
   int ret = -1;
   int zeros = (int)len;

   while ((zeros > 0) && (*p_start == 0))
   {
      p_start++;
      zeros--;
   }

   if (zeros <= 0)
   {
      /* Found nothing but zeroes. */
      ret = 0;
   }
   else
   {
      /* Found something not zero. */
      ret = -1;
   }
   return ret;
}

/**
 * Verify that at least one operational port has a speed of at
 * least 100 MBit/s, full duplex.
 *
 * Otherwise we should refuse to make a connection.
 *
 * See PdevCheckFailed() in Profinet 2.4 Protocol 5.6.3.2.1.4 CMDEV state table
 *
 * @param net              InOut: The p-net stack instance
 * @return 0 if at least one port has the required speed, else -1.
 */
static int pf_cmdev_check_pdev (pnet_t * net)
{
   return (pf_pdport_is_a_fast_port_in_use (net)) ? 0 : -1;
}

/**
 * @internal
 * Check if the UUID is the CM originator UUID.
 *
 * ToDo: Move to pf_types and create a struct to compare against instead
 * of hardcoding here.
 * @param p_uuid           In: The UUID to check.
 * @return  0  if the UUID is correct.
 *          -1 if the UUID is not correct.
 */
static int pf_cmdev_check_cm_initiator_object_uuid (const pf_uuid_t * p_uuid)
{
   int ret = -1;

   if (
      (p_uuid->data1 == 0xDEA00000) && (p_uuid->data2 == 0x6c97) &&
      (p_uuid->data3 == 0x11D1) && (p_uuid->data4[0] == 0x82) &&
      (p_uuid->data4[1] == 0x71))
   {
      ret = 0;
   }

   return ret;
}

/**
 * @internal
 * Check if the string contains only visible characters.
 *
 * A visible character has its ASCII value 0x20 <= x <= 0x7E.
 *
 * @param s                In:    The string to check.
 * @return  0  if the string contains only visible characters.
 *          -1 if at least one character is invalid, or string length is zero.
 */
int pf_cmdev_check_visible_string (const char * s)
{
   int ret = -1;

   if (*s != 0)
   {
      while ((*s != 0) && (isprint ((int)(*s)) != 0))
      {
         s++;
      }
      if (*s == 0)
      {
         ret = 0;
      }
   }

   return ret;
}

/**
 * @internal
 * Check if the Specified AR type is supported.
 *
 * @param ar_type          In:    The AR type to check.
 * @return  0  if the AR type is supported.
 *          -1 if the AR type is not supported.
 */
int pf_cmdev_check_ar_type (uint16_t ar_type)
{
   int ret = -1;

   if (ar_type == PF_ART_IOCAR_SINGLE)
   {
      ret = 0;
   }

   return ret;
}

/**
 * @internal
 * Check the AR param for errors.
 * @param p_ar             InOut: The AR instance.
 * @param p_stat           Out:   Detailed error information if return != 0.
 * @return  0  if no error was detected.
 *          -1 if an error was detected.
 */
static int pf_cmdev_check_ar_param (pf_ar_t * p_ar, pnet_result_t * p_stat)
{
   int ret = 0; /* OK until error found. */

   if (pf_cmdev_check_ar_type (p_ar->ar_param.ar_type) != 0)
   {
      LOG_INFO (PNET_LOG, "CMDEV(%d): Wrong incoming AR type\n", __LINE__);
      pf_set_error (
         p_stat,
         PNET_ERROR_CODE_CONNECT,
         PNET_ERROR_DECODE_PNIO,
         PNET_ERROR_CODE_1_CONN_FAULTY_AR_BLOCK_REQ,
         4);
      ret = -1;
   }
   else if (
      pf_cmdev_check_zero (
         (uint8_t *)&p_ar->ar_param.ar_uuid,
         sizeof (p_ar->ar_param.ar_uuid)) == 0)
   {
      pf_set_error (
         p_stat,
         PNET_ERROR_CODE_CONNECT,
         PNET_ERROR_DECODE_PNIO,
         PNET_ERROR_CODE_1_CONN_FAULTY_AR_BLOCK_REQ,
         5);
      ret = -1;
   }
   /* cm_initiator_mac_add also checked iocr_check */
   else if ((p_ar->ar_param.cm_initiator_mac_add.addr[0] & 0x01) != 0)
   {
      /* Is multicast MAC address */
      pf_set_error (
         p_stat,
         PNET_ERROR_CODE_CONNECT,
         PNET_ERROR_DECODE_PNIO,
         PNET_ERROR_CODE_1_CONN_FAULTY_AR_BLOCK_REQ,
         7);
      ret = -1;
   }
   else if (
      pf_cmdev_check_cm_initiator_object_uuid (
         &p_ar->ar_param.cm_initiator_object_uuid) != 0)
   {
      pf_set_error (
         p_stat,
         PNET_ERROR_CODE_CONNECT,
         PNET_ERROR_DECODE_PNIO,
         PNET_ERROR_CODE_1_CONN_FAULTY_AR_BLOCK_REQ,
         8);
      ret = -1;
   }
   else if (p_ar->ar_param.ar_properties.state != 0x1) /* Must be ACTIVE (1) */
   {
      pf_set_error (
         p_stat,
         PNET_ERROR_CODE_CONNECT,
         PNET_ERROR_DECODE_PNIO,
         PNET_ERROR_CODE_1_CONN_FAULTY_AR_BLOCK_REQ,
         9);
      ret = -1;
   }
   else if (
      p_ar->ar_param.ar_properties.parameterization_server ==
      PF_PS_EXTERNAL_PARAMETER_SERVER)
   {
      pf_set_error (
         p_stat,
         PNET_ERROR_CODE_CONNECT,
         PNET_ERROR_DECODE_PNIO,
         PNET_ERROR_CODE_1_CONN_FAULTY_AR_BLOCK_REQ,
         9);
      ret = -1;
   }
   else if (
      (p_ar->ar_param.ar_properties.device_access == true) &&
      (p_ar->ar_param.ar_type != PF_ART_IOSAR))
   {
      pf_set_error (
         p_stat,
         PNET_ERROR_CODE_CONNECT,
         PNET_ERROR_DECODE_PNIO,
         PNET_ERROR_CODE_1_CONN_FAULTY_AR_BLOCK_REQ,
         9);
      ret = -1;
   }
   else if (p_ar->ar_param.ar_properties.companion_ar == 3)
   {
      pf_set_error (
         p_stat,
         PNET_ERROR_CODE_CONNECT,
         PNET_ERROR_DECODE_PNIO,
         PNET_ERROR_CODE_1_CONN_FAULTY_AR_BLOCK_REQ,
         9);
      ret = -1;
   }
   else if (
      (p_ar->ar_param.cm_initiator_activity_timeout_factor < 1) ||
      (p_ar->ar_param.cm_initiator_activity_timeout_factor > 1000))
   {
      pf_set_error (
         p_stat,
         PNET_ERROR_CODE_CONNECT,
         PNET_ERROR_DECODE_PNIO,
         PNET_ERROR_CODE_1_CONN_FAULTY_AR_BLOCK_REQ,
         10);
      ret = -1;
   }
   /* cm_initiator_udp_rt_port is checked in iocr check */
   else if (
      (p_ar->ar_param.cm_initiator_station_name_len == 0) ||
      (p_ar->ar_param.cm_initiator_station_name_len >
       sizeof (p_ar->ar_param.cm_initiator_station_name) - 1))
   {
      pf_set_error (
         p_stat,
         PNET_ERROR_CODE_CONNECT,
         PNET_ERROR_DECODE_PNIO,
         PNET_ERROR_CODE_1_CONN_FAULTY_AR_BLOCK_REQ,
         12);
      ret = -1;
   }
   else if (
      pf_cmdev_check_visible_string (
         p_ar->ar_param.cm_initiator_station_name) != 0)
   {
      pf_set_error (
         p_stat,
         PNET_ERROR_CODE_CONNECT,
         PNET_ERROR_DECODE_PNIO,
         PNET_ERROR_CODE_1_CONN_FAULTY_AR_BLOCK_REQ,
         13);
      ret = -1;
   }
   else
   {
      p_ar->ar_param.valid = true;
   }

   return ret;
}

/**
 * Find a specific expected API instance in the AR.
 * @param p_ar             In:    The AR instance.
 * @param api_id           In:    The API identifier to find.
 * @param pp_api           Out:   The expected APi instance.
 * @return  0  if a API instance was found.
 *          -1 if the API instance was not found.
 */
static int pf_cmdev_get_exp_api (
   pf_ar_t * p_ar,
   uint32_t api_id,
   pf_exp_api_t ** pp_api)
{
   int ret = -1;
   uint16_t ix;

   ix = 0;
   while ((ix < p_ar->nbr_exp_apis) && (p_ar->exp_apis[ix].api != api_id))
   {
      ix++;
   }

   if (ix < p_ar->nbr_exp_apis)
   {
      *pp_api = &p_ar->exp_apis[ix];

      ret = 0;
   }

   return ret;
}

/**
 * @internal
 * Find the expected sub-module struct indicated by the API identifier and the
 * slot and sub-slot numbers.
 * @param p_ar             In:    The AR instance.
 * @param api_id           In:    The API identifier.
 * @param slot_nbr         In:    The slot number.
 * @param subslot_nbr      In:    The sub-slot number.
 * @param pp_exp_sub       Out:   The sub-module instance.
 * @return  0  if a sub-module instance was found.
 *          -1 if the sub-module instance was not found.
 */
static int pf_cmdev_get_exp_sub (
   pf_ar_t * p_ar,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   pf_exp_submodule_t ** pp_exp_sub)
{
   int ret = -1;
   uint16_t exp_mod_ix;
   uint16_t exp_sub_ix;
   pf_exp_api_t * p_exp_api = NULL;
   pf_exp_module_t * p_exp_mod = NULL;
   pf_exp_submodule_t * p_exp_sub = NULL;

   if (
      (pf_cmdev_get_exp_api (p_ar, api_id, &p_exp_api) == 0) &&
      (p_exp_api != NULL))
   {
      exp_mod_ix = 0;
      while ((exp_mod_ix < p_exp_api->nbr_modules) &&
             (p_exp_api->modules[exp_mod_ix].slot_number != slot_nbr))
      {
         exp_mod_ix++;
      }

      if (exp_mod_ix < p_exp_api->nbr_modules)
      {
         p_exp_mod = &p_exp_api->modules[exp_mod_ix];

         exp_sub_ix = 0;
         while (
            (exp_sub_ix < p_exp_mod->nbr_submodules) &&
            (p_exp_mod->submodules[exp_sub_ix].subslot_number != subslot_nbr))
         {
            exp_sub_ix++;
         }

         if (exp_sub_ix < p_exp_mod->nbr_submodules)
         {
            p_exp_sub = &p_exp_mod->submodules[exp_sub_ix];
            ret = 0;
         }
      }
   }

   *pp_exp_sub = p_exp_sub;

   return ret;
}

/**
 * @internal
 * Calculate which data direction we should use when looking for a data
 * descriptor.
 *
 * For example an output module has only a single data descriptor, which has
 * an output data direction. The IOPS is sent in the output CR, and the IOCS
 * is sent in the input CR.
 *
 * An input + output module has two data descriptors.
 *
 * A module with no cyclic data (PNET_DIR_NO_IO) is implemented an input module.
 *
 * @param submodule_dir       In:   Whether the submodule is IN, IN+OUT etc
 * @param data_dir            In:   The data direction for the IOCR we are
 *                                  working on (input CR or output CR)
 * @param status_type         In:   Whether we are interested in IOCS or IOPS
 * @param resulting_data_dir  Out:  The resulting data direction for use in the
 *                                  search.
 * @return  0  if the direction could be calculated.
 *          -1 for illegal input combinations.
 */
int pf_cmdev_calculate_exp_sub_data_descriptor_direction (
   pnet_submodule_dir_t submodule_dir,
   pf_data_direction_values_t data_dir,
   pf_dev_status_type_t status_type,
   pf_data_direction_values_t * resulting_data_dir)
{
   int ret = -1;

   if (submodule_dir == PNET_DIR_NO_IO || submodule_dir == PNET_DIR_INPUT)
   {
      if (
         (data_dir == PF_DIRECTION_INPUT &&
          status_type == PF_DEV_STATUS_TYPE_IOPS) ||
         (data_dir == PF_DIRECTION_OUTPUT &&
          status_type == PF_DEV_STATUS_TYPE_IOCS))
      {
         *resulting_data_dir = PF_DIRECTION_INPUT;
         ret = 0;
      }
   }
   else if (submodule_dir == PNET_DIR_IO)
   {
      if (
         (data_dir == PF_DIRECTION_INPUT &&
          status_type == PF_DEV_STATUS_TYPE_IOPS) ||
         (data_dir == PF_DIRECTION_OUTPUT &&
          status_type == PF_DEV_STATUS_TYPE_IOCS))
      {
         *resulting_data_dir = PF_DIRECTION_INPUT;
         ret = 0;
      }
      else
      {
         *resulting_data_dir = PF_DIRECTION_OUTPUT;
         ret = 0;
      }
   }
   else if (submodule_dir == PNET_DIR_OUTPUT)
   {
      if (
         (data_dir == PF_DIRECTION_INPUT &&
          status_type == PF_DEV_STATUS_TYPE_IOCS) ||
         (data_dir == PF_DIRECTION_OUTPUT &&
          status_type == PF_DEV_STATUS_TYPE_IOPS))
      {
         *resulting_data_dir = PF_DIRECTION_OUTPUT;
         ret = 0;
      }
   }
   return ret;
}

/**
 * @internal
 * Find the data descriptor within the submodule with specified direction.
 * @param p_exp_sub        In:    The expected sub-module instance.
 * @param dir              In:    The data direction.
 * @param status_type      In:    Whether we are interested in IOCS or IOPS
 * @param pp_desc          Out:   The data descriptor.
 * @return  0  if the data descriptor was found.
 *          -1 if the data descriptor was not found.
 */
static int pf_cmdev_get_exp_sub_data_descriptor (
   pf_exp_submodule_t * p_exp_sub,
   pf_data_direction_values_t dir,
   pf_dev_status_type_t status_type,
   pf_data_descriptor_t ** pp_desc)
{
   int ret = -1;
   uint16_t ix = 0;
   pf_data_direction_values_t data_direction_to_look_for = PF_DIRECTION_INPUT;

   if (p_exp_sub != NULL)
   {
      if (
         pf_cmdev_calculate_exp_sub_data_descriptor_direction (
            p_exp_sub->submodule_properties.type,
            dir,
            status_type,
            &data_direction_to_look_for) == 0)
      {
         for (ix = 0; ix < p_exp_sub->nbr_data_descriptors; ix++)
         {
            if (
               p_exp_sub->data_descriptor[ix].data_direction ==
               data_direction_to_look_for)
            {
               *pp_desc = &p_exp_sub->data_descriptor[ix];
               ret = 0;
               break;
            }
         }
      }
   }

   return ret;
}

/**
 * @internal
 * Collect iodata object IOCS information from IOCR param and expected
 * (sub-)modules.
 * @param p_ar             In:    The AR instance.
 * @param p_iocr           InOut: The IOCR instance.
 * @param dir              In:    The data direction to consider.
 * @param p_stat           Out:   Detailed error information. Not used.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmdev_iocr_setup_iocs (
   pf_ar_t * p_ar,
   pf_iocr_t * p_iocr,
   pf_data_direction_values_t dir,
   pnet_result_t * p_stat)
{
   int ret = 0;
   uint32_t api_id;
   uint16_t slot_nbr;
   uint16_t subslot_nbr;
   pf_iocr_param_t * p_iocr_param;
   pf_iodata_object_t * p_iodata;
   pf_data_descriptor_t * p_desc;
   pf_exp_submodule_t * p_exp_sub;
   uint16_t api_ix;
   uint16_t ix;
   uint16_t iy;
   uint16_t iodata_cnt;
   uint16_t in_len;
   uint16_t out_len;

   in_len = p_iocr->in_length;
   out_len = p_iocr->out_length;
   iodata_cnt = p_iocr->nbr_data_desc;

   p_iocr_param = &p_iocr->param;

   for (api_ix = 0; api_ix < p_iocr_param->nbr_apis; api_ix++)
   {
      for (ix = 0; ix < p_iocr_param->apis[api_ix].nbr_iocs; ix++)
      {
         api_id = p_iocr_param->apis[api_ix].api;
         slot_nbr = p_iocr_param->apis[api_ix].iocs[ix].slot_number;
         subslot_nbr = p_iocr_param->apis[api_ix].iocs[ix].subslot_number;
         if (
            pf_cmdev_get_exp_sub (
               p_ar,
               api_id,
               slot_nbr,
               subslot_nbr,
               &p_exp_sub) != 0)
         {
            LOG_ERROR (
               PNET_LOG,
               "CMDEV(%d): api %u exp slot %u subslot %u not found\n",
               __LINE__,
               (unsigned)api_id,
               (unsigned)slot_nbr,
               (unsigned)subslot_nbr);
            ret = -1;
         }
         else if (
            pf_cmdev_get_exp_sub_data_descriptor (
               p_exp_sub,
               dir,
               PF_DEV_STATUS_TYPE_IOCS,
               &p_desc) != 0)
         {
            LOG_ERROR (
               PNET_LOG,
               "CMDEV(%d): api %u exp slot %u subslot %u and dir %u not "
               "found\n",
               __LINE__,
               (unsigned)api_id,
               (unsigned)slot_nbr,
               (unsigned)subslot_nbr,
               (unsigned)dir);
         }
         else
         {
            LOG_INFO (
               PNET_LOG,
               "CMDEV(%d) Read          IOCS size from API %u slot %u subslot "
               "0x%04x with data direction %s. AREP %u CREP %" PRIu32 ". Data+"
               "IOPS+IOCS %3u+%u+%u bytes\n",
               __LINE__,
               (unsigned)api_id,
               (unsigned)slot_nbr,
               (unsigned)subslot_nbr,
               p_desc->data_direction == PF_DIRECTION_INPUT ? " input"
                                                            : "output",
               p_ar->arep,
               p_iocr->crep,
               (unsigned)p_desc->submodule_data_length,
               (unsigned)p_desc->length_iops,
               p_desc->length_iocs);

            /* Put the iocs into the same as data + iops (if it exists) */
            iy = 0;
            while ((iy < iodata_cnt) &&
                   ((p_iocr->data_desc[iy].api_id != api_id) ||
                    (p_iocr->data_desc[iy].slot_nbr != slot_nbr) ||
                    (p_iocr->data_desc[iy].subslot_nbr != subslot_nbr)))
            {
               iy++;
            }
            if (iy < iodata_cnt)
            {
               /* Re-use any pre-existing data + iops descriptor */
               p_iodata = &p_iocr->data_desc[iy];
            }
            else
            {
               /* Use a new data descriptor. There is no data+iops descriptor */
               p_iodata = &p_iocr->data_desc[iodata_cnt];
               p_iodata->in_use = true;
               iodata_cnt++;

               p_iodata->api_id = api_id;
               p_iodata->slot_nbr = slot_nbr;
               p_iodata->subslot_nbr = subslot_nbr;

               p_iodata->data_length = 0;
               p_iodata->data_offset = 0;
               p_iodata->iops_offset = 0;
               p_iodata->iops_length = 0;
            }

            if (p_exp_sub->submodule_properties.discard_ioxs == true)
            {
               p_iodata->iocs_offset = 0;
               p_iodata->iocs_length = 0;
            }
            else
            {
               p_iodata->iocs_offset =
                  p_iocr_param->apis[api_ix].iocs[ix].frame_offset;
               p_iodata->iocs_length = p_desc->length_iocs;
            }

            if (dir == PF_DIRECTION_INPUT)
            {
               in_len += p_iodata->iocs_length;
            }
            else if (dir == PF_DIRECTION_OUTPUT)
            {
               out_len += p_iodata->iocs_length;
            }
         }
      }
   }

   p_iocr->in_length = in_len;
   p_iocr->out_length = out_len;
   p_iocr->nbr_data_desc = iodata_cnt;

   return ret;
}

/**
 * @internal
 * Collect iodata object data and IOPS information from IOCR param and expected
 * (sub-)modules.
 * @param p_ar             In:    The AR instance.
 * @param p_iocr           InOut: The IOCR instance.
 * @param dir              In:    The data direction to consider.
 * @param p_stat           Out:   Detailed error information. Not used.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmdev_iocr_setup_data_iops (
   pf_ar_t * p_ar,
   pf_iocr_t * p_iocr,
   pf_data_direction_values_t dir,
   pnet_result_t * p_stat)
{
   int ret = 0;
   uint32_t api_id;
   uint16_t slot_nbr;
   uint16_t subslot_nbr;
   pf_iocr_param_t * p_iocr_param;
   pf_iodata_object_t * p_iodata;
   pf_data_descriptor_t * p_desc;
   pf_exp_submodule_t * p_exp_sub;
   uint32_t api_ix;
   uint16_t ix;
   uint16_t iy;
   uint16_t iodata_cnt = 0;
   uint16_t in_len = 0;
   uint16_t out_len = 0;
   uint16_t in_user_len = 0;
   uint16_t out_user_len = 0;

   in_len = p_iocr->in_length;
   out_len = p_iocr->out_length;
   iodata_cnt = p_iocr->nbr_data_desc;

   p_iocr_param = &p_iocr->param;

   for (api_ix = 0; api_ix < p_iocr_param->nbr_apis; api_ix++)
   {
      for (ix = 0; ix < p_iocr_param->apis[api_ix].nbr_io_data; ix++)
      {
         api_id = p_iocr_param->apis[api_ix].api;
         slot_nbr = p_iocr_param->apis[api_ix].io_data[ix].slot_number;
         subslot_nbr = p_iocr_param->apis[api_ix].io_data[ix].subslot_number;
         if (
            pf_cmdev_get_exp_sub (
               p_ar,
               api_id,
               slot_nbr,
               subslot_nbr,
               &p_exp_sub) != 0)
         {
            LOG_ERROR (
               PNET_LOG,
               "CMDEV(%d): api %u exp slot %u subslot %u not found\n",
               __LINE__,
               (unsigned)api_id,
               (unsigned)slot_nbr,
               (unsigned)subslot_nbr);
         }
         else if (
            pf_cmdev_get_exp_sub_data_descriptor (
               p_exp_sub,
               dir,
               PF_DEV_STATUS_TYPE_IOPS,
               &p_desc) != 0)
         {
            LOG_ERROR (
               PNET_LOG,
               "CMDEV(%d): api %u exp slot %u subslot %u and dir %u not "
               "found\n",
               __LINE__,
               (unsigned)api_id,
               (unsigned)slot_nbr,
               (unsigned)subslot_nbr,
               (unsigned)dir);
         }
         else
         {
            LOG_INFO (
               PNET_LOG,
               "CMDEV(%d) Read data and IOPS size from API %u slot %u subslot "
               "0x%04x with data direction %s. AREP %u CREP %" PRIu32 ". Data+"
               "IOPS+IOCS %3u+%u+%u bytes\n",
               __LINE__,
               (unsigned)api_id,
               (unsigned)slot_nbr,
               (unsigned)subslot_nbr,
               p_desc->data_direction == PF_DIRECTION_INPUT ? " input"
                                                            : "output",
               p_ar->arep,
               p_iocr->crep,
               (unsigned)p_desc->submodule_data_length,
               (unsigned)p_desc->length_iops,
               p_desc->length_iocs);

            /* Put the data+iops into the same as iocs (if it exists) */
            iy = 0;
            while ((iy < iodata_cnt) &&
                   ((p_iocr->data_desc[iy].api_id != api_id) ||
                    (p_iocr->data_desc[iy].slot_nbr != slot_nbr) ||
                    (p_iocr->data_desc[iy].subslot_nbr != subslot_nbr)))
            {
               iy++;
            }
            if (iy < iodata_cnt)
            {
               /* Re-use existing data+iops descriptor */
               p_iodata = &p_iocr->data_desc[iy];
            }
            else
            {
               /* Use a new data descriptor */
               p_iodata = &p_iocr->data_desc[iodata_cnt];
               p_iodata->in_use = true;
               iodata_cnt++;

               p_iodata->api_id = api_id;
               p_iodata->slot_nbr = slot_nbr;
               p_iodata->subslot_nbr = subslot_nbr;

               p_iodata->iocs_offset = 0;
               p_iodata->iocs_length = 0;
            }

            if (
               ((dir == PF_DIRECTION_INPUT) &&
                (p_exp_sub->submodule_properties
                    .reduce_input_submodule_data_length == true)) ||
               ((dir == PF_DIRECTION_OUTPUT) &&
                (p_exp_sub->submodule_properties
                    .reduce_output_submodule_data_length == true)))
            {
               p_iodata->data_length = 0;
               p_iodata->data_offset = 0;
            }
            else
            {
               p_iodata->data_length = p_desc->submodule_data_length;
               p_iodata->data_offset =
                  p_iocr_param->apis[api_ix].io_data[ix].frame_offset;
            }

            if (p_exp_sub->submodule_properties.discard_ioxs == true)
            {
               /* Only allowed for NO_IO so data_length = 0 */
               p_iodata->data_length = 0;
               p_iodata->data_offset = 0;

               p_iodata->iops_length = 0;
               p_iodata->iops_offset = 0;
            }
            else
            {
               p_iodata->iops_length = p_desc->length_iops;
               p_iodata->iops_offset =
                  p_iodata->data_offset + p_iodata->data_length;
            }

            if (dir == PF_DIRECTION_INPUT)
            {
               in_len += p_iodata->data_length + p_iodata->iops_length +
                         p_iodata->iocs_length;
               in_user_len += p_iodata->data_length + p_iodata->iops_length +
                              p_iodata->iocs_length;
            }
            else if (dir == PF_DIRECTION_OUTPUT)
            {
               out_len += p_iodata->data_length + p_iodata->iops_length +
                          p_iodata->iocs_length;
               out_user_len += p_iodata->data_length + p_iodata->iops_length +
                               p_iodata->iocs_length;
            }
         }
      }
   }

   p_iocr->in_length = in_len;
   p_iocr->out_length = out_len;
   p_iocr->nbr_data_desc = iodata_cnt;

   return ret;
}

/**
 * @internal
 * Collect iodata object IOCS, data and IOPS information from IOCR param and
 * expected (sub-)modules.
 * @param p_ar             InOut: The AR instance.
 * @param crep             In:    The IOCR index.
 * @param p_stat           Out:   Detailed error information. Not used.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmdev_iocr_setup_desc (
   pf_ar_t * p_ar,
   uint32_t crep,
   pnet_result_t * p_stat)
{
   int ret = 0;
   pf_data_direction_values_t dir;
   pf_iocr_param_t * p_iocr_param;
   pf_iocr_t * p_iocr;

   p_iocr = &p_ar->iocrs[crep];
   p_iocr->p_ar = p_ar;
   p_iocr->crep = crep;

   p_iocr->in_length = 0;
   p_iocr->out_length = 0;
   p_iocr->nbr_data_desc = 0;

   p_iocr_param = &p_iocr->param;

   if (
      (p_iocr_param->iocr_type == PF_IOCR_TYPE_INPUT) ||
      (p_iocr_param->iocr_type == PF_IOCR_TYPE_MC_PROVIDER))
   {
      dir = PF_DIRECTION_INPUT;
   }
   else
   {
      dir = PF_DIRECTION_OUTPUT;
   }

   if (pf_cmdev_iocr_setup_data_iops (p_ar, p_iocr, dir, p_stat) != 0)
   {
      LOG_ERROR (
         PNET_LOG,
         "CMDEV(%d): Failed to collect IOPS information\n",
         __LINE__);
      ret = -1;
   }
   else if (pf_cmdev_iocr_setup_iocs (p_ar, p_iocr, dir, p_stat) != 0)
   {
      LOG_ERROR (
         PNET_LOG,
         "CMDEV(%d): Failed to collect IOCS information\n",
         __LINE__);
      ret = -1;
   }

   return ret;
}

/**
 * Check if two areas overlap (straddle).
 *
 *           x x x          Area 1 (Start = 4, Length = 3)
 *   0 1 2 3 4 5 6 7 8 9
 *   x x                    No overlap
 *     x x                  No overlap
 *       x x                No overlap
 *         x x              Overlap
 *           x x            Overlap
 *             x x          Overlap
 *               x x        Overlap
 *                 x x      No overlap
 *                   x x    No overlap
 *
 * @param start_1          In:    The start of area 1.
 * @param length_1         In:    The length of area 1.
 * @param start_2          In:    The start of area 2.
 * @param length_2         In:    The length of area 2.
 * @return  0  If the areas do NOT straddle each other.
 *          -1 If the areas do overlap.
 */
int pf_cmdev_check_no_straddle (
   uint16_t start_1,
   uint16_t length_1,
   uint16_t start_2,
   uint16_t length_2)
{
   int ret = 0;

   if ((length_1 > 0) && (length_2 > 0))
   {
      if (start_1 <= start_2)
      {
         if ((start_1 + length_1) > start_2)
         {
            ret = -1;
         }
      }
      else
      {
         if (start_1 < (start_2 + length_2))
         {
            ret = -1;
         }
      }
   }

   return ret;
}

/**
 * Check if a data_desc overlaps any previous data_desc in same iocrs[].
 * @param p_iocr           In:    The iocrs instance.
 * @param ix_this          In:    The data_desc index to verify.
 * @param p_stat           Out:   Detailed error information if return != 0.
 * @return  0  If this area does not overlap any previously defined area.
 *          -1 If there is overlap.
 */
static int pf_cmdev_check_iocr_overlap (
   pf_iocr_t * p_iocr,
   uint16_t ix_this,
   pnet_result_t * p_stat)
{
   /*
    * If area io_this overlaps with any previous areas [0..(io_this-1)] then
    * there is an overlap.
    */
   int ret = 0; /* Assume no overlap */
   uint16_t ix;
   uint16_t start;
   uint16_t len;

   for (ix = 0; ix < ix_this; ix++)
   {
      /* Check data area against all previous data_desc */
      start = p_iocr->data_desc[ix_this].data_offset;
      len = p_iocr->data_desc[ix_this].data_length;
      if (len > 0)
      {
         if (
            pf_cmdev_check_no_straddle (
               start,
               len,
               p_iocr->data_desc[ix].data_offset,
               p_iocr->data_desc[ix].data_length) != 0)
         {
            ret = -1;
         }
         else if (
            pf_cmdev_check_no_straddle (
               start,
               len,
               p_iocr->data_desc[ix].iocs_offset,
               p_iocr->data_desc[ix].iocs_length) != 0)
         {
            ret = -1;
         }
         else if (
            pf_cmdev_check_no_straddle (
               start,
               len,
               p_iocr->data_desc[ix].iops_offset,
               p_iocr->data_desc[ix].iops_length) != 0)
         {
            ret = -1;
         }
      }
      /* Check IOPS area against all previous data_desc */
      start = p_iocr->data_desc[ix_this].iops_offset;
      len = p_iocr->data_desc[ix_this].iops_length;
      if (len > 0)
      {
         if (
            pf_cmdev_check_no_straddle (
               start,
               len,
               p_iocr->data_desc[ix].data_offset,
               p_iocr->data_desc[ix].data_length) != 0)
         {
            ret = -1;
         }
         else if (
            pf_cmdev_check_no_straddle (
               start,
               len,
               p_iocr->data_desc[ix].iocs_offset,
               p_iocr->data_desc[ix].iocs_length) != 0)
         {
            ret = -1;
         }
         else if (
            pf_cmdev_check_no_straddle (
               start,
               len,
               p_iocr->data_desc[ix].iops_offset,
               p_iocr->data_desc[ix].iops_length) != 0)
         {
            ret = -1;
         }
      }
   }

   if (ret != 0)
   {
      pf_set_error (
         p_stat,
         PNET_ERROR_CODE_CONNECT,
         PNET_ERROR_DECODE_PNIO,
         PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
         24);
   }
   else
   {
      for (ix = 0; ix < ix_this; ix++)
      {
         /* Check IOCS area against all previous data_desc */
         start = p_iocr->data_desc[ix_this].iocs_offset;
         len = p_iocr->data_desc[ix_this].iocs_length;
         if (len > 0)
         {
            if (
               pf_cmdev_check_no_straddle (
                  start,
                  len,
                  p_iocr->data_desc[ix].data_offset,
                  p_iocr->data_desc[ix].data_length) != 0)
            {
               ret = -1;
            }
            else if (
               pf_cmdev_check_no_straddle (
                  start,
                  len,
                  p_iocr->data_desc[ix].iocs_offset,
                  p_iocr->data_desc[ix].iocs_length) != 0)
            {
               ret = -1;
            }
            else if (
               pf_cmdev_check_no_straddle (
                  start,
                  len,
                  p_iocr->data_desc[ix].iops_offset,
                  p_iocr->data_desc[ix].iops_length) != 0)
            {
               ret = -1;
            }
         }
      }

      if (ret != 0)
      {
         pf_set_error (
            p_stat,
            PNET_ERROR_CODE_CONNECT,
            PNET_ERROR_DECODE_PNIO,
            PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
            28);
      }
   }

   return ret;
}

/**
 * @internal
 * Perform final validation of IOCR APIs, after the data_desc has been set up.
 * @param p_ar             In:    The AR instance.
 * @param p_stat           Out:   Detailed error information if return != 0.
 * @return  0  if no error was found.
 *          -1 if an error was found.
 */
static int pf_cmdev_check_iocr_apis (pf_ar_t * p_ar, pnet_result_t * p_stat)
{
   int ret = 0;
   pf_iocr_param_t * p_iocr_param;
   uint16_t api_ix = 0;
   pf_api_entry_t * p_iocr_api = NULL;
   uint16_t io_ix = 0;  /* io_data/iocs index */
   uint16_t io2_ix = 0; /* io_data/iocs index */
   uint16_t ix = 0;     /* exp_api index */
   uint16_t iy = 0;     /* exp_module index */
   uint16_t iz = 0;     /* exp_submodule index */
   pf_exp_api_t * p_exp_api = NULL;
   pf_exp_module_t * p_exp_mod = NULL;
   pf_exp_submodule_t * p_exp_sub = NULL;
   uint16_t slot_nbr;
   uint16_t subslot_nbr;
   uint16_t combo_cnt = 0;

   for (ix = 0; ix < p_ar->nbr_iocrs; ix++)
   {
      p_iocr_param = &p_ar->iocrs[ix].param;
      for (api_ix = 0; api_ix < p_iocr_param->nbr_apis; api_ix++)
      {
         p_iocr_api = &p_iocr_param->apis[api_ix];
         /* The IOCR API must be present in the expected API */
         p_exp_api = NULL;
         if (pf_cmdev_get_exp_api (p_ar, p_iocr_api->api, &p_exp_api) != 0)
         {
            pf_set_error (
               p_stat,
               PNET_ERROR_CODE_CONNECT,
               PNET_ERROR_DECODE_PNIO,
               PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
               20);
            ret = -1;
         }
         else if ((p_iocr_api->nbr_io_data == 0) && (p_iocr_api->nbr_iocs == 0))
         {
            pf_set_error (
               p_stat,
               PNET_ERROR_CODE_CONNECT,
               PNET_ERROR_DECODE_PNIO,
               PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
               21);
            ret = -1;
         }

         if ((ret == 0) && (p_exp_api != NULL))
         {
            /* %%%%%%%%%%%%%%%%%% io_data %%%%%%%%%%%%%%%%%%%%% */

            if (ret == 0)
            {
               /* Within each io_data in each API the slot/subslot combo must be
                * unique. */
               for (io_ix = 0; io_ix < p_iocr_api->nbr_io_data; io_ix++)
               {
                  slot_nbr = p_iocr_api->io_data[io_ix].slot_number;
                  subslot_nbr = p_iocr_api->io_data[io_ix].subslot_number;
                  combo_cnt = 0;
                  for (io2_ix = 0; io2_ix < p_iocr_api->nbr_io_data; io2_ix++)
                  {
                     if (
                        (p_iocr_api->io_data[io2_ix].slot_number == slot_nbr) &&
                        (p_iocr_api->io_data[io2_ix].subslot_number ==
                         subslot_nbr))
                     {
                        combo_cnt++;
                     }
                  }
                  if (combo_cnt != 1)
                  {
                     pf_set_error (
                        p_stat,
                        PNET_ERROR_CODE_CONNECT,
                        PNET_ERROR_DECODE_PNIO,
                        PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
                        23);
                     ret = -1;
                  }
               }
            }

            for (io_ix = 0; io_ix < p_iocr_api->nbr_io_data; io_ix++)
            {
               /* io_data slot must be in expected modules list */
               p_exp_mod = NULL;
               p_exp_sub = NULL;
               for (iy = 0; iy < p_exp_api->nbr_modules; iy++)
               {
                  if (
                     p_iocr_api->io_data[io_ix].slot_number ==
                     p_exp_api->modules[iy].slot_number)
                  {
                     p_exp_mod = &p_exp_api->modules[iy];
                  }
               }
               if (p_exp_mod == NULL)
               {
                  pf_set_error (
                     p_stat,
                     PNET_ERROR_CODE_CONNECT,
                     PNET_ERROR_DECODE_PNIO,
                     PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
                     22);
                  ret = -1;
               }
               else
               {
                  /* Slot/subslot must be in corresponding exp module/exp
                   * submodule. */
                  for (iz = 0; iz < p_exp_mod->nbr_submodules; iz++)
                  {
                     if (
                        p_iocr_api->io_data[io_ix].subslot_number ==
                        p_exp_mod->submodules[iz].subslot_number)
                     {
                        p_exp_sub = &p_exp_mod->submodules[iz];
                     }
                  }

                  if (p_exp_sub == NULL)
                  {
                     pf_set_error (
                        p_stat,
                        PNET_ERROR_CODE_CONNECT,
                        PNET_ERROR_DECODE_PNIO,
                        PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
                        23);
                     ret = -1;
                  }
               }

               if ((ret == 0) && (p_exp_sub != NULL))
               {
                  if (
                     ((p_exp_sub->submodule_properties.type == PNET_DIR_NO_IO) ||
                      (p_exp_sub->submodule_properties.type ==
                       PNET_DIR_INPUT)) &&
                     ((p_iocr_param->iocr_type == PF_IOCR_TYPE_OUTPUT) ||
                      (p_iocr_param->iocr_type == PF_IOCR_TYPE_MC_CONSUMER)))
                  {
                     pf_set_error (
                        p_stat,
                        PNET_ERROR_CODE_CONNECT,
                        PNET_ERROR_DECODE_PNIO,
                        PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
                        23);
                     ret = -1;
                  }
                  else if (
                     (p_exp_sub->submodule_properties.type ==
                      PNET_DIR_OUTPUT) &&
                     ((p_iocr_param->iocr_type == PF_IOCR_TYPE_INPUT) ||
                      (p_iocr_param->iocr_type == PF_IOCR_TYPE_MC_PROVIDER)))
                  {
                     pf_set_error (
                        p_stat,
                        PNET_ERROR_CODE_CONNECT,
                        PNET_ERROR_DECODE_PNIO,
                        PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
                        23);
                     ret = -1;
                  }
               }
            }

            /* %%%%%%%%%%%%%%%%%% IOCS %%%%%%%%%%%%%%%%%%%%% */

            if (ret == 0)
            {
               /* Within each iocs in each API the slot/subslot combo must be
                * unique. */
               for (io_ix = 0; io_ix < p_iocr_api->nbr_iocs; io_ix++)
               {
                  slot_nbr = p_iocr_api->iocs[io_ix].slot_number;
                  subslot_nbr = p_iocr_api->iocs[io_ix].subslot_number;
                  combo_cnt = 0;
                  for (io2_ix = 0; io2_ix < p_iocr_api->nbr_iocs; io2_ix++)
                  {
                     if (
                        (p_iocr_api->iocs[io2_ix].slot_number == slot_nbr) &&
                        (p_iocr_api->iocs[io2_ix].subslot_number ==
                         subslot_nbr))
                     {
                        combo_cnt++;
                     }
                  }
                  if (combo_cnt != 1)
                  {
                     pf_set_error (
                        p_stat,
                        PNET_ERROR_CODE_CONNECT,
                        PNET_ERROR_DECODE_PNIO,
                        PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
                        27);
                     ret = -1;
                  }
               }
            }

            if (ret == 0)
            {
               for (io_ix = 0; io_ix < p_iocr_api->nbr_iocs; io_ix++)
               {
                  p_exp_mod = NULL;
                  p_exp_sub = NULL;
                  for (iy = 0; iy < p_exp_api->nbr_modules; iy++)
                  {
                     if (
                        p_iocr_api->iocs[io_ix].slot_number ==
                        p_exp_api->modules[iy].slot_number)
                     {
                        p_exp_mod = &p_exp_api->modules[iy];
                     }
                  }
                  if (p_exp_mod == NULL)
                  {
                     pf_set_error (
                        p_stat,
                        PNET_ERROR_CODE_CONNECT,
                        PNET_ERROR_DECODE_PNIO,
                        PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
                        27);
                     ret = -1;
                  }
                  else
                  {
                     /* Must be in corresponding exp module/exp submodule. */
                     for (iz = 0; iz < p_exp_mod->nbr_submodules; iz++)
                     {
                        if (
                           p_iocr_api->iocs[io_ix].subslot_number ==
                           p_exp_mod->submodules[iz].subslot_number)
                        {
                           p_exp_sub = &p_exp_mod->submodules[iz];
                        }
                     }
                     if (p_exp_sub == NULL)
                     {
                        pf_set_error (
                           p_stat,
                           PNET_ERROR_CODE_CONNECT,
                           PNET_ERROR_DECODE_PNIO,
                           PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
                           27);
                        ret = -1;
                     }
                  }

                  if ((ret == 0) && (p_exp_sub != NULL))
                  {
                     if (
                        ((p_exp_sub->submodule_properties.type ==
                          PNET_DIR_NO_IO) ||
                         (p_exp_sub->submodule_properties.type ==
                          PNET_DIR_INPUT)) &&
                        (p_iocr_param->iocr_type == PF_IOCR_TYPE_INPUT))
                     {
                        pf_set_error (
                           p_stat,
                           PNET_ERROR_CODE_CONNECT,
                           PNET_ERROR_DECODE_PNIO,
                           PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
                           27);
                        ret = -1;
                     }
                     else if (
                        (p_exp_sub->submodule_properties.type ==
                         PNET_DIR_OUTPUT) &&
                        (p_iocr_param->iocr_type == PF_IOCR_TYPE_OUTPUT))
                     {
                        pf_set_error (
                           p_stat,
                           PNET_ERROR_CODE_CONNECT,
                           PNET_ERROR_DECODE_PNIO,
                           PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
                           27);
                        ret = -1;
                     }

                     if (
                        (ret == 0) &&
                        (p_exp_sub->submodule_properties.type == PNET_DIR_IO))
                     {
                        /* Must have 2 data descriptors */
                        if (p_exp_sub->nbr_data_descriptors != 2)
                        {
                           pf_set_error (
                              p_stat,
                              PNET_ERROR_CODE_CONNECT,
                              PNET_ERROR_DECODE_PNIO,
                              PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
                              28);
                           ret = -1;
                        }
                     }
                  }
               }
            }
         }
      }

      for (io_ix = 0; io_ix < p_ar->iocrs[ix].nbr_data_desc; io_ix++)
      {
         if (ret == 0)
         {
            if (
               (p_ar->iocrs[ix].data_desc[io_ix].data_offset +
                p_ar->iocrs[ix].data_desc[io_ix].data_length) >
               p_iocr_param->c_sdu_length)
            {
               pf_set_error (
                  p_stat,
                  PNET_ERROR_CODE_CONNECT,
                  PNET_ERROR_DECODE_PNIO,
                  PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
                  24);
               ret = -1;
            }
            else if (
               (p_ar->iocrs[ix].data_desc[io_ix].iops_offset +
                p_ar->iocrs[ix].data_desc[io_ix].iops_length) >
               p_iocr_param->c_sdu_length)
            {
               pf_set_error (
                  p_stat,
                  PNET_ERROR_CODE_CONNECT,
                  PNET_ERROR_DECODE_PNIO,
                  PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
                  24);
               ret = -1;
            }
            else if (
               (p_ar->iocrs[ix].data_desc[io_ix].iocs_offset +
                p_ar->iocrs[ix].data_desc[io_ix].iocs_length) >
               p_iocr_param->c_sdu_length)
            {
               pf_set_error (
                  p_stat,
                  PNET_ERROR_CODE_CONNECT,
                  PNET_ERROR_DECODE_PNIO,
                  PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
                  28);
               ret = -1;
            }
            else if (
               pf_cmdev_check_iocr_overlap (&p_ar->iocrs[ix], io_ix, p_stat) !=
               0)
            {
               ret = -1;
            }
         }
      }

      if (ret != 0)
      {
         p_iocr_param->valid = false;
      }
   }

   return ret;
}

/**
 * @internal
 * Check the IOCR param of an AR for errors.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param p_stat           Out:   Detailed error information if return != 0.
 * @return  0  if no error was found
 *          -1 if an error was found.
 */
static int pf_cmdev_check_iocr_param (
   pnet_t * net,
   pf_ar_t * p_ar,
   pnet_result_t * p_stat)
{
   int ret = 0; /* OK until error found */
   uint16_t ix;
   pf_iocr_param_t * p_iocr;

   for (ix = 0; ix < p_ar->nbr_iocrs; ix++)
   {
      p_iocr = &p_ar->iocrs[ix].param;

      if (p_ar->ar_param.ar_properties.device_access == true)
      {
         pf_set_error (
            p_stat,
            PNET_ERROR_CODE_CONNECT,
            PNET_ERROR_DECODE_PNIO,
            PNET_ERROR_CODE_1_CMRPC,
            PNET_ERROR_CODE_2_CMRPC_UNKNOWN_BLOCKS);
         ret = -1;
      }
      else if (
         (p_iocr->iocr_type < PF_IOCR_TYPE_INPUT) ||
         (p_iocr->iocr_type > PF_IOCR_TYPE_MC_CONSUMER))
      {
         pf_set_error (
            p_stat,
            PNET_ERROR_CODE_CONNECT,
            PNET_ERROR_DECODE_PNIO,
            PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
            4);
         ret = -1;
      }
      /* Defer IOCReference checks to below. */
      else if (
         (p_iocr->iocr_properties.rt_class != PF_RT_CLASS_UDP) &&
         (p_iocr->lt_field != PNAL_ETHTYPE_PROFINET))
      {
         pf_set_error (
            p_stat,
            PNET_ERROR_CODE_CONNECT,
            PNET_ERROR_DECODE_PNIO,
            PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
            6);
         ret = -1;
      }
      else if (
         (p_iocr->iocr_properties.rt_class == PF_RT_CLASS_UDP) &&
         (p_iocr->lt_field != PNAL_ETHTYPE_IP))
      {
         pf_set_error (
            p_stat,
            PNET_ERROR_CODE_CONNECT,
            PNET_ERROR_DECODE_PNIO,
            PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
            6);
         ret = -1;
      }
      else if (
         (p_ar->ar_param.ar_properties.startup_mode == false) &&
         ((p_iocr->iocr_properties.rt_class < PF_RT_CLASS_1) ||
          (p_iocr->iocr_properties.rt_class > PF_RT_CLASS_STREAM)))
      {
         /* Legacy startup mode:     1 <= rtclass <= 5
            Profinet 2.4 Protocol table 984 "IOCRBlockReq" */
         pf_set_error (
            p_stat,
            PNET_ERROR_CODE_CONNECT,
            PNET_ERROR_DECODE_PNIO,
            PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
            7);
         ret = -1;
      }
      else if (
         (p_ar->ar_param.ar_properties.startup_mode == true) &&
         ((p_iocr->iocr_properties.rt_class < PF_RT_CLASS_2) ||
          (p_iocr->iocr_properties.rt_class > PF_RT_CLASS_STREAM)))
      {
         /* Advanced startup mode:     2 <= rtclass <= 5
            Profinet 2.4 Protocol table 984 "IOCRBlockReq" */
         pf_set_error (
            p_stat,
            PNET_ERROR_CODE_CONNECT,
            PNET_ERROR_DECODE_PNIO,
            PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
            7);
         ret = -1;
      }
      /*
       * Spec says to check reserved_1 and reserved_2, nothing about reserved_3.
       * Spec also says reserved_2 may have any value and shall not be checked
       * by device. Lets do what is intended!
       */
      else if (
         (p_iocr->iocr_properties.reserved_1 != 0) ||
         (p_iocr->iocr_properties.reserved_3 != 0))
      {
         pf_set_error (
            p_stat,
            PNET_ERROR_CODE_CONNECT,
            PNET_ERROR_DECODE_PNIO,
            PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
            7);
         ret = -1;
      }
      else if (
         ((p_iocr->iocr_properties.rt_class == PF_RT_CLASS_UDP) &&
          ((p_iocr->c_sdu_length < 12) || (p_iocr->c_sdu_length > 1440))) ||
         ((p_iocr->iocr_properties.rt_class == PF_RT_CLASS_1) &&
          ((p_iocr->c_sdu_length < 40) || (p_iocr->c_sdu_length > 1440))))
      {
         pf_set_error (
            p_stat,
            PNET_ERROR_CODE_CONNECT,
            PNET_ERROR_DECODE_PNIO,
            PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
            8);
         ret = -1;
      }
      else if (
         ((p_iocr->iocr_type == PF_IOCR_TYPE_MC_CONSUMER) ||
          (p_iocr->iocr_type == PF_IOCR_TYPE_MC_PROVIDER)) &&
         (((p_iocr->iocr_properties.rt_class == PF_RT_CLASS_1) &&
           ((p_iocr->frame_id < 0xF800) || (p_iocr->frame_id > 0xFBFF))) ||
          ((p_iocr->iocr_properties.rt_class == PF_RT_CLASS_2) &&
           ((p_iocr->frame_id < 0xBC00) || (p_iocr->frame_id > 0xBFFF))) ||
          ((p_iocr->iocr_properties.rt_class == PF_RT_CLASS_3) &&
           ((p_iocr->frame_id < 0x0100) || (p_iocr->frame_id > 0x7FFF))) ||
          ((p_iocr->iocr_properties.rt_class == PF_RT_CLASS_UDP) &&
           ((p_iocr->frame_id < 0xF800) || (p_iocr->frame_id > 0xFBFF)))))
      {
         pf_set_error (
            p_stat,
            PNET_ERROR_CODE_CONNECT,
            PNET_ERROR_DECODE_PNIO,
            PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
            9);
         ret = -1;
      }
      else if (
         (p_iocr->iocr_type == PF_IOCR_TYPE_INPUT) &&
         (((p_iocr->iocr_properties.rt_class == PF_RT_CLASS_1) &&
           ((p_iocr->frame_id < 0xC000) || (p_iocr->frame_id > 0xF7FF))) ||
          ((p_iocr->iocr_properties.rt_class == PF_RT_CLASS_2) &&
           ((p_iocr->frame_id < 0x8000) || (p_iocr->frame_id > 0xBBFF))) ||
          ((p_iocr->iocr_properties.rt_class == PF_RT_CLASS_3) &&
           ((p_iocr->frame_id < 0x0100) || (p_iocr->frame_id > 0x7FFF))) ||
          ((p_iocr->iocr_properties.rt_class == PF_RT_CLASS_UDP) &&
           ((p_iocr->frame_id < 0xC000) || (p_iocr->frame_id > 0xF7FF)))))
      {
         pf_set_error (
            p_stat,
            PNET_ERROR_CODE_CONNECT,
            PNET_ERROR_DECODE_PNIO,
            PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
            9);
         ret = -1;
      }
      else if (
         (p_iocr->send_clock_factor != 1) && (p_iocr->send_clock_factor != 2) &&
         (p_iocr->send_clock_factor != 4) && (p_iocr->send_clock_factor != 8) &&
         (p_iocr->send_clock_factor != 16) &&
         (p_iocr->send_clock_factor != 32) &&
         (p_iocr->send_clock_factor != 64) &&
         (p_iocr->send_clock_factor != 128))
      {
         pf_set_error (
            p_stat,
            PNET_ERROR_CODE_CONNECT,
            PNET_ERROR_DECODE_PNIO,
            PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
            10);
         ret = -1;
      }
      /* ToDo: Handle IOSAR:
       * else if (p_iocr->send_clock_factor != local_active_send_clock_factor)
       * // From established AR
       * {
       *    pf_set_error(p_stat, PNET_ERROR_CODE_CONNECT,
       * PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
       * 10); ret = -1;
       * }
       */
      else if (
         ((p_iocr->iocr_properties.rt_class == PF_RT_CLASS_1) ||
          (p_iocr->iocr_properties.rt_class == PF_RT_CLASS_2) ||
          (p_iocr->iocr_properties.rt_class == PF_RT_CLASS_3)) &&
         (((p_iocr->reduction_ratio < 1) || (p_iocr->reduction_ratio > 512)) ||
          ((p_iocr->reduction_ratio >= 256) &&
           (p_iocr->send_clock_factor > 64)) ||
          ((p_iocr->reduction_ratio == 512) &&
           (p_iocr->send_clock_factor > 32)) ||
          (pf_fspm_get_min_device_interval (net) >
           p_iocr->send_clock_factor * p_iocr->reduction_ratio)))
      {
         pf_set_error (
            p_stat,
            PNET_ERROR_CODE_CONNECT,
            PNET_ERROR_DECODE_PNIO,
            PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
            11);
         ret = -1;
      }
      else if (
         (p_iocr->iocr_properties.rt_class == PF_RT_CLASS_UDP) &&
         (((p_iocr->reduction_ratio < 1) ||
           (p_iocr->reduction_ratio > 16384)) ||
          ((p_iocr->reduction_ratio >= 8192) &&
           (p_iocr->send_clock_factor > 64)) ||
          ((p_iocr->reduction_ratio == 16384) &&
           (p_iocr->send_clock_factor > 32))))
      {
         pf_set_error (
            p_stat,
            PNET_ERROR_CODE_CONNECT,
            PNET_ERROR_DECODE_PNIO,
            PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
            11);
         ret = -1;
      }
      else if ((p_iocr->phase == 0) || (p_iocr->phase > p_iocr->reduction_ratio))
      {
         pf_set_error (
            p_stat,
            PNET_ERROR_CODE_CONNECT,
            PNET_ERROR_DECODE_PNIO,
            PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
            12);
         ret = -1;
      }
      else if (
         (p_iocr->frame_send_offset != 0xffffffffUL) && /* Best effort is OK */
         ((p_iocr->frame_send_offset >=
           (p_iocr->send_clock_factor * 31250UL)) || /* ns */
          ((p_iocr->frame_send_offset >= 0x003d0900UL) &&
           (p_iocr->frame_send_offset <= 0xfffffffeUL))))
      {
         pf_set_error (
            p_stat,
            PNET_ERROR_CODE_CONNECT,
            PNET_ERROR_DECODE_PNIO,
            PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
            14);
         ret = -1;
      }
      /* No check for dataHoldFactor (legacy). */
      else if (
         ((p_iocr->data_hold_factor < 1) ||
          (p_iocr->data_hold_factor > 0x1e00)) ||
         ((p_iocr->iocr_properties.rt_class == PF_RT_CLASS_UDP) &&
          (((uint32_t)p_iocr->data_hold_factor * p_iocr->reduction_ratio *
            p_iocr->send_clock_factor * 1000) /
              32 >
           61440000)) ||
         (((p_iocr->iocr_properties.rt_class == PF_RT_CLASS_1) ||
           (p_iocr->iocr_properties.rt_class == PF_RT_CLASS_2) ||
           (p_iocr->iocr_properties.rt_class == PF_RT_CLASS_3)) &&
          (((uint32_t)p_iocr->data_hold_factor * p_iocr->reduction_ratio *
            p_iocr->send_clock_factor * 1000) /
              32 >
           1920000)))
      {
         pf_set_error (
            p_stat,
            PNET_ERROR_CODE_CONNECT,
            PNET_ERROR_DECODE_PNIO,
            PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
            16);
         ret = -1;
      }
      else if (p_iocr->iocr_tag_header.vlan_id != 0)
      {
         pf_set_error (
            p_stat,
            PNET_ERROR_CODE_CONNECT,
            PNET_ERROR_DECODE_PNIO,
            PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
            17);
         ret = -1;
      }
      else if (p_iocr->iocr_tag_header.iocr_user_priority != 6) /* IOCR_PRIORITY
                                                                 */
      {
         pf_set_error (
            p_stat,
            PNET_ERROR_CODE_CONNECT,
            PNET_ERROR_DECODE_PNIO,
            PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
            17);
         ret = -1;
      }
      else if (
         ((p_iocr->iocr_type == PF_IOCR_TYPE_MC_PROVIDER) ||
          (p_iocr->iocr_type == PF_IOCR_TYPE_MC_CONSUMER)) &&
         ((p_iocr->iocr_multicast_mac_add.addr[0] & 0x01) == 0)) /* Not an MC
                                                                    addr */
      {
         pf_set_error (
            p_stat,
            PNET_ERROR_CODE_CONNECT,
            PNET_ERROR_DECODE_PNIO,
            PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
            18);
         ret = -1;
      }
      else if (p_iocr->nbr_apis == 0)
      {
         pf_set_error (
            p_stat,
            PNET_ERROR_CODE_CONNECT,
            PNET_ERROR_DECODE_PNIO,
            PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ,
            19);
         ret = -1;
      }
      /* Check of apis later */
      else if (
         (p_iocr->iocr_properties.rt_class == PF_RT_CLASS_UDP) &&
         (pf_cmdev_check_zero (
             p_ar->ar_param.cm_initiator_mac_add.addr,
             sizeof (p_ar->ar_param.cm_initiator_mac_add.addr)) != 0))
      {
         pf_set_error (
            p_stat,
            PNET_ERROR_CODE_CONNECT,
            PNET_ERROR_DECODE_PNIO,
            PNET_ERROR_CODE_1_CONN_FAULTY_AR_BLOCK_REQ,
            7);
         ret = -1;
      }
      else if (
         (p_iocr->iocr_properties.rt_class == PF_RT_CLASS_UDP) &&
         (p_ar->ar_param.cm_initiator_udp_rt_port <= 0x03FF))
      {
         pf_set_error (
            p_stat,
            PNET_ERROR_CODE_CONNECT,
            PNET_ERROR_DECODE_PNIO,
            PNET_ERROR_CODE_1_CONN_FAULTY_AR_BLOCK_REQ,
            11);
         ret = -1;
      }
      else
      {
         p_iocr->valid = true;
      }
   }

   return ret;
}

/**
 * @internal
 * Configure an expected sub-module.
 *
 * This function configures an expected sub-module.
 * Perform final checks of parameters and let the application plug the
 * sub-module if needed.
 *
 * Triggers user call-back \a pnet_exp_submodule_ind()
 *
 * @param net              InOut: The p-net stack instance
 * @param p_exp_api        In:    The expected API instance.
 * @param p_exp_mod        In:    The expected sub-module instance.
 * @param p_cfg_api        InOut: The configured API instance.
 * @param p_cfg_slot       InOut: The configured module instance.
 * @param p_stat           Out:   Detailed error information if return != 0.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmdev_exp_submodule_configure (
   pnet_t * net,
   const pf_exp_api_t * p_exp_api,
   const pf_exp_module_t * p_exp_mod,
   pf_api_t * p_cfg_api,
   pf_slot_t * p_cfg_slot,
   pnet_result_t * p_stat)
{
   int ret = -1;
   uint16_t sub_ix;
   uint16_t subslot_nbr;
   const pf_exp_submodule_t * p_exp_sub = NULL;
   pf_subslot_t * p_cfg_sub = NULL;
   uint16_t api_ix;
   uint16_t ix;
   uint16_t data_ix;
   uint16_t cnt;
   pf_ar_t * p_ar;
   pf_iocr_param_t * p_iocr_param;
   pf_api_entry_t * p_iocr_api;
   uint16_t i;
   pnet_data_cfg_t exp_data = {0};

   ret = 0; /* Assume all goes well */
   for (sub_ix = 0; sub_ix < p_exp_mod->nbr_submodules; sub_ix++)
   {
      p_exp_sub = &p_exp_mod->submodules[sub_ix];
      if (
         pf_cmdev_get_subslot (
            p_cfg_slot,
            p_exp_sub->subslot_number,
            &p_cfg_sub) != 0)
      {
         memset (&exp_data, 0, sizeof (exp_data));
         for (i = 0; i < p_exp_sub->nbr_data_descriptors; i++)
         {
            if (p_exp_sub->data_descriptor[i].data_direction == PF_DIRECTION_INPUT)
            {
               exp_data.data_dir |= PNET_DIR_INPUT;
               exp_data.insize =
                  p_exp_sub->data_descriptor[i].submodule_data_length;
            }
            if (p_exp_sub->data_descriptor[i].data_direction == PF_DIRECTION_OUTPUT)
            {
               exp_data.data_dir |= PNET_DIR_OUTPUT;
               exp_data.outsize =
                  p_exp_sub->data_descriptor[i].submodule_data_length;
            }
         }

         /*
          * Return code is not interesting here.
          */
         (void)pf_fspm_exp_submodule_ind (
            net,
            p_exp_api->api,
            p_exp_mod->slot_number,
            p_exp_sub->subslot_number,
            p_exp_mod->module_ident_number,
            p_exp_sub->submodule_ident_number,
            &exp_data);
      }
      else
      {
         if (p_exp_sub->submodule_ident_number != p_cfg_sub->submodule_ident_number)
         {
            LOG_DEBUG (
               PNET_LOG,
               "CMDEV(%d): Substitute mode for slot %u subslot 0x%04x Sub"
               "module expected 0x%08" PRIx32 " configured 0x%08" PRIx32 "\n",
               __LINE__,
               p_exp_mod->slot_number,
               p_exp_sub->subslot_number,
               p_exp_sub->submodule_ident_number,
               p_cfg_sub->submodule_ident_number);

            p_cfg_sub->submodule_state.ident_info = PF_SUBMOD_PLUG_SUBSTITUTE;
         }
         else
         {
            p_cfg_sub->submodule_state.ident_info = PF_SUBMOD_PLUG_OK;
         }
      }

      if (ret == 0)
      {
         /* Within each slot a sub-slot number may only be specified once. */
         subslot_nbr = p_exp_sub->subslot_number;
         cnt = 0;
         for (ix = 0; ix < sub_ix; ix++)
         {
            if (subslot_nbr == p_exp_mod->submodules[ix].subslot_number)
            {
               cnt++;
            }
         }
         if (cnt != 0)
         {
            pf_set_error (
               p_stat,
               PNET_ERROR_CODE_CONNECT,
               PNET_ERROR_DECODE_PNIO,
               PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ,
               10);
            ret = -1;
         }
      }

      if (ret == 0)
      {
         /* Each exp sub-slot must be used in at least one IOCR. */
         cnt = 0;
         p_ar = p_cfg_slot->p_ar;
         for (ix = 0; ix < p_ar->nbr_iocrs; ix++)
         {
            p_iocr_param = &p_ar->iocrs[ix].param;
            for (api_ix = 0; api_ix < p_iocr_param->nbr_apis; api_ix++)
            {
               if (p_iocr_param->apis[api_ix].api == p_exp_api->api)
               {
                  p_iocr_api = &p_iocr_param->apis[api_ix];

                  for (data_ix = 0; data_ix < p_iocr_api->nbr_io_data;
                       data_ix++)
                  {
                     if (
                        (p_iocr_api->io_data[data_ix].slot_number ==
                         p_exp_mod->slot_number) &&
                        (p_iocr_api->io_data[data_ix].subslot_number ==
                         p_exp_sub->subslot_number))
                     {
                        cnt++;
                     }
                  }
                  for (data_ix = 0; data_ix < p_iocr_api->nbr_iocs; data_ix++)
                  {
                     if (
                        (p_iocr_api->iocs[data_ix].slot_number ==
                         p_exp_mod->slot_number) &&
                        (p_iocr_api->iocs[data_ix].subslot_number ==
                         p_exp_sub->subslot_number))
                     {
                        cnt++;
                     }
                  }
               }
            }
         }

         if (cnt == 0)
         {
            pf_set_error (
               p_stat,
               PNET_ERROR_CODE_CONNECT,
               PNET_ERROR_DECODE_PNIO,
               PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ,
               10);
            ret = -1;
         }
      }

      if (ret == 0)
      {
         if (p_exp_sub->subslot_number > 0x9FFF)
         {
            pf_set_error (
               p_stat,
               PNET_ERROR_CODE_CONNECT,
               PNET_ERROR_DECODE_PNIO,
               PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ,
               10);
            ret = -1;
         }
         else if (
            pf_cmdev_get_subslot (
               p_cfg_slot,
               p_exp_sub->subslot_number,
               &p_cfg_sub) != 0)
         {
            /* Not supported in GSDML/Application */
            pf_set_error (
               p_stat,
               PNET_ERROR_CODE_CONNECT,
               PNET_ERROR_DECODE_PNIO,
               PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ,
               10);
            ret = -1;
         }
         else if (
            (p_exp_sub->subslot_number >= PNET_SUBSLOT_DAP_INTERFACE_1_IDENT) &&
            (p_exp_api->api != 0))
         {
            pf_set_error (
               p_stat,
               PNET_ERROR_CODE_CONNECT,
               PNET_ERROR_DECODE_PNIO,
               PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ,
               10);
            ret = -1;
         }
         else if (
            (((p_exp_sub->submodule_properties.type == PNET_DIR_NO_IO) ||
              (p_exp_sub->submodule_properties.type == PNET_DIR_INPUT)) &&
             !((p_exp_sub->nbr_data_descriptors == 1) &&
               (p_exp_sub->data_descriptor[0].data_direction ==
                PF_DIRECTION_INPUT))) || /* Must have input descriptor. */
            ((p_exp_sub->submodule_properties.type == PNET_DIR_OUTPUT) &&
             !((p_exp_sub->nbr_data_descriptors == 1) &&
               (p_exp_sub->data_descriptor[0].data_direction ==
                PF_DIRECTION_OUTPUT))) || /* Must have output descriptor. */
            ((p_exp_sub->submodule_properties.type == PNET_DIR_IO) &&
             !((p_exp_sub->nbr_data_descriptors == 2) && /* Must have both input
                                                            and output
                                                            descriptors. */
               (((p_exp_sub->data_descriptor[0].data_direction ==
                  PF_DIRECTION_INPUT) ||
                 (p_exp_sub->data_descriptor[1].data_direction ==
                  PF_DIRECTION_INPUT)) &&
                ((p_exp_sub->data_descriptor[0].data_direction ==
                  PF_DIRECTION_OUTPUT) ||
                 (p_exp_sub->data_descriptor[1].data_direction ==
                  PF_DIRECTION_OUTPUT))))))
         {
            pf_set_error (
               p_stat,
               PNET_ERROR_CODE_CONNECT,
               PNET_ERROR_DECODE_PNIO,
               PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ,
               12);
            ret = -1;
         }
         else if (
            (p_exp_sub->submodule_properties.type == PNET_DIR_OUTPUT) &&
            (p_exp_sub->submodule_properties.sharedInput == true))
         {
            pf_set_error (
               p_stat,
               PNET_ERROR_CODE_CONNECT,
               PNET_ERROR_DECODE_PNIO,
               PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ,
               12);
            ret = -1;
         }
         else if (
            (p_exp_sub->submodule_properties
                .reduce_input_submodule_data_length == true) &&
            ((p_exp_sub->submodule_properties.type == PNET_DIR_INPUT) ||
             (p_exp_sub->submodule_properties.type == PNET_DIR_IO)))
         {
            pf_set_error (
               p_stat,
               PNET_ERROR_CODE_CONNECT,
               PNET_ERROR_DECODE_PNIO,
               PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ,
               12);
            ret = -1;
         }
         else if (
            (p_exp_sub->submodule_properties
                .reduce_output_submodule_data_length == true) &&
            ((p_exp_sub->submodule_properties.type == PNET_DIR_OUTPUT) ||
             (p_exp_sub->submodule_properties.type == PNET_DIR_IO)))
         {
            pf_set_error (
               p_stat,
               PNET_ERROR_CODE_CONNECT,
               PNET_ERROR_DECODE_PNIO,
               PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ,
               12);
            ret = -1;
         }
         else if (
            (p_exp_sub->submodule_properties.discard_ioxs == true) &&
            (p_exp_sub->submodule_properties.type != PNET_DIR_IO))
         {
            pf_set_error (
               p_stat,
               PNET_ERROR_CODE_CONNECT,
               PNET_ERROR_DECODE_PNIO,
               PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ,
               12);
            ret = -1;
         }
         /* Reserved member not checked. */
         else
         {
            p_cfg_sub->exp_submodule_ident_number =
               p_exp_sub->submodule_ident_number;

            /*
             * During device init the stack plugs its own DAP.
             * At that time there is no AR.
             * When the DAP is plugged by the application there is an AR (from
             * the connect). Update the AR reference if not already set - this
             * is important in case the application wants to set IOCS of a DAP
             * sub-module (which it should).
             */
            if (p_cfg_sub->p_ar == NULL)
            {
               /* Inherit AR from slot */
               p_cfg_sub->p_ar = p_cfg_slot->p_ar;
            }

            for (data_ix = 0; data_ix < p_exp_sub->nbr_data_descriptors;
                 data_ix++)
            {
               if (
                  (p_exp_sub->data_descriptor[data_ix].data_direction !=
                   PF_DIRECTION_INPUT) &&
                  (p_exp_sub->data_descriptor[data_ix].data_direction !=
                   PF_DIRECTION_OUTPUT))
               {
                  pf_set_error (
                     p_stat,
                     PNET_ERROR_CODE_CONNECT,
                     PNET_ERROR_DECODE_PNIO,
                     PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ,
                     13);
                  ret = -1;
               }
               else if (
                  (p_exp_sub->data_descriptor[data_ix].data_direction ==
                   PF_DIRECTION_INPUT) &&
                  (p_exp_sub->submodule_properties.type == PNET_DIR_OUTPUT))
               {
                  pf_set_error (
                     p_stat,
                     PNET_ERROR_CODE_CONNECT,
                     PNET_ERROR_DECODE_PNIO,
                     PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ,
                     13);
                  ret = -1;
               }
               else if (
                  (p_exp_sub->data_descriptor[data_ix].data_direction ==
                   PF_DIRECTION_OUTPUT) &&
                  ((p_exp_sub->submodule_properties.type != PNET_DIR_OUTPUT) &&
                   (p_exp_sub->submodule_properties.type != PNET_DIR_IO)))
               {
                  pf_set_error (
                     p_stat,
                     PNET_ERROR_CODE_CONNECT,
                     PNET_ERROR_DECODE_PNIO,
                     PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ,
                     13);
                  ret = -1;
               }
               /* Reserved member not checked. */
               else if (
                  (p_exp_sub->data_descriptor[data_ix].submodule_data_length >
                   1439) ||
                  ((p_exp_sub->submodule_properties.type == PNET_DIR_NO_IO) &&
                   (p_exp_sub->data_descriptor[data_ix].submodule_data_length !=
                    0)))
               {
                  pf_set_error (
                     p_stat,
                     PNET_ERROR_CODE_CONNECT,
                     PNET_ERROR_DECODE_PNIO,
                     PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ,
                     14);
                  ret = -1;
               }
               else if (p_exp_sub->data_descriptor[data_ix].length_iops != 1)
               {
                  pf_set_error (
                     p_stat,
                     PNET_ERROR_CODE_CONNECT,
                     PNET_ERROR_DECODE_PNIO,
                     PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ,
                     15);
                  ret = -1;
               }
               else if (p_exp_sub->data_descriptor[data_ix].length_iocs != 1)
               {
                  pf_set_error (
                     p_stat,
                     PNET_ERROR_CODE_CONNECT,
                     PNET_ERROR_DECODE_PNIO,
                     PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ,
                     16);
                  ret = -1;
               }
            }
         }
      }
   }

   return ret;
}

/**
 * @internal
 * Configure an expected module.
 *
 * This function configures an expected module.
 * Perform final checks of parameters and let the application plug the module if
 * needed.
 *
 * Triggers the user call-back \a pnet_exp_module_ind() and \a
 * pnet_exp_submodule_ind().
 *
 * @param net              InOut: The p-net stack instance
 * @param p_exp_api        In:    The expected API instance.
 * @param p_cfg_api        InOut: The configured API instance.
 * @param p_stat           Out:   Detailed error information if return != 0.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmdev_exp_modules_configure (
   pnet_t * net,
   const pf_exp_api_t * p_exp_api,
   pf_api_t * p_cfg_api,
   pnet_result_t * p_stat)
{
   int ret = -1;
   uint16_t mod_ix;
   const pf_exp_module_t * p_exp_mod = NULL;
   pf_slot_t * p_cfg_slot = NULL;
   uint16_t slot;
   uint16_t cnt;
   uint16_t ix;

   ret = 0; /* Assume all goes well */
   for (mod_ix = 0; mod_ix < p_exp_api->nbr_modules; mod_ix++)
   {
      if (ret == 0)
      {
         p_exp_mod = &p_exp_api->modules[mod_ix];

         /* Within each API a slot_number may only be specified once. */
         slot = p_exp_mod->slot_number;
         cnt = 0;
         for (ix = 0; ix < mod_ix; ix++)
         {
            if (slot == p_exp_api->modules[ix].slot_number)
            {
               cnt++;
            }
         }
         if (cnt != 0)
         {
            pf_set_error (
               p_stat,
               PNET_ERROR_CODE_CONNECT,
               PNET_ERROR_DECODE_PNIO,
               PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ,
               6);
            ret = -1;
         }

         /* If the module is not yet plugged then let the application plug it
          * now! */
         if (pf_cmdev_get_slot (p_cfg_api, p_exp_mod->slot_number, &p_cfg_slot) != 0)
         {
            /*
             * Return code is not interesting here.
             */
            (void)pf_fspm_exp_module_ind (
               net,
               p_exp_api->api,
               p_exp_mod->slot_number,
               p_exp_mod->module_ident_number);
         }
         else
         {
            if (p_exp_mod->module_ident_number != p_cfg_slot->module_ident_number)
            {
               p_cfg_slot->plug_state = PF_MOD_PLUG_SUBSTITUTE;
            }
            else
            {
               p_cfg_slot->plug_state = PF_MOD_PLUG_PROPER_MODULE;
            }
         }
      }

      if (ret == 0)
      {
         if (p_exp_mod->slot_number > 0x7FFF)
         {
            pf_set_error (
               p_stat,
               PNET_ERROR_CODE_CONNECT,
               PNET_ERROR_DECODE_PNIO,
               PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ,
               6);
            ret = -1;
         }
         /* Defer slot_number not unique test to later. */
         else if (
            pf_cmdev_get_slot (p_cfg_api, p_exp_mod->slot_number, &p_cfg_slot) !=
            0)
         {
            /* Not supported in GSDML/Application */
            pf_set_error (
               p_stat,
               PNET_ERROR_CODE_CONNECT,
               PNET_ERROR_DECODE_PNIO,
               PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ,
               6);
            ret = -1;
         }
         else if (p_exp_mod->module_ident_number == 0)
         {
            pf_set_error (
               p_stat,
               PNET_ERROR_CODE_CONNECT,
               PNET_ERROR_DECODE_PNIO,
               PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ,
               7);
            ret = -1;
         }
         /* No check of module properties. */
         else if (p_exp_mod->nbr_submodules == 0)
         {
            pf_set_error (
               p_stat,
               PNET_ERROR_CODE_CONNECT,
               PNET_ERROR_DECODE_PNIO,
               PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ,
               9);
            ret = -1;
         }
         else
         {
            /*
             * During device init the stack plugs its own DAP.
             * At that time there is no AR.
             * When the DAP is plugged by the application there is an AR (from
             * the connect). Update the AR reference if not already set. This is
             * important in case the application wants (it should!) to set IOCS
             * of a DAP sub-module.
             */
            if (p_cfg_slot->p_ar == NULL)
            {
               /* Inherit AR from API */
               p_cfg_slot->p_ar = p_cfg_api->p_ar;
            }

            p_cfg_slot->exp_module_ident_number =
               p_exp_mod->module_ident_number;
            ret = pf_cmdev_exp_submodule_configure (
               net,
               p_exp_api,
               p_exp_mod,
               p_cfg_api,
               p_cfg_slot,
               p_stat);
         }
      }
   }

   return ret;
}

/**
 * @internal
 * Configure an expected API.
 *
 * This function configures an expected API.
 * Perform final checks of parameters and let the application plug the
 * (sub-)modules when needed.
 *
 * Triggers the user call-back \a pnet_exp_module_ind() and \a
 * pnet_exp_submodule_ind().
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param p_stat           Out:   Detailed error information if return != 0.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmdev_exp_apis_configure (
   pnet_t * net,
   pf_ar_t * p_ar,
   pnet_result_t * p_stat)
{
   int ret = -1;
   pf_exp_api_t * p_exp_api = NULL;
   pf_api_t * p_cfg_api = NULL;
   uint16_t api_ix;

   /* Error discovery for the whole block */
   if (
      (p_ar->ar_param.ar_properties.device_access == false) &&
      (p_ar->nbr_exp_apis == 0))
   {
      pf_set_error (
         p_stat,
         PNET_ERROR_CODE_CONNECT,
         PNET_ERROR_DECODE_PNIO,
         PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ,
         4);
   }
   else
   {
      ret = 0; /* Assume all goes well */
      for (api_ix = 0; api_ix < p_ar->nbr_exp_apis; api_ix++)
      {
         if (ret == 0)
         {
            p_exp_api = &p_ar->exp_apis[api_ix];
            /* If it does not exist then create it */
            if (
               (pf_cmdev_get_api (net, p_exp_api->api, &p_cfg_api) == 0) ||
               (pf_cmdev_new_api (net, p_exp_api->api, &p_cfg_api) == 0))
            {
               p_cfg_api->p_ar = p_ar;

               ret = pf_cmdev_exp_modules_configure (
                  net,
                  p_exp_api,
                  p_cfg_api,
                  p_stat);
               p_exp_api->valid = (ret == 0);
            }
         }
      }
   }

   return ret;
}

/**
 * @internal
 * Check the alarm CR block for errors.
 * @param p_ar             InOut: The AR instance.
 * @param p_stat           Out:   Detailed error information if return != 0.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmdev_check_alarm_cr (pf_ar_t * p_ar, pnet_result_t * p_stat)
{
   int ret = 0;

   if (p_ar->ar_param.ar_properties.device_access == true)
   {
      pf_set_error (
         p_stat,
         PNET_ERROR_CODE_CONNECT,
         PNET_ERROR_DECODE_PNIO,
         PNET_ERROR_CODE_1_CMRPC,
         PNET_ERROR_CODE_2_CMRPC_UNKNOWN_BLOCKS);
      ret = -1;
   }
   else if (p_ar->alarm_cr_request.alarm_cr_type != PF_ALARM_CR)
   {
      pf_set_error (
         p_stat,
         PNET_ERROR_CODE_CONNECT,
         PNET_ERROR_DECODE_PNIO,
         PNET_ERROR_CODE_1_CONN_FAULTY_ALARM_BLOCK_REQ,
         4);
      ret = -1;
   }
   else if (
      ((p_ar->alarm_cr_request.alarm_cr_properties.transport_udp == false) &&
       (p_ar->alarm_cr_request.lt_field != PNAL_ETHTYPE_PROFINET)) ||
      ((p_ar->alarm_cr_request.alarm_cr_properties.transport_udp == true) &&
       (p_ar->alarm_cr_request.lt_field != PNAL_ETHTYPE_IP)))
   {
      pf_set_error (
         p_stat,
         PNET_ERROR_CODE_CONNECT,
         PNET_ERROR_DECODE_PNIO,
         PNET_ERROR_CODE_1_CONN_FAULTY_ALARM_BLOCK_REQ,
         5);
      ret = -1;
   }
   /* alarm_cr_properties.reserved not checked */
   else if (
      (p_ar->alarm_cr_request.rta_timeout_factor < 1) ||
      (p_ar->alarm_cr_request.rta_timeout_factor > 0x0064))
   {
      pf_set_error (
         p_stat,
         PNET_ERROR_CODE_CONNECT,
         PNET_ERROR_DECODE_PNIO,
         PNET_ERROR_CODE_1_CONN_FAULTY_ALARM_BLOCK_REQ,
         7);
      ret = -1;
   }
   else if (
      (p_ar->alarm_cr_request.rta_retries < 3) ||
      (p_ar->alarm_cr_request.rta_retries > 15))
   {
      pf_set_error (
         p_stat,
         PNET_ERROR_CODE_CONNECT,
         PNET_ERROR_DECODE_PNIO,
         PNET_ERROR_CODE_1_CONN_FAULTY_ALARM_BLOCK_REQ,
         8);
      ret = -1;
   }
   else if (
      (p_ar->alarm_cr_request.max_alarm_data_length < 200) ||
      (p_ar->alarm_cr_request.max_alarm_data_length > 1432))
   {
      pf_set_error (
         p_stat,
         PNET_ERROR_CODE_CONNECT,
         PNET_ERROR_DECODE_PNIO,
         PNET_ERROR_CODE_1_CONN_FAULTY_ALARM_BLOCK_REQ,
         10);
      ret = -1;
   }
   else if (p_ar->alarm_cr_request.alarm_cr_tag_header_high.vlan_id != 0) /* No
                                                                             VLAN
                                                                           */
   {
      pf_set_error (
         p_stat,
         PNET_ERROR_CODE_CONNECT,
         PNET_ERROR_DECODE_PNIO,
         PNET_ERROR_CODE_1_CONN_FAULTY_ALARM_BLOCK_REQ,
         11);
      ret = -1;
   }
   else if (
      p_ar->alarm_cr_request.alarm_cr_tag_header_high.alarm_user_priority !=
      6) /* Alarm CR priority high */
   {
      pf_set_error (
         p_stat,
         PNET_ERROR_CODE_CONNECT,
         PNET_ERROR_DECODE_PNIO,
         PNET_ERROR_CODE_1_CONN_FAULTY_ALARM_BLOCK_REQ,
         11);
      ret = -1;
   }
   else if (p_ar->alarm_cr_request.alarm_cr_tag_header_low.vlan_id != 0) /* No
                                                                            VLAN
                                                                          */
   {
      pf_set_error (
         p_stat,
         PNET_ERROR_CODE_CONNECT,
         PNET_ERROR_DECODE_PNIO,
         PNET_ERROR_CODE_1_CONN_FAULTY_ALARM_BLOCK_REQ,
         12);
      ret = -1;
   }
   else if (
      p_ar->alarm_cr_request.alarm_cr_tag_header_low.alarm_user_priority !=
      5) /* Alarm CR priority low */
   {
      pf_set_error (
         p_stat,
         PNET_ERROR_CODE_CONNECT,
         PNET_ERROR_DECODE_PNIO,
         PNET_ERROR_CODE_1_CONN_FAULTY_ALARM_BLOCK_REQ,
         12);
      ret = -1;
   }
   else
   {
      p_ar->alarm_cr_request.valid = true;
   }

   return ret;
}

/**
 * @internal
 * Check the AR RPC block for errors.
 *
 * Validates that the UDP port is in correct range.
 *
 * @param p_ar             InOut: The AR instance.
 * @param p_stat           Out:   Detailed error information if return != 0.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmdev_check_ar_rpc (pf_ar_t * p_ar, pnet_result_t * p_stat)
{
   int ret = 0; /* OK until we discover an error. */

   /* Error discovery */
   if (p_ar->ar_rpc_request.initiator_rpc_server_port < 0x0400)
   {
      pf_set_error (
         p_stat,
         PNET_ERROR_CODE_CONNECT,
         PNET_ERROR_DECODE_PNIO,
         PNET_ERROR_CODE_1_CONN_FAULTY_AR_RPC_BLOCK_REQ,
         4);
      ret = -1;
   }
   else
   {
      p_ar->ar_rpc_request.valid = true;
   }

   return ret;
}

/**
 * @internal
 * Check the AR for errors.
 *
 * This function implements the APDUCheck function in the Profinet spec.
 *
 * Triggers the user call-back \a pnet_exp_module_ind() and \a
 * pnet_exp_submodule_ind().
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param p_stat           Out:   Detailed error information if return != 0;
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmdev_check_apdu (
   pnet_t * net,
   pf_ar_t * p_ar,
   pnet_result_t * p_stat)
{
   int ret = -1;
   uint16_t ix;

   if (p_ar != NULL)
   {
      if (p_ar->nbr_ar_param > 0)
      {
         ret = pf_cmdev_check_ar_param (p_ar, p_stat);
      }

      if (ret == 0)
      {
         ret = pf_cmdev_check_iocr_param (net, p_ar, p_stat);
      }

      if (ret == 0)
      {
         /* Let the application plug expected configuration */
         ret = pf_cmdev_exp_apis_configure (net, p_ar, p_stat);
      }

      if (ret == 0)
      {
         /* Build internal data structure for each IOCR */
         for (ix = 0; ix < p_ar->nbr_iocrs; ix++)
         {
            if ((ret == 0) && (pf_cmdev_iocr_setup_desc (p_ar, ix, p_stat) != 0))
            {
               ret = -1;
            }
         }
      }

      if (ret == 0)
      {
         /* Perform the final checks of the IOCRs */
         ret = pf_cmdev_check_iocr_apis (p_ar, p_stat);
      }

      if (ret == 0)
      {
         if (
            (p_ar->ar_param.ar_properties.device_access == false) &&
            ((p_ar->input_cr_cnt == 0) || (p_ar->output_cr_cnt == 0)))
         {
            pf_set_error (
               p_stat,
               PNET_ERROR_CODE_CONNECT,
               PNET_ERROR_DECODE_PNIO,
               PNET_ERROR_CODE_1_CMRPC,
               PNET_ERROR_CODE_2_CMRPC_IOCR_MISSING);
            ret = -1;
         }
         else if (
            (p_ar->ar_param.ar_properties.device_access == false) &&
            (p_ar->nbr_alarm_cr != 1))
         {
            pf_set_error (
               p_stat,
               PNET_ERROR_CODE_CONNECT,
               PNET_ERROR_DECODE_PNIO,
               PNET_ERROR_CODE_1_CMRPC,
               PNET_ERROR_CODE_2_CMRPC_WRONG_BLOCK_COUNT);
            ret = -1;
         }
#if PNET_OPTION_MC_CR
         else if ((p_ar->nbr_mcr == 0) && (p_ar->mcr_cons_cnt > 0))
         {
            /* ToDo: Which error_code_2? */
            pf_set_error (
               p_stat,
               PNET_ERROR_CODE_CONNECT,
               PNET_ERROR_DECODE_PNIO,
               PNET_ERROR_CODE_1_CMRPC,
               PNET_ERROR_CODE_2_CMRPC_WRONG_BLOCK_COUNT);
            ret = -1;
         }
#endif
         else if ((p_ar->ir_info.valid == false) && (p_ar->rtc3_present == true))
         {
            /* ToDo: Which error_code_2? */
            pf_set_error (
               p_stat,
               PNET_ERROR_CODE_CONNECT,
               PNET_ERROR_DECODE_PNIO,
               PNET_ERROR_CODE_1_CMRPC,
               PNET_ERROR_CODE_2_CMRPC_WRONG_BLOCK_COUNT);
            ret = -1;
         }
      }

      if ((ret == 0) && (p_ar->nbr_alarm_cr > 0))
      {
         ret = pf_cmdev_check_alarm_cr (p_ar, p_stat);
      }

      if ((ret == 0) && (p_ar->nbr_rpc_server > 0))
      {
         ret = pf_cmdev_check_ar_rpc (p_ar, p_stat);
      }
   }

   return ret;
}

int pf_cmdev_generate_submodule_diff (pnet_t * net, pf_ar_t * p_ar)
{
   int ret = 0;
   uint16_t exp_api_ix;
   uint16_t exp_mod_ix;
   uint16_t exp_sub_ix;
   uint16_t slot_nbr;
   uint16_t subslot_nbr;
   pf_api_t * p_cfg_api = NULL;
   pf_slot_t * p_cfg_slot = NULL;
   pf_subslot_t * p_cfg_subslot = NULL;
   uint16_t nbr_api_diffs = 0;
   uint16_t nbr_mod_diffs = 0;
   uint16_t nbr_sub_diffs = 0;
   bool has_api_diff = false;
   bool has_mod_diff = false;
   bool has_sub_diff = false;
   pf_submodule_state_t * p_submodule_state;

   if (p_ar == NULL)
   {
      return -1;
   }

   /* Generate a diff including all expected (sub)modules */
   for (exp_api_ix = 0; exp_api_ix < p_ar->nbr_exp_apis; exp_api_ix++)
   {
      p_ar->api_diffs[nbr_api_diffs].api = p_ar->exp_apis[exp_api_ix].api;

      if (pf_cmdev_get_api (net, p_ar->exp_apis[exp_api_ix].api, &p_cfg_api) != 0)
      {
         /* API not found in CFG */
         has_api_diff = true;
      }
      else
      {
         for (exp_mod_ix = 0;
              exp_mod_ix < p_ar->exp_apis[exp_api_ix].nbr_modules;
              exp_mod_ix++)
         {
            slot_nbr =
               p_ar->exp_apis[exp_api_ix].modules[exp_mod_ix].slot_number;

            /* In case an error is found */
            p_ar->api_diffs[nbr_api_diffs]
               .module_diffs[nbr_mod_diffs]
               .slot_number = slot_nbr;
            p_ar->api_diffs[nbr_api_diffs]
               .module_diffs[nbr_mod_diffs]
               .submodule_diffs[nbr_sub_diffs]
               .submodule_state.format_indicator = true;

            if (pf_cmdev_get_slot (p_cfg_api, slot_nbr, &p_cfg_slot) != 0)
            {
               /* slot_number not found in CFG for specified API */
               p_ar->api_diffs[nbr_api_diffs]
                  .module_diffs[nbr_mod_diffs]
                  .module_state = PF_MOD_PLUG_NO_MODULE;
               has_mod_diff = true;
            }
            else if (
               p_ar->exp_apis[exp_api_ix]
                  .modules[exp_mod_ix]
                  .module_ident_number != p_cfg_slot->module_ident_number)
            {
               /* module ident number does not match CFG */
               p_ar->api_diffs[nbr_api_diffs]
                  .module_diffs[nbr_mod_diffs]
                  .module_state = PF_MOD_PLUG_WRONG_MODULE;
               p_ar->api_diffs[nbr_api_diffs]
                  .module_diffs[nbr_mod_diffs]
                  .module_ident_number = p_cfg_slot->module_ident_number;
               has_mod_diff = true;
            }
            else
            {
               /* In case an error is found */
               p_ar->api_diffs[nbr_api_diffs]
                  .module_diffs[nbr_mod_diffs]
                  .module_ident_number = p_cfg_slot->module_ident_number;
               p_ar->api_diffs[nbr_api_diffs]
                  .module_diffs[nbr_mod_diffs]
                  .module_state = PF_MOD_PLUG_PROPER_MODULE;

               for (exp_sub_ix = 0; exp_sub_ix < p_ar->exp_apis[exp_api_ix]
                                                    .modules[exp_mod_ix]
                                                    .nbr_submodules;
                    exp_sub_ix++)
               {
                  subslot_nbr = p_ar->exp_apis[exp_api_ix]
                                   .modules[exp_mod_ix]
                                   .submodules[exp_sub_ix]
                                   .subslot_number;

                  p_ar->api_diffs[nbr_api_diffs]
                     .module_diffs[nbr_mod_diffs]
                     .submodule_diffs[nbr_sub_diffs]
                     .subslot_number = subslot_nbr;
                  p_ar->api_diffs[nbr_api_diffs]
                     .module_diffs[nbr_mod_diffs]
                     .submodule_diffs[nbr_sub_diffs]
                     .submodule_state.format_indicator = true;

                  if (
                     pf_cmdev_get_subslot (
                        p_cfg_slot,
                        subslot_nbr,
                        &p_cfg_subslot) != 0)
                  {
                     /* subslot_number not found in CFG for specified API and
                      * slot number */
                     p_ar->api_diffs[nbr_api_diffs]
                        .module_diffs[nbr_mod_diffs]
                        .submodule_diffs[nbr_sub_diffs]
                        .submodule_state.ident_info = PF_SUBMOD_PLUG_NO;
                     has_sub_diff = true;
                  }
                  else
                  {
                     /* slot_number/subslot_number configured for this API */
                     if (
                        p_ar->exp_apis[exp_api_ix]
                           .modules[exp_mod_ix]
                           .submodules[exp_sub_ix]
                           .submodule_ident_number !=
                        p_cfg_subslot->submodule_ident_number)
                     {
                        /* submodule ident number does not match CFG */
                        p_ar->api_diffs[nbr_api_diffs]
                           .module_diffs[nbr_mod_diffs]
                           .submodule_diffs[nbr_sub_diffs]
                           .submodule_ident_number =
                           p_cfg_subslot->submodule_ident_number;
                        p_ar->api_diffs[nbr_api_diffs]
                           .module_diffs[nbr_mod_diffs]
                           .submodule_diffs[nbr_sub_diffs]
                           .submodule_state.ident_info = PF_SUBMOD_PLUG_WRONG;
                        has_sub_diff = true;
                     }

                     p_submodule_state = &p_ar->api_diffs[nbr_api_diffs]
                                             .module_diffs[nbr_mod_diffs]
                                             .submodule_diffs[nbr_sub_diffs]
                                             .submodule_state;

                     /* Check submodule diagnosis state and update diff. */
                     if (
                        (p_cfg_subslot->submodule_state.fault == true) ||
                        (p_cfg_subslot->submodule_state.maintenance_demanded ==
                         true) ||
                        (p_cfg_subslot->submodule_state.maintenance_required ==
                         true))
                     {
                        p_ar->api_diffs[nbr_api_diffs]
                           .module_diffs[nbr_mod_diffs]
                           .submodule_diffs[nbr_sub_diffs]
                           .submodule_ident_number =
                           p_cfg_subslot->submodule_ident_number;

                        p_submodule_state->fault =
                           p_cfg_subslot->submodule_state.fault;
                        p_submodule_state->maintenance_demanded =
                           p_cfg_subslot->submodule_state.maintenance_demanded;
                        p_submodule_state->maintenance_required =
                           p_cfg_subslot->submodule_state.maintenance_required;
                        has_sub_diff = true;
                     }

                     /* Check if other AR owns the submodule and update diff */
                     if (p_cfg_subslot->p_ar != NULL && p_cfg_subslot->p_ar != p_ar)
                     {
                        p_ar->api_diffs[nbr_api_diffs]
                           .module_diffs[nbr_mod_diffs]
                           .submodule_diffs[nbr_sub_diffs]
                           .submodule_ident_number =
                           p_cfg_subslot->submodule_ident_number;

                        p_submodule_state->ar_info =
                           PF_SUBMOD_AR_INFO_LOCKED_BY_IO_CONTROLLER;
                        has_sub_diff = true;
                     }
                  }

                  if (has_sub_diff == true)
                  {
                     nbr_sub_diffs++;
                     has_sub_diff = false;

                     has_mod_diff = true;
                  }
               }
               p_ar->api_diffs[nbr_api_diffs]
                  .module_diffs[nbr_mod_diffs]
                  .nbr_submodule_diffs = nbr_sub_diffs;
               nbr_sub_diffs = 0;
            }
            if (has_mod_diff == true)
            {
               nbr_mod_diffs++;
               has_mod_diff = false;

               has_api_diff = true;
            }
         }
         p_ar->api_diffs[nbr_api_diffs].nbr_module_diffs = nbr_mod_diffs;
         nbr_mod_diffs = 0;
      }
      if (has_api_diff == true)
      {
         nbr_api_diffs++;
         has_api_diff = false;
      }
   }
   p_ar->nbr_api_diffs = nbr_api_diffs;

   return ret;
}

/**
 * @internal
 * Verify that the frame id is not already used by any AR.
 *
 * Looks in all ARs, both in input CR and output CR.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:    The AR instance.
 * @param frame_id         In:    The frame id to check.
 * @return  true  if the frame_id is free to use.
 *          false if the frame id is already used.
 */
static bool pf_cmdev_verify_free_frame_id (pnet_t * net, uint16_t frame_id)
{
   int ix = 0;
   int iy = 0;
   pf_ar_t * p_ar = NULL;

   for (ix = 0; ix < PNET_MAX_AR; ix++)
   {
      p_ar = pf_ar_find_by_index (net, ix);
      if ((p_ar != NULL) && (p_ar->in_use == true))
      {
         for (iy = 0; iy < p_ar->nbr_iocrs; iy++)
         {
            if (p_ar->iocrs[iy].param.frame_id == frame_id)
            {
               return false;
            }
         }
      }
   }

   return true;
}

/**
 * @internal
 * The controller may send 0xffff as the frame id for output CRs.
 * In that case we must supply a preferred frame id in the response.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 */
static void pf_cmdev_fix_frame_id (pnet_t * net, pf_ar_t * p_ar)
{
   uint16_t ix;
   pf_iocr_param_t * p_iocr_param;
   uint16_t start;
   uint16_t stop;
   uint16_t frame_id;

   for (ix = 0; ix < p_ar->nbr_iocrs; ix++)
   {
      p_iocr_param = &p_ar->iocrs[ix].param;
      if (
         (p_iocr_param->iocr_type == PF_IOCR_TYPE_OUTPUT) &&
         (p_iocr_param->frame_id == 0xFFFF))
      {
         switch (p_iocr_param->iocr_properties.rt_class)
         {
         case PF_RT_CLASS_1:
            start = 0xC000;
            stop = 0xF7FF;
            break;
         case PF_RT_CLASS_2:
            start = 0x8000;
            stop = 0xBBFF;
            break;
         case PF_RT_CLASS_3:
            start = 0x0100;
            stop = 0x7FFF;
            break;
         case PF_RT_CLASS_UDP:
            start = 0xC000;
            stop = 0xF7FF;
            break;
         default:
            start = 0xFFFF;
            stop = 0xFFFF;
            LOG_ERROR (PNET_LOG, "CMDEV(%d): Invalid rt_class\n", __LINE__);
            break;
         }

         frame_id = start;
         while ((frame_id <= stop) &&
                (pf_cmdev_verify_free_frame_id (net, frame_id) == false))
         {
            frame_id++;
         }

         if (frame_id <= stop)
         {
            p_iocr_param->frame_id = frame_id;
            LOG_DEBUG (
               PNET_LOG,
               "CMDEV(%d): Using FrameID 0x%04x for output CR with AREP %u "
               "CREP %" PRIu32 "\n",
               __LINE__,
               frame_id,
               p_ar->arep,
               p_ar->iocrs[ix].crep);
         }
         else
         {
            LOG_ERROR (
               PNET_LOG,
               "CMDEV(%d): No free frame_id found\n",
               __LINE__);
         }
      }
   }
}

/**
 * @internal
 * Handle a negative result to a connect request.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param p_stat           Out:   Detailed error information.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmdev_cm_connect_rsp_neg (
   pnet_t * net,
   pf_ar_t * p_ar,
   pnet_result_t * p_stat)
{
   if (p_ar->ar_param.ar_properties.device_access == false)
   {
      pnet_create_log_book_entry (
         net,
         p_ar->arep,
         &p_stat->pnio_status,
         (p_stat->add_data_1 << 16) + (p_stat->add_data_2));
      LOG_ERROR (PNET_LOG, "CMDEV(%d): Connect failed\n", __LINE__);
   }

   pf_cmdev_set_state (net, p_ar, PF_CMDEV_STATE_W_CIND);

   return 0;
}

/**
 * @internal
 * Handle a positive answer to a connect request.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param p_stat           Out:   Detailed error info if returning != 0
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmdev_cm_connect_rsp_pos (
   pnet_t * net,
   pf_ar_t * p_ar,
   pnet_result_t * p_stat)
{
   int ret = -1;

   if (p_ar->ar_param.ar_properties.device_access == false)
   {
      if (p_ar->cmdev_state == PF_CMDEV_STATE_W_CRES)
      {
         ret = pf_cmsu_start_req (net, p_ar, p_stat); /* Start all required
                                                         protocol machines. */

         pf_cmdev_set_state (net, p_ar, PF_CMDEV_STATE_W_SUCNF);
         if (ret == 0)
         {
            /* pf_cmdev_cmsu_start_cnf_pos */
            pf_cmdev_state_ind (net, p_ar, PNET_EVENT_STARTUP);
            pf_cmdev_set_state (net, p_ar, PF_CMDEV_STATE_W_PEIND);
            /* pf_cmdev_rm_connect_rsp_pos handled by caller. */
         }
         else
         {
            /* pf_cmdev_cmsu_start_cnf_neg */
            pf_set_error (
               p_stat,
               PNET_ERROR_CODE_CONNECT,
               PNET_ERROR_DECODE_PNIO,
               PNET_ERROR_CODE_1_CMRPC,
               PNET_ERROR_CODE_2_CMRPC_PDEV_ALREADY_OWNED);
            pnet_create_log_book_entry (
               net,
               p_ar->arep,
               &p_stat->pnio_status,
               (p_stat->add_data_1 << 16) + (p_stat->add_data_2));
            /* pf_cmdev_rm_connect_rsp_neg handled by caller. */
         }
      }
   }
   else
   {
      if (p_ar->cmdev_state == PF_CMDEV_STATE_W_CRES)
      {
         pf_cmdev_state_ind (net, p_ar, PNET_EVENT_STARTUP);
         pf_cmdev_set_state (net, p_ar, PF_CMDEV_STATE_DATA);
         /* pf_cmdev_rm_connect_rsp_pos handled by caller. */
         ret = 0;
      }
      else
      {
         pf_cmdev_set_state (net, p_ar, PF_CMDEV_STATE_W_CIND);
         /* pf_cmdev_rm_connect_rsp_neg handled by caller. */
         ret = -1;
      }
   }

   return ret;
}

/**
 * @internal
 * Reset observers / configurable checks
 * - Disable configurable checks / observers
 * - Observers should remove active diagnostics
 *   from diagnostics ASE
 *
 * Todo: Only observers for submodules used within a
 * certain AR should be reset.
 *
 * @param net              InOut: The p-net stack instance
 */
static void pf_cmdev_reset_observers (pnet_t * net)
{
   pf_pdport_reset_all (net);
}

/* ================================================
 *       Remote primitives
 */

int pf_cmdev_rm_connect_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   pnet_result_t * p_connect_result)
{
   int ret = -1;
   uint16_t ix;
   char station_name[PNET_STATION_NAME_MAX_SIZE]; /** Terminated */
   const pnet_ethaddr_t * mac_address = pf_cmina_get_device_macaddr (net);

   /* RM_Connect.ind */
   if (pf_cmdev_check_apdu (net, p_ar, p_connect_result) == 0)
   {
      pf_cmdev_reset_observers (net);
      pf_pdport_ar_connect_ind (net, p_ar);

      if (pf_cmdev_generate_submodule_diff (net, p_ar) == 0)
      {
         /* Start building the response to the connect request. */
         memcpy (
            p_ar->ar_result.cm_responder_mac_add.addr,
            mac_address->addr,
            sizeof (pnet_ethaddr_t));
         p_ar->ar_result.responder_udp_rt_port = PF_UDP_UNICAST_PORT;

         pf_cmdev_fix_frame_id (net, p_ar);

         for (ix = 0; ix < p_ar->nbr_iocrs; ix++)
         {
            p_ar->iocrs[ix].result.iocr_type = p_ar->iocrs[ix].param.iocr_type;
            p_ar->iocrs[ix].result.iocr_reference =
               p_ar->iocrs[ix].param.iocr_reference;
            p_ar->iocrs[ix].result.frame_id = p_ar->iocrs[ix].param.frame_id;
         }
         p_ar->alarm_cr_result.alarm_cr_type =
            p_ar->alarm_cr_request.alarm_cr_type;
         p_ar->alarm_cr_result.remote_alarm_reference =
            p_ar->alarm_cr_request.local_alarm_reference;
         p_ar->alarm_cr_result.max_alarm_data_length = PF_MAX_ALARM_DATA_LEN;

         pf_cmina_get_station_name (net, station_name);
         strncpy (
            p_ar->ar_server.cm_responder_station_name,
            station_name,
            sizeof (p_ar->ar_server.cm_responder_station_name));
         p_ar->ar_server.cm_responder_station_name
            [sizeof (p_ar->ar_server.cm_responder_station_name) - 1] = '\0';
         p_ar->ar_server.length_cm_responder_station_name =
            (uint16_t)strlen (station_name);

         p_ar->ready_4_data = false;

         ret = pf_fspm_cm_connect_ind (net, p_ar, p_connect_result);
      }
   }

   if (ret == 0)
   {
      pf_cmdev_set_state (net, p_ar, PF_CMDEV_STATE_W_CRES);

      ret = pf_cmdev_cm_connect_rsp_pos (net, p_ar, p_connect_result);
   }
   else
   {
      ret = pf_cmdev_cm_connect_rsp_neg (net, p_ar, p_connect_result);
   }

   /* rm_connect.rsp handled by caller */
   return ret;
}

int pf_cmdev_rm_release_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   pnet_result_t * p_release_result)
{
   int ret = -1;

   if (p_ar->ar_param.ar_properties.device_access == false)
   {
      switch (p_ar->cmdev_state)
      {
      case PF_CMDEV_STATE_W_PEIND:
      case PF_CMDEV_STATE_W_ARDY:
      case PF_CMDEV_STATE_W_ARDYCNF:
      case PF_CMDEV_STATE_WDATA:
      case PF_CMDEV_STATE_DATA:
         p_ar->err_cls = PNET_ERROR_CODE_1_RTA_ERR_CLS_PROTOCOL;
         p_ar->err_code = PNET_ERROR_CODE_2_ABORT_AR_RELEASE_IND_RECEIVED;
         if (pf_fspm_cm_release_ind (net, p_ar, p_release_result) != 0)
         {
            p_ar->err_code = p_release_result->pnio_status.error_code_2;
         }
         ret = pf_cmdev_set_state (net, p_ar, PF_CMDEV_STATE_ABORT);
         break;
      default:
         /* Ignore and stay in all other states. */
         ret = 0;
         break;
      }
   }
   else
   {
      if (p_ar->cmdev_state == PF_CMDEV_STATE_DATA)
      {
         p_ar->err_cls = PNET_ERROR_CODE_1_RTA_ERR_CLS_PROTOCOL;
         p_ar->err_code = PNET_ERROR_CODE_2_ABORT_AR_RELEASE_IND_RECEIVED;
         ret = pf_cmdev_state_ind (net, p_ar, PNET_EVENT_ABORT);
      }
      else
      {
         /* Ignore */
         ret = 0;
      }
   }

   return ret;
}

int pf_cmdev_rm_dcontrol_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   pf_control_block_t * p_control_io,
   pnet_result_t * p_dcontrol_result,
   bool * p_set_state_prmend)
{
   int ret = -1;

   if (p_ar->ar_param.ar_properties.device_access == false)
   {
      switch (p_ar->cmdev_state)
      {
      case PF_CMDEV_STATE_W_PEIND:
         if (p_control_io->control_command == BIT (PF_CONTROL_COMMAND_BIT_PRM_END))
         {
            pf_cmdev_set_state (net, p_ar, PF_CMDEV_STATE_W_PERES);
            if (
               pf_fspm_cm_dcontrol_ind (
                  net,
                  p_ar,
                  PNET_CONTROL_COMMAND_PRM_END,
                  p_dcontrol_result) == 0)
            {
               pf_cmdev_set_state (net, p_ar, PF_CMDEV_STATE_W_ARDY);
               if (pf_cmdev_check_pdev (net) == 0)
               {
                  LOG_DEBUG (
                     PNET_LOG,
                     "CMDEV(%d): Prepare to send event PNET_EVENT_PRMEND\n",
                     __LINE__);
                  *p_set_state_prmend = true;
                  /**
                   * The application must issue the applicationReady call
                   * _AFTER_ returning from the pf_fspm_cm_dcontrol_ind_cb()
                   * call-back. It _may_ be issued at the earliest from the
                   * state_cb(PNET_EVENT_PRMEND) call-back.
                   */
                  ret = 0;
               }
               else
               {
                  LOG_ERROR (
                     PNET_LOG,
                     "CMDEV(%d): There is no local Ethernet port with high "
                     "enough speed.\n",
                     __LINE__);
                  p_ar->err_cls = PNET_ERROR_CODE_1_RTA_ERR_CLS_PROTOCOL;
                  p_ar->err_code = PNET_ERROR_CODE_2_ABORT_PDEV_CHECK_FAILED;
               }
            }
         }
         break;
      default:
         /* Ignore and stay in all other states. */
         ret = 0;
         break;
      }
   }

   if (ret != 0)
   {
      ret = pf_cmdev_state_ind (net, p_ar, PNET_EVENT_ABORT);
   }

   return ret;
}

int pf_cmdev_rm_ccontrol_cnf (
   pnet_t * net,
   pf_ar_t * p_ar,
   pf_control_block_t * p_control_io,
   pnet_result_t * p_ccontrol_result)
{
   int ret = -1;
   CC_ASSERT (p_ar != NULL);

   if (p_ar->cmdev_state == PF_CMDEV_STATE_W_ARDYCNF)
   {
      if (
         (p_ccontrol_result->pnio_status.error_code ==
          PNET_ERROR_CODE_NOERROR) &&
         (p_ccontrol_result->pnio_status.error_decode ==
          PNET_ERROR_DECODE_NOERROR) &&
         (p_ccontrol_result->pnio_status.error_code_1 == 0) &&
         (p_ccontrol_result->pnio_status.error_code_2 == 0))
      {
         /*
          * The command_control in the control_block only has the "Done" bit
          * set. Assume that it is the confirmation to the appl_rdy
          * ccontrol_req.
          */
         if (p_control_io->control_command == BIT (PF_CONTROL_COMMAND_BIT_DONE))
         {
            /*
             * Alarm transmitter enabled on APPLRDY CNF.
             * PN-AL-protocol (Mar20) Figure A.7
             */
            pf_alarm_enable (p_ar);

            if (p_ar->ready_4_data == true)
            {
               (void)pf_cmdev_state_ind (net, p_ar, PNET_EVENT_DATA);
               ret = pf_cmdev_set_state (net, p_ar, PF_CMDEV_STATE_DATA);
            }
            else
            {
               ret = pf_cmdev_set_state (net, p_ar, PF_CMDEV_STATE_WDATA);
            }
            /* Send result to application */
            pf_fspm_ccontrol_cnf (net, p_ar, p_ccontrol_result);
         }
         else
         {
            LOG_ERROR (PNET_LOG, "CMDEV(%d): DONE bit not set\n", __LINE__);
         }
      }
      else
      {
         pnet_create_log_book_entry (
            net,
            p_ar->arep,
            &p_ccontrol_result->pnio_status,
            (p_ccontrol_result->add_data_1 << 16) +
               (p_ccontrol_result->add_data_2));
         /* Send result to application */
         pf_fspm_ccontrol_cnf (net, p_ar, p_ccontrol_result);
         (void)pf_cmdev_set_state (net, p_ar, PF_CMDEV_STATE_ABORT);
      }
   }
   else
   {
      LOG_ERROR (
         PNET_LOG,
         "CMDEV(%d): ccontrol cnf in state %s\n",
         __LINE__,
         pf_cmdev_state_to_string (p_ar->cmdev_state));
   }

   return ret;
}

int pf_cmdev_cm_ccontrol_req (pnet_t * net, pf_ar_t * p_ar)
{
   int ret = -1;
   uint16_t ix;
   uint16_t iy;
   bool data_avail = true; /* Assume all OK */
   pf_iodata_object_t * p_iodata = NULL;
   pf_ar_t * p_owning_ar = NULL;
   pf_iocr_t * p_owning_iocr = NULL;
   pf_iodata_object_t * p_owning_iodata = NULL;
   uint32_t crep;

   if (p_ar->ar_param.ar_properties.device_access == false)
   {
      switch (p_ar->cmdev_state)
      {
      case PF_CMDEV_STATE_W_ARDY:
         /* Verify that appl has created input data for all PPM */
         for (ix = 0; ix < p_ar->nbr_iocrs; ix++)
         {
            if (
               (p_ar->iocrs[ix].param.iocr_type == PF_IOCR_TYPE_INPUT) ||
               (p_ar->iocrs[ix].param.iocr_type == PF_IOCR_TYPE_MC_PROVIDER))
            {
               for (iy = 0; iy < p_ar->iocrs[ix].nbr_data_desc; iy++)
               {
                  p_iodata = &p_ar->iocrs[ix].data_desc[iy];

                  /* The application is limited to setting data for a specific
                     subslot. Check that it has tried to set the data.
                     If some other AR owns the subslot, check that data is
                     set for that AR.

                     In the future we might implement reading inputdata from
                     one subslot to several connections (different PLCs).

                     Find owning AR, CR and descriptor */
                  if (
                     pf_ppm_get_ar_iocr_desc (
                        net,
                        p_iodata->api_id,
                        p_iodata->slot_nbr,
                        p_iodata->subslot_nbr,
                        &p_owning_ar,
                        &p_owning_iocr,
                        &p_owning_iodata,
                        &crep) == 0)
                  {
                     if (
                        p_owning_ar != p_ar ||
                        p_owning_iocr != &p_ar->iocrs[ix] ||
                        p_owning_iodata != p_iodata)
                     {
                        LOG_DEBUG (
                           PNET_LOG,
                           "CMDEV(%d): Slot %u subslot 0x%04x is owned by "
                           "AREP %u CREP %" PRIu32 " but AREP %u CREP %" PRIu32
                           " is asking to use it.\n",
                           __LINE__,
                           p_iodata->slot_nbr,
                           p_iodata->subslot_nbr,
                           p_owning_ar->arep,
                           p_owning_iocr->crep,
                           p_ar->arep,
                           p_ar->iocrs[ix].crep);
                     }

                     /* Member data_avail is set directly by the PPM. The value
                        is true also if only the IOPS is set. */
                     if (
                        p_owning_iodata->iops_length > 0 &&
                        p_owning_iodata->data_avail == false)
                     {
                        data_avail = false;
                     }
                  }
                  else
                  {
                     LOG_DEBUG (
                        PNET_LOG,
                        "CMDEV(%d): Could not find owning AR for slot %u "
                        "subslot 0x%04x\n",
                        __LINE__,
                        p_iodata->slot_nbr,
                        p_iodata->subslot_nbr);
                     data_avail = false;
                  }
               }
            }
         }

         if (data_avail == true)
         {
            pf_cmdev_state_ind (net, p_ar, PNET_EVENT_APPLRDY);
            if (pf_cmrpc_rm_ccontrol_req (net, p_ar) == 0)
            {
               ret = pf_cmdev_set_state (net, p_ar, PF_CMDEV_STATE_W_ARDYCNF);
            }
         }

         break;
      default:
         /* Ignore and stay in all other states. */
         ret = 0;
         break;
      }
   }
   else
   {
      (void)pf_cmdev_state_ind (net, p_ar, PNET_EVENT_ABORT);
   }

   return ret;
}
