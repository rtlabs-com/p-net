
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
 * @brief Diagnosis implementation
 *
 * Add, update and remove diagnosis items (and send alarms accordingly).
 *
 * Uses the alarm implementation via the pf_alarm_send_diagnosis() function.
 *
 * The bookkeeping of diag items is done by CMDEV via:
 *  - pf_cmdev_get_device()
 *  - pf_cmdev_get_subslot_full()
 *  - pf_cmdev_new_diag()
 *  - pf_cmdev_get_diag_item()
 *  - pf_cmdev_free_diag()
 *
 * An array of PNET_MAX_DIAG_ITEMS diagnosis items is available for use.
 * In CMDEV, each subslot uses a linked list of diagnosis items, and stores the
 * index to the head of its (possibly empty) list.
 */

#ifdef UNIT_TEST
#define pf_alarm_send_diagnosis mock_pf_alarm_send_diagnosis
#endif

#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <inttypes.h>

#include "pf_includes.h"
#include "pf_block_reader.h"
#include "pf_block_writer.h"

#if PNET_MAX_DIAG_ITEMS > 65534
#error "PNET_MAX_DIAG_ITEMS is too large"
#endif

int pf_diag_init (void)
{
   return 0;
}

int pf_diag_exit (void)
{
   return 0;
}

/**
 * @internal
 * Update the problem indicator in the PPM data status
 * by looking at all diagnosis items of a sub-slot.
 *
 * @param net              InOut: The p-net stack instance.
 * @param p_ar             InOut: The AR instance.
 * @param p_subslot        In:    The sub-slot instance.
 */
static void pf_diag_update_station_problem_indicator (
   pnet_t * net,
   pf_ar_t * p_ar,
   const pf_subslot_t * p_subslot)
{
   bool is_problem = false;
   pf_device_t * p_dev = NULL;
   uint16_t ix;
   pf_diag_item_t * p_item;

   if (pf_cmdev_get_device (net, &p_dev) == 0)
   {
      /* A problem is indicated if at least one FAULT diagnosis exists. */
      ix = p_subslot->diag_list;
      pf_cmdev_get_diag_item (net, ix, &p_item);
      while (p_item != NULL)
      {
         if (
            (p_item->in_use == true) &&
            ((p_item->usi < PF_USI_CHANNEL_DIAGNOSIS) ||
             (PF_DIAG_CH_PROP_MAINT_GET (p_item->fmt.std.ch_properties) ==
              PNET_DIAG_CH_PROP_MAINT_FAULT)))
         {
            is_problem = true;
         }

         ix = p_item->next;
         pf_cmdev_get_diag_item (net, ix, &p_item);
      }
   }

   pf_ppm_set_problem_indicator (net, p_ar, is_problem);
}

/**
 * @internal
 * Update the submodule diff state
 *
 * @param net              InOut: The p-net stack instance.
 * @param p_ar             InOut: The AR instance.
 * @param p_subslot        In:    The sub-slot instance.
 */
static void pf_diag_update_submodule_state (
   pnet_t * net,
   pf_ar_t * p_ar,
   pf_subslot_t * p_subslot)
{
   pf_device_t * p_dev = NULL;
   uint16_t ix;
   pf_diag_item_t * p_item;
   pf_submod_state_t submodule_state;
   pnet_alarm_spec_t alarm_spec = {0};
   uint32_t maint_status = 0;

   memset (&submodule_state, 0, sizeof (submodule_state));

   if (pf_cmdev_get_device (net, &p_dev) == 0)
   {
      ix = p_subslot->diag_list;
      pf_cmdev_get_diag_item (net, ix, &p_item);
      while (p_item != NULL)
      {
         if (p_item->in_use == true)
         {
            pf_alarm_add_diag_item_to_summary (
               p_ar,
               p_subslot,
               p_item,
               &alarm_spec,
               &maint_status);

            if (alarm_spec.submodule_diagnosis == true)
            {
               submodule_state.fault = true;
            }

            if (maint_status & PF_DIAG_BIT_MAINTENANCE_REQUIRED)
            {
               submodule_state.maintenance_required = true;
            }

            if (maint_status & PF_DIAG_BIT_MAINTENANCE_DEMANDED)
            {
               submodule_state.maintenance_demanded = true;
            }
         }

         ix = p_item->next;
         pf_cmdev_get_diag_item (net, ix, &p_item);
      }

      p_subslot->submodule_state.fault = submodule_state.fault;
      p_subslot->submodule_state.maintenance_required =
         submodule_state.maintenance_required;
      p_subslot->submodule_state.maintenance_demanded =
         submodule_state.maintenance_demanded;
   }
}

/**
 * @internal
 * Find and unlink a diag item in the specified sub-slot.
 *
 * If the USI of the item is in the manufacturer-specified range then
 * the USI is used to identify the item.
 * Otherwise the other parameters are used (all must match):
 * - p_diag_source (all fields)
 * - Channel error type.
 * - Extended error type.
 *
 * Note that the severity or ExtChannelErrorAddValue are not used for
 * identification, so there could be no two diagnosis where these values
 * differ but the values above are the same.
 *
 * Similarly for diagnosis in USI format, the ManufacturerData is not used
 * for identification.
 *
 * @param net               InOut: The p-net stack instance.
 * @param p_diag_source     In:    Slot, subslot, channel, direction etc.
 * @param ch_error_type     In:    The channel error type.
 * @param ext_ch_error_type In:    The extended channel error type, or 0.
 * @param usi               In:    The USI.
 * @param pp_subslot        Out:   The sub-slot instance, or NULL if not found.
 * @param p_diag_ix         Out:   The diag item index.
 */
static void pf_diag_find_entry (
   pnet_t * net,
   const pnet_diag_source_t * p_diag_source,
   uint16_t ch_error_type,
   uint16_t ext_ch_error_type,
   uint16_t usi,
   pf_subslot_t ** pp_subslot,
   uint16_t * p_diag_ix)
{
   pf_diag_item_t * p_item = NULL;
   pf_diag_item_t * p_prev = NULL;
   uint16_t prev_ix;
   uint16_t item_ix;

   *p_diag_ix = PF_DIAG_IX_NULL;
   *pp_subslot = NULL;
   if (
      pf_cmdev_get_subslot_full (
         net,
         p_diag_source->api,
         p_diag_source->slot,
         p_diag_source->subslot,
         pp_subslot) == 0)
   {
      /* Only search valid sub-modules */
      if (
         (((*pp_subslot)->submodule_state.ident_info == PF_SUBMOD_PLUG_OK) ||
          ((*pp_subslot)->submodule_state.ident_info ==
           PF_SUBMOD_PLUG_SUBSTITUTE)) &&
         (((*pp_subslot)->submodule_state.ar_info == PF_SUBMOD_AR_INFO_OWN) ||
          ((*pp_subslot)->submodule_state.ar_info ==
           PF_SUBMOD_AR_INFO_APPLICATION_READY_PENDING)))
      {
         prev_ix = PF_DIAG_IX_NULL;
         item_ix = (*pp_subslot)->diag_list;
         pf_cmdev_get_diag_item (net, item_ix, &p_item);
         while ((p_item != NULL) && (*p_diag_ix == PF_DIAG_IX_NULL))
         {
            /* Found a match if all of:
             *    format is STD.
             *    channel number matches.
             *    individual channel or channel group
             *    direction matches.
             *    ch_error_type matches.
             *    ext_ch_error_type matches.
             * ToDo: How do we find manuf_data diag entries??
             */
            if (p_item->usi >= PF_USI_CHANNEL_DIAGNOSIS)
            {
               if (
                  (p_item->fmt.std.ch_nbr == p_diag_source->ch) &&
                  (p_item->fmt.std.ch_error_type == ch_error_type) &&
                  (p_item->fmt.std.ext_ch_error_type == ext_ch_error_type) &&
                  (PF_DIAG_CH_PROP_ACC_GET (p_item->fmt.std.ch_properties) ==
                   p_diag_source->ch_grouping) &&
                  (PF_DIAG_CH_PROP_DIR_GET (p_item->fmt.std.ch_properties) ==
                   p_diag_source->ch_direction))
               {
                  *p_diag_ix = item_ix;

                  /* Unlink it from the list so it can be updated. */
                  if (prev_ix != PF_DIAG_IX_NULL)
                  {
                     /* Not first in list */
                     pf_cmdev_get_diag_item (net, prev_ix, &p_prev);
                     p_prev->next = p_item->next;
                  }
                  else
                  {
                     /* Unlink the first item in the list */
                     (*pp_subslot)->diag_list = p_item->next;
                  }
               }
            }
            else if (p_item->usi == usi)
            {
               *p_diag_ix = item_ix;

               /* Unlink it from the list so it can be updated. */
               if (prev_ix != PF_DIAG_IX_NULL)
               {
                  /* Not first in list */
                  pf_cmdev_get_diag_item (net, prev_ix, &p_prev);
                  p_prev->next = p_item->next;
               }
               else
               {
                  /* Unlink the first item in the list */
                  (*pp_subslot)->diag_list = p_item->next;
               }
            }

            prev_ix = item_ix;
            item_ix = p_item->next;
            pf_cmdev_get_diag_item (net, item_ix, &p_item);
         }
      }
      else
      {
         /* Signal not found! */
         *pp_subslot = NULL;
      }
   }
}

/**
 * @internal
 * Find first used ar.
 *
 * @param net              InOut: The p-net stack instance.
 * @param pp_ar            Out:   Resulting ar
 * @return 0 on success, -1 on error.
 */
static int pf_diag_find_first_ar (pnet_t * net, pf_ar_t ** pp_ar)
{
   int ret = -1;
   uint16_t ix;

   for (ix = 0; ix < NELEMENTS (net->cmrpc_ar); ix++)
   {
      if (net->cmrpc_ar[ix].in_use == true)
      {
         *pp_ar = &net->cmrpc_ar[ix];
         ret = 0;
         break;
      }
   }

   return ret;
}

int pf_diag_add (
   pnet_t * net,
   const pnet_diag_source_t * p_diag_source,
   pnet_diag_ch_prop_type_values_t ch_bits,
   pnet_diag_ch_prop_maint_values_t severity,
   uint16_t ch_error_type,
   uint16_t ext_ch_error_type,
   uint32_t ext_ch_add_value,
   uint32_t qual_ch_qualifier,
   uint16_t usi,
   uint16_t manuf_data_len,
   const uint8_t * p_manuf_data)
{
   int ret = -1;
   pf_device_t * p_dev = NULL;
   pf_subslot_t * p_subslot = NULL;
   uint16_t item_ix = PF_DIAG_IX_NULL;
   pf_diag_item_t * p_item = NULL;
   bool overwrite = false;
   pf_diag_item_t old_item;
   uint16_t ch_properties = 0;
   pf_ar_t * p_ar = NULL;

   /* Remove reserved bits before we use it */
   /* TODO: Validate qual_ch_qualifier */
   qual_ch_qualifier &= PF_DIAG_QUALIFIED_SEVERITY_MASK;

   if (usi > PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS || usi == PF_USI_ALARM_MULTIPLE)
   {
      LOG_ERROR (PF_ALARM_LOG, "DIAG(%d): The given USI is invalid\n", __LINE__);
   }
   else if (usi < PF_USI_CHANNEL_DIAGNOSIS && ch_error_type != 0)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "DIAG(%d): The ch_error_type should not be used for USI %u\n",
         __LINE__,
         usi);
   }
   else if (usi <= PF_USI_CHANNEL_DIAGNOSIS && ext_ch_error_type != 0)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "DIAG(%d): The ext_ch_error_type should not be used for USI %u\n",
         __LINE__,
         usi);
   }
   else if (usi <= PF_USI_CHANNEL_DIAGNOSIS && ext_ch_add_value != 0)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "DIAG(%d): The ext_ch_add_value should not be used for USI %u\n",
         __LINE__,
         usi);
   }
   else if (usi != PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS && qual_ch_qualifier != 0)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "DIAG(%d): The qual_ch_qualifier should only be used when "
         "usi=PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS\n",
         __LINE__);
   }
   else if (
      usi != PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS &&
      severity == PNET_DIAG_CH_PROP_MAINT_QUALIFIED)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "DIAG(%d): The severity PNET_DIAG_CH_PROP_MAINT_QUALIFIED should only "
         "be used when usi=PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS\n",
         __LINE__);
   }
   else if (qual_ch_qualifier != 0 && severity != PNET_DIAG_CH_PROP_MAINT_QUALIFIED)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "DIAG(%d): The qual_ch_qualifier should only be used when "
         "severity=PNET_DIAG_CH_PROP_MAINT_QUALIFIED\n",
         __LINE__);
   }
   else if (
      usi >= PF_USI_CHANNEL_DIAGNOSIS &&
      (p_manuf_data != NULL || manuf_data_len != 0))
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "DIAG(%d): Manufacturer specific data should only be given for USI up "
         "to 0x7fff\n",
         __LINE__);
   }
   else if (manuf_data_len > 0 && p_manuf_data == NULL)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "DIAG(%d): manuf_data_len is %u but no data is given.\n",
         __LINE__,
         manuf_data_len);
   }
   else if (p_diag_source->ch > PNET_CHANNEL_WHOLE_SUBMODULE)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "DIAG(%d): Illegal channel %u\n",
         __LINE__,
         p_diag_source->ch);
   }
   else if (manuf_data_len > sizeof (p_item->fmt.usi.manuf_data))
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "DIAG(%d): Too long manufacturer specific data given: %u Max: %zu "
         "Increase PNET_MAX_DIAG_MANUF_DATA_SIZE\n",
         __LINE__,
         manuf_data_len,
         sizeof (p_item->fmt.usi.manuf_data));
   }
   else if (pf_cmdev_get_device (net, &p_dev) == 0)
   {
      os_mutex_lock (p_dev->diag_mutex);

      pf_diag_find_entry (
         net,
         p_diag_source,
         ch_error_type,
         ext_ch_error_type,
         usi,
         &p_subslot,
         &item_ix);

      if (p_subslot != NULL)
      {
         if (item_ix == PF_DIAG_IX_NULL)
         {
            /* Allocate a new entry and determine format */
            if (pf_cmdev_new_diag (net, &item_ix) == 0)
            {
               (void)pf_cmdev_get_diag_item (net, item_ix, &p_item);
            }
            else
            {
               /* Diag overflow */
               /* ToDo: Handle diag overflow */
               LOG_ERROR (PF_ALARM_LOG, "DIAG(%d): overflow\n", __LINE__);
            }
         }
         else
         {
            /* Re-use the old entry */
            (void)pf_cmdev_get_diag_item (net, item_ix, &p_item);
            overwrite = true;
         }

         if (p_item != NULL)
         {
            old_item = *p_item; /* In case we need to remove the old - after
                                   inserting the new */

            if (usi < PF_USI_CHANNEL_DIAGNOSIS)
            {
               /* Manufacturer specific (USI) format */
               LOG_DEBUG (
                  PF_ALARM_LOG,
                  "DIAG(%d): Adding manufacturer specific diag, ix %u. USI "
                  "0x%04X Slot %u Subslot 0x%04X\n",
                  __LINE__,
                  item_ix,
                  usi,
                  p_diag_source->slot,
                  p_diag_source->subslot);

               p_item->usi = usi;
               p_item->fmt.usi.len = manuf_data_len;
               if (manuf_data_len > 0)
               {
                  memcpy (
                     p_item->fmt.usi.manuf_data,
                     p_manuf_data,
                     manuf_data_len);
               }
            }
            else
            {
               /* Standard format */
               LOG_DEBUG (
                  PF_ALARM_LOG,
                  "DIAG(%d): Adding standard format diagnosis, ix %u. USI "
                  "0x%04X Slot "
                  "%u Subslot 0x%04X Channel 0x%04X\n",
                  __LINE__,
                  item_ix,
                  usi,
                  p_diag_source->slot,
                  p_diag_source->subslot,
                  p_diag_source->ch);

               PF_DIAG_CH_PROP_TYPE_SET (ch_properties, ch_bits);
               PF_DIAG_CH_PROP_ACC_SET (
                  ch_properties,
                  p_diag_source->ch_grouping == PNET_DIAG_CH_CHANNEL_GROUP);
               PF_DIAG_CH_PROP_MAINT_SET (ch_properties, severity);
               PF_DIAG_CH_PROP_DIR_SET (
                  ch_properties,
                  p_diag_source->ch_direction);
               PF_DIAG_CH_PROP_SPEC_SET (
                  ch_properties,
                  PF_DIAG_CH_PROP_SPEC_APPEARS);

               p_item->usi = usi;
               p_item->fmt.std.ch_nbr = p_diag_source->ch;
               p_item->fmt.std.ch_properties = ch_properties;
               p_item->fmt.std.ch_error_type = ch_error_type;
               p_item->fmt.std.ext_ch_error_type = ext_ch_error_type;
               p_item->fmt.std.ext_ch_add_value = ext_ch_add_value;
               p_item->fmt.std.qual_ch_qualifier = qual_ch_qualifier;
            }

            /* Link it into the sub-slot reported list */
            p_item->next = p_subslot->diag_list;
            p_subslot->diag_list = item_ix;

            pf_diag_update_submodule_state (net, p_ar, p_subslot);

            /* TODO Use better strategy to find AR */
            if (pf_diag_find_first_ar (net, &p_ar) == 0)
            {
               ret = pf_alarm_send_diagnosis (
                  net,
                  p_ar,
                  p_diag_source->api,
                  p_diag_source->slot,
                  p_diag_source->subslot,
                  p_item);

               pf_diag_update_station_problem_indicator (net, p_ar, p_subslot);

               if (overwrite == true)
               {
                  /* Time to remove the previous entry */
                  /* Remove the old diag by sending a disappear alarm */

                  if (old_item.usi >= PF_USI_CHANNEL_DIAGNOSIS)
                  {
                     /* Standard format */
                     PF_DIAG_CH_PROP_SPEC_SET (
                        old_item.fmt.std.ch_properties,
                        PF_DIAG_CH_PROP_SPEC_DIS_OTHERS_REMAIN);
                     ret = pf_alarm_send_diagnosis (
                        net,
                        p_ar,
                        p_diag_source->api,
                        p_diag_source->slot,
                        p_diag_source->subslot,
                        &old_item);
                  }

                  /* Do not send a disappear alarm when updating diagnosis in
                     USI format */
               }
            }
            else
            {
               LOG_DEBUG (
                  PNET_LOG,
                  "DIAG(%d): No active connection, so no alarm is sent.\n",
                  __LINE__);
            }
         }
         else
         {
            LOG_ERROR (
               PNET_LOG,
               "DIAG(%d): Can not add diagnosis. Alarm "
               "item is NULL\n",
               __LINE__);
         }
      }
      else
      {
         LOG_ERROR (
            PF_ALARM_LOG,
            "DIAG(%d): Can not add diagnosis. Unknown sub-slot 0x%04x.\n",
            __LINE__,
            p_diag_source->subslot);
      }

      os_mutex_unlock (p_dev->diag_mutex);
   }
   else
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "DIAG(%d): Can not add diagnosis, as the CMDEV not is initialized.\n",
         __LINE__);
   }

   return ret;
}

int pf_diag_update (
   pnet_t * net,
   const pnet_diag_source_t * p_diag_source,
   uint16_t ch_error_type,
   uint16_t ext_ch_error_type,
   uint32_t ext_ch_add_value,
   uint16_t usi,
   uint16_t manuf_data_len,
   const uint8_t * p_manuf_data)
{
   int ret = -1;
   pf_device_t * p_dev = NULL;
   pf_subslot_t * p_subslot = NULL;
   pf_diag_item_t * p_item = NULL;
   pf_diag_item_t old_item;
   uint16_t item_ix = PF_DIAG_IX_NULL;
   pf_ar_t * p_ar = NULL;

   if (usi > PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS || usi == PF_USI_ALARM_MULTIPLE)
   {
      LOG_ERROR (PF_ALARM_LOG, "DIAG(%d): The given USI is invalid\n", __LINE__);
   }
   else if (usi < PF_USI_CHANNEL_DIAGNOSIS && ch_error_type != 0)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "DIAG(%d): The ch_error_type should not be used for USI %u\n",
         __LINE__,
         usi);
   }
   else if (usi <= PF_USI_CHANNEL_DIAGNOSIS && ext_ch_error_type != 0)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "DIAG(%d): The ext_ch_error_type should not be used for USI %u\n",
         __LINE__,
         usi);
   }
   else if (usi <= PF_USI_CHANNEL_DIAGNOSIS && ext_ch_add_value != 0)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "DIAG(%d): The ext_ch_add_value should not be used for USI %u\n",
         __LINE__,
         usi);
   }
   else if (
      usi >= PF_USI_CHANNEL_DIAGNOSIS &&
      (p_manuf_data != NULL || manuf_data_len != 0))
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "DIAG(%d): Manufacturer specific data should only be given for USI up "
         "to 0x7fff\n",
         __LINE__);
   }
   else if (manuf_data_len > 0 && p_manuf_data == NULL)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "DIAG(%d): manuf_data_len is %u but no data is given.\n",
         __LINE__,
         manuf_data_len);
   }
   else if (manuf_data_len > sizeof (p_item->fmt.usi.manuf_data))
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "DIAG(%d): Too long manufacturer specific data given: %u\n",
         __LINE__,
         manuf_data_len);
   }
   else if (p_diag_source->ch > PNET_CHANNEL_WHOLE_SUBMODULE)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "DIAG(%d): Illegal channel %u\n",
         __LINE__,
         p_diag_source->ch);
   }
   else if (pf_cmdev_get_device (net, &p_dev) == 0)
   {
      os_mutex_lock (p_dev->diag_mutex);

      pf_diag_find_entry (
         net,
         p_diag_source,
         ch_error_type,
         ext_ch_error_type,
         usi,
         &p_subslot,
         &item_ix);
      if (p_subslot != NULL)
      {
         if (pf_cmdev_get_diag_item (net, item_ix, &p_item) == 0)
         {
            /* Make a copy of it so it can be removed after inserting the new
             * entry */
            old_item = *p_item;

            if (p_item->usi < PF_USI_CHANNEL_DIAGNOSIS)
            {
               /* Manufacturer specific */
               p_item->fmt.usi.len = manuf_data_len;
               if (manuf_data_len > 0)
               {
                  memcpy (
                     p_item->fmt.usi.manuf_data,
                     p_manuf_data,
                     manuf_data_len);
               }
            }
            else
            {
               PF_DIAG_CH_PROP_SPEC_SET (
                  p_item->fmt.std.ch_properties,
                  PF_DIAG_CH_PROP_SPEC_APPEARS);
               p_item->fmt.std.ext_ch_add_value = ext_ch_add_value;
            }

            /* Link it into the sub-slot diag list */
            p_item->next = p_subslot->diag_list;
            p_subslot->diag_list = item_ix;

            pf_diag_update_submodule_state (net, p_ar, p_subslot);

            /* TODO Use better strategy to find AR */
            if (pf_diag_find_first_ar (net, &p_ar) == 0)
            {
               ret = pf_alarm_send_diagnosis (
                  net,
                  p_ar,
                  p_diag_source->api,
                  p_diag_source->slot,
                  p_diag_source->subslot,
                  p_item);

               pf_diag_update_station_problem_indicator (net, p_ar, p_subslot);

               /* Remove the old diag by sending a disappear alarm.
                  The old item should be removed after the new is added */
               if (old_item.usi >= PF_USI_CHANNEL_DIAGNOSIS)
               {
                  /* Standard format */
                  PF_DIAG_CH_PROP_SPEC_SET (
                     old_item.fmt.std.ch_properties,
                     PF_DIAG_CH_PROP_SPEC_DIS_OTHERS_REMAIN);
                  ret = pf_alarm_send_diagnosis (
                     net,
                     p_ar,
                     p_diag_source->api,
                     p_diag_source->slot,
                     p_diag_source->subslot,
                     &old_item);
               }

               /* Do not send a disappear alarm when updating diagnosis in
                  USI format */
            }
            else
            {
               LOG_ERROR (
                  PNET_LOG,
                  "DIAG(%d): No active connection, so no alarm is sent.\n",
                  __LINE__);
            }
         }
         else
         {
            /* Diag not found */
            LOG_ERROR (
               PF_ALARM_LOG,
               "DIAG(%d): Diag not found. No update possible.\n",
               __LINE__);
         }
      }
      else
      {
         LOG_ERROR (PF_ALARM_LOG, "DIAG(%d): Unknown sub-slot\n", __LINE__);
      }

      os_mutex_unlock (p_dev->diag_mutex);
   }

   return ret;
}

int pf_diag_remove (
   pnet_t * net,
   const pnet_diag_source_t * p_diag_source,
   uint16_t ch_error_type,
   uint16_t ext_ch_error_type,
   uint16_t usi)
{
   int ret = -1;
   pf_device_t * p_dev = NULL;
   pf_subslot_t * p_subslot = NULL;
   uint16_t item_ix = PF_DIAG_IX_NULL;
   pf_diag_item_t * p_item = NULL;
   pf_ar_t * p_ar = NULL;

   if (usi > PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS || usi == PF_USI_ALARM_MULTIPLE)
   {
      LOG_ERROR (PF_ALARM_LOG, "DIAG(%d): The given USI is invalid\n", __LINE__);
   }
   else if (usi < PF_USI_CHANNEL_DIAGNOSIS && ch_error_type != 0)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "DIAG(%d): The ch_error_type should not be used for USI %u\n",
         __LINE__,
         usi);
   }
   else if (usi <= PF_USI_CHANNEL_DIAGNOSIS && ext_ch_error_type != 0)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "DIAG(%d): The ext_ch_error_type should not be used for USI %u\n",
         __LINE__,
         usi);
   }
   else if (p_diag_source->ch > PNET_CHANNEL_WHOLE_SUBMODULE)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "DIAG(%d): Illegal channel %u\n",
         __LINE__,
         p_diag_source->ch);
   }
   else if (pf_cmdev_get_device (net, &p_dev) == 0)
   {
      os_mutex_lock (p_dev->diag_mutex);

      pf_diag_find_entry (
         net,
         p_diag_source,
         ch_error_type,
         ext_ch_error_type,
         usi,
         &p_subslot,
         &item_ix);

      os_mutex_unlock (p_dev->diag_mutex);

      if ((p_subslot != NULL) && (item_ix != PF_DIAG_IX_NULL))
      {
         if (pf_cmdev_get_diag_item (net, item_ix, &p_item) == 0)
         {
            /* TODO Use better strategy to find AR */
            if (pf_diag_find_first_ar (net, &p_ar) == 0)
            {
               if (p_subslot->diag_list != PF_DIAG_IX_NULL)
               {
                  /*
                   * If diagnosis information is removed a diagnosis alarm with
                   * specifier DISAPPEARS is queued into the alarm queue. ToDo:
                   * If the channel has other diagnosis information of the same
                   * severity, the specifier PNET_DIAG_CH_SPEC_DIS_OTHERS_REMAIN
                   * is used instead.
                   */
                  if (p_item->usi >= PF_USI_CHANNEL_DIAGNOSIS)
                  {
                     PF_DIAG_CH_PROP_SPEC_SET (
                        p_item->fmt.std.ch_properties,
                        PF_DIAG_CH_PROP_SPEC_DISAPPEARS);
                  }
               }
               else
               {
                  /* If the channel has no diagnosis information of any
                   * severity,
                   * the specifier PNET_DIAG_CH_SPEC_ALL_DISAPPEARS is used. */
                  /* Overwrite the item to remove with correct cr_properties for
                   * this diag message. */
                  p_item->usi = PF_USI_EXTENDED_CHANNEL_DIAGNOSIS;
                  PF_DIAG_CH_PROP_MAINT_SET (
                     p_item->fmt.std.ch_properties,
                     PNET_DIAG_CH_PROP_MAINT_FAULT);
                  PF_DIAG_CH_PROP_SPEC_SET (
                     p_item->fmt.std.ch_properties,
                     PF_DIAG_CH_PROP_SPEC_ALL_DISAPPEARS);
               }

               /* Remove the old diag by sending a disappear alarm */
               if (usi >= PF_USI_CHANNEL_DIAGNOSIS)
               {
                  /* Standard format */
                  ret = pf_alarm_send_diagnosis (
                     net,
                     p_ar,
                     p_diag_source->api,
                     p_diag_source->slot,
                     p_diag_source->subslot,
                     p_item);
               }
               else
               {
                  /* USI format */
                  ret = pf_alarm_send_usi_diagnosis_disappears (
                     net,
                     p_ar,
                     p_diag_source->api,
                     p_diag_source->slot,
                     p_diag_source->subslot,
                     usi);
               }
            }
            else
            {
               LOG_DEBUG (
                  PNET_LOG,
                  "DIAG(%d): No active connection, so no alarm is sent.\n",
                  __LINE__);
            }
            pf_diag_update_submodule_state (net, p_ar, p_subslot);
         }

         /* Free diag entry */
         pf_cmdev_free_diag (net, item_ix);

         if (p_ar != NULL)
         {
            pf_diag_update_station_problem_indicator (net, p_ar, p_subslot);
         }
      }
      else
      {
         LOG_DEBUG (
            PF_ALARM_LOG,
            "DIAG(%d): Did not find the diagnosis to remove. Slot %u Subslot "
            "0x%04x Channel 0x%04x Ch err type 0x%04x Ext err type 0x%04x Usi "
            "0x%04x\n",
            __LINE__,
            p_diag_source->slot,
            p_diag_source->subslot,
            p_diag_source->ch,
            ch_error_type,
            ext_ch_error_type,
            usi);
      }
   }

   return ret;
}

/************************** Diagnosis in standard format *********************/

/**
 * @internal
 * Get the preferred diagnosis USI, for use with standard format.
 *
 * @param net              InOut: The p-net stack instance.
 * @return the USI
 */
static uint16_t pf_diag_get_preferred_usi (pnet_t * net)
{
   pnet_cfg_t * p_cfg = NULL;
   pf_fspm_get_cfg (net, &p_cfg);
   if (p_cfg->use_qualified_diagnosis == true)
   {
      return PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS;
   }
   return PF_USI_EXTENDED_CHANNEL_DIAGNOSIS;
}

int pf_diag_std_add (
   pnet_t * net,
   const pnet_diag_source_t * p_diag_source,
   pnet_diag_ch_prop_type_values_t ch_bits,
   pnet_diag_ch_prop_maint_values_t severity,
   uint16_t ch_error_type,
   uint16_t ext_ch_error_type,
   uint32_t ext_ch_add_value,
   uint32_t qual_ch_qualifier)
{
   return pf_diag_add (
      net,
      p_diag_source,
      ch_bits,
      severity,
      ch_error_type,
      ext_ch_error_type,
      ext_ch_add_value,
      qual_ch_qualifier,
      pf_diag_get_preferred_usi (net),
      0,
      NULL);
}

int pf_diag_std_update (
   pnet_t * net,
   const pnet_diag_source_t * p_diag_source,
   uint16_t ch_error_type,
   uint16_t ext_ch_error_type,
   uint32_t ext_ch_add_value)
{
   return pf_diag_update (
      net,
      p_diag_source,
      ch_error_type,
      ext_ch_error_type,
      ext_ch_add_value,
      pf_diag_get_preferred_usi (net),
      0,
      NULL);
}

int pf_diag_std_remove (
   pnet_t * net,
   const pnet_diag_source_t * p_diag_source,
   uint16_t ch_error_type,
   uint16_t ext_ch_error_type)
{
   return pf_diag_remove (
      net,
      p_diag_source,
      ch_error_type,
      ext_ch_error_type,
      pf_diag_get_preferred_usi (net));
}

/************************** Diagnosis in USI format **************************/

int pf_diag_usi_add (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint16_t usi,
   uint16_t manuf_data_len,
   const uint8_t * p_manuf_data)
{
   pnet_diag_source_t diag_source = {
      .api = api_id,
      .slot = slot_nbr,
      .subslot = subslot_nbr,
      .ch = PNET_CHANNEL_WHOLE_SUBMODULE,
      .ch_grouping = PNET_DIAG_CH_INDIVIDUAL_CHANNEL,
      .ch_direction = PNET_DIAG_CH_PROP_DIR_MANUF_SPEC};

   if (usi >= PF_USI_CHANNEL_DIAGNOSIS)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "DIAG(%d): Wrong USI given for adding diagnosis: %u Slot: %u Subslot "
         "%u\n",
         __LINE__,
         usi,
         slot_nbr,
         subslot_nbr);
      return -1;
   }

   return pf_diag_add (
      net,
      &diag_source,
      PNET_DIAG_CH_PROP_TYPE_UNSPECIFIED,
      PNET_DIAG_CH_PROP_MAINT_FAULT,
      0,
      0,
      0,
      0,
      usi,
      manuf_data_len,
      p_manuf_data);
}

int pf_diag_usi_update (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint16_t usi,
   uint16_t manuf_data_len,
   const uint8_t * p_manuf_data)
{
   pnet_diag_source_t diag_source = {
      .api = api_id,
      .slot = slot_nbr,
      .subslot = subslot_nbr,
      .ch = PNET_CHANNEL_WHOLE_SUBMODULE,
      .ch_grouping = PNET_DIAG_CH_INDIVIDUAL_CHANNEL,
      .ch_direction = PNET_DIAG_CH_PROP_DIR_MANUF_SPEC};

   if (usi >= PF_USI_CHANNEL_DIAGNOSIS)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "DIAG(%d): Wrong USI given for updating diagnosis: %u Slot: %u "
         "Subslot %u\n",
         __LINE__,
         usi,
         slot_nbr,
         subslot_nbr);
      return -1;
   }

   return pf_diag_update (
      net,
      &diag_source,
      0,
      0,
      0,
      usi,
      manuf_data_len,
      p_manuf_data);
}

int pf_diag_usi_remove (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint16_t usi)
{
   pnet_diag_source_t diag_source = {
      .api = api_id,
      .slot = slot_nbr,
      .subslot = subslot_nbr,
      .ch = PNET_CHANNEL_WHOLE_SUBMODULE,
      .ch_grouping = PNET_DIAG_CH_INDIVIDUAL_CHANNEL,
      .ch_direction = PNET_DIAG_CH_PROP_DIR_MANUF_SPEC};

   if (usi >= PF_USI_CHANNEL_DIAGNOSIS)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "DIAG(%d): Wrong USI given for diagnosis removal: %u Slot: %u Subslot "
         "%u\n",
         __LINE__,
         usi,
         slot_nbr,
         subslot_nbr);
      return -1;
   }

   return pf_diag_remove (net, &diag_source, 0, 0, usi);
}
