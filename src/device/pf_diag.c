
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

#ifdef UNIT_TEST
#define pf_alarm_send_diagnosis     mock_pf_alarm_send_diagnosis
#endif


#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <inttypes.h>

#include "pf_includes.h"
#include "pf_block_reader.h"
#include "pf_block_writer.h"


int pf_diag_init(void)
{
   return 0;
}

int pf_diag_exit(void)
{
   return 0;
}

/**
 * @internal
 * Update the problem indicator for all input/producer CRs of a sub-slot.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param p_subslot        In:   The sub-slot instance.
 */
static void pf_diag_update_station_problem_indicator(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pf_subslot_t            *p_subslot)
{
   bool                    is_problem = false;
   pf_device_t             *p_dev = NULL;
   uint16_t                ix;
   pf_diag_item_t          *p_item;

   if (pf_cmdev_get_device(net, &p_dev) == 0)
   {
      /* A problem is indicated if at least one FAULT diagnosis exists. */
      ix = p_subslot->diag_list;
      pf_cmdev_get_diag_item(net, ix, &p_item);
      while (p_item != NULL)
      {
         if ((p_item->in_use == true) &&
             ((p_item->usi < PF_USI_CHANNEL_DIAGNOSIS) ||
              (PNET_DIAG_CH_PROP_MAINT_GET(p_item->fmt.std.ch_properties) == PNET_DIAG_CH_PROP_MAINT_FAULT)))
         {
            is_problem = true;
         }

         ix = p_item->next;
         pf_cmdev_get_diag_item(net, ix, &p_item);
      }
   }

   pf_ppm_set_problem_indicator(p_ar, is_problem);
}

/**
 * @internal
 * Find and unlink a diag item in the specified sub-slot.
 *
 * If the USI is in the manufacturer-specified range then only match the USI.
 * Otherwise the following items must match (not the USI!):
 * - API id.
 * - Slot number.
 * - Sub-slot number.
 * - Channel number.
 * - Channel properties.
 * - Channel error type.
 *
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:   The API id.
 * @param slot_nbr         In:   The slot number.
 * @param subslot_nbr      In:   The sub-slot number.
 * @param ch_nbr           In:   The channel number.
 * @param ch_properties    In:   The channel properties.
 * @param ch_error_type    In:   The error type.
 * @param usi              In:   The USI.
 * @param pp_subslot       Out:  The sub-slot instance.
 * @param p_diag_ix        Out:  The diag item index.
 */
static void pf_diag_find_entry(
   pnet_t                  *net,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   uint16_t                ch_nbr,
   uint16_t                ch_properties,
   uint16_t                ch_error_type,
   uint16_t                usi,
   pf_subslot_t            **pp_subslot,
   uint16_t                *p_diag_ix)
{
   pf_diag_item_t          *p_item = NULL;
   pf_diag_item_t          *p_prev = NULL;
   uint16_t                prev_ix;
   uint16_t                item_ix;


   *p_diag_ix = PF_DIAG_IX_NULL;
   *pp_subslot = NULL;
   if (pf_cmdev_get_subslot_full(net, api_id, slot_nbr, subslot_nbr, pp_subslot) == 0)
   {
      /* Only search valid sub-modules */
      if ((((*pp_subslot)->submodule_state.ident_info == PF_SUBMOD_PLUG_OK) ||
           ((*pp_subslot)->submodule_state.ident_info == PF_SUBMOD_PLUG_SUBSTITUTE)) &&
          (((*pp_subslot)->submodule_state.ar_info == PF_SUBMOD_AR_INFO_OWN) ||
           ((*pp_subslot)->submodule_state.ar_info == PF_SUBMOD_AR_INFO_APPLICATION_READY_PENDING)))
      {
         prev_ix = PF_DIAG_IX_NULL;
         item_ix = (*pp_subslot)->diag_list;
         pf_cmdev_get_diag_item(net, item_ix, &p_item);
         while ((p_item != NULL) && (*p_diag_ix == PF_DIAG_IX_NULL))
         {
            /* Found a match if all of:
             *    format is STD.
             *    ch_nbr matches.
             *    direction matches.
             *    ch_error_type matches.
             * ToDo: How do we find manuf_data diag entries??
             */
            if (p_item->usi >= PF_USI_CHANNEL_DIAGNOSIS)
            {
               if ((p_item->fmt.std.ch_nbr == ch_nbr) &&
                   (p_item->fmt.std.ch_error_type == ch_error_type) &&
                   (PNET_DIAG_CH_PROP_DIR_GET(p_item->fmt.std.ch_properties) == PNET_DIAG_CH_PROP_DIR_GET(ch_properties)))
               {
                  *p_diag_ix = item_ix;

                  /* Unlink it from the list so it can be updated. */
                  if (prev_ix != PF_DIAG_IX_NULL)
                  {
                     /* Not first in list */
                     pf_cmdev_get_diag_item(net, prev_ix, &p_prev);
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
                  pf_cmdev_get_diag_item(net, prev_ix, &p_prev);
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
            pf_cmdev_get_diag_item(net, item_ix, &p_item);
         }
      }
      else
      {
         /* Signal not found! */
         *pp_subslot = NULL;
      }
   }
}

int pf_diag_add(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   uint16_t                ch_nbr,
   uint16_t                ch_properties,
   uint16_t                ch_error_type,
   uint16_t                ext_ch_error_type,
   uint32_t                ext_ch_add_value,
   uint32_t                qual_ch_qualifier,
   uint16_t                usi,
   uint8_t                 *p_manuf_data)
{
   int                     ret = -1;
   pf_device_t             *p_dev = NULL;
   pf_subslot_t            *p_subslot = NULL;
   uint16_t                item_ix = PF_DIAG_IX_NULL;
   pf_diag_item_t          *p_item = NULL;
   bool                    overwrite = false;
   pf_diag_item_t          old_item;

   /* Remove reserved bits before we use it */
   qual_ch_qualifier &= ~0x00000007;

   if (usi > PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS)
   {
      LOG_ERROR(PNET_LOG, "DIAG(%d): Bad USI\n", __LINE__);
   }
   else if (pf_cmdev_get_device(net, &p_dev) == 0)
   {
      os_mutex_lock(p_dev->diag_mutex);

      pf_diag_find_entry(net, api_id, slot_nbr, subslot_nbr, ch_nbr, ch_properties, ch_error_type, usi,
         &p_subslot, &item_ix);
      if (p_subslot != NULL)
      {
         if (item_ix == PF_DIAG_IX_NULL)
         {
            /* Allocate a new entry and determine format */
            if (pf_cmdev_new_diag(net, &item_ix) == 0)
            {
               (void)pf_cmdev_get_diag_item(net, item_ix, &p_item);
            }
            else
            {
               /* Diag overflow */
               /* ToDo: Handle diag overflow */
               LOG_ERROR(PNET_LOG, "DIAG(%d): overflow\n", __LINE__);
            }
         }
         else
         {
            /* Re-use the old entry */
            (void)pf_cmdev_get_diag_item(net, item_ix, &p_item);
            overwrite = true;
         }

         if (p_item != NULL)
         {
            old_item = *p_item;  /* In case we need to remove the old - after inserting the new */

            /* Determine diag format */
            if (usi < PF_USI_CHANNEL_DIAGNOSIS)
            {
               p_item->usi = usi;
               memcpy(p_item->fmt.usi.manuf_data, p_manuf_data, sizeof(p_item->fmt.usi.manuf_data));
            }
            else
            {
               p_item->usi = PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS;
               PNET_DIAG_CH_PROP_SPEC_SET(p_item->fmt.std.ch_properties, PNET_DIAG_CH_PROP_SPEC_APPEARS);

               p_item->fmt.std.ch_nbr = ch_nbr;
               p_item->fmt.std.ch_properties = ch_properties;
               p_item->fmt.std.ch_error_type = ch_error_type;
               p_item->fmt.std.ext_ch_error_type = ext_ch_error_type;
               p_item->fmt.std.ext_ch_add_value = ext_ch_add_value;
               p_item->fmt.std.qual_ch_qualifier = qual_ch_qualifier;
            }

            ret = pf_alarm_send_diagnosis(net, p_ar, api_id, slot_nbr, subslot_nbr, p_item);

            pf_diag_update_station_problem_indicator(net, p_ar, p_subslot);

            if (ret == 0)
            {
               /* Link it into the sub-slot reported list */
               p_item->next = p_subslot->diag_list;
               p_subslot->diag_list = item_ix;

               if (overwrite == true)
               {
                  /* Time to remove the previous entry */
                  /* Remove the old diag by sending a disappear alarm */
                  if (old_item.usi >= PF_USI_CHANNEL_DIAGNOSIS)
                  {
                     PNET_DIAG_CH_PROP_SPEC_SET(old_item.fmt.std.ch_properties, PNET_DIAG_CH_PROP_SPEC_DIS_OTHERS_REMAIN);
                  }
                  ret = pf_alarm_send_diagnosis(net, p_ar, api_id, slot_nbr, subslot_nbr, &old_item);
               }
            }
         }
      }
      else
      {
         LOG_ERROR(PNET_LOG, "DIAG(%d): Unknown sub-slot\n", __LINE__);
      }

      os_mutex_unlock(p_dev->diag_mutex);
   }

   return ret;
}

int pf_diag_update(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   uint16_t                ch_nbr,
   uint16_t                ch_properties,
   uint16_t                ch_error_code,
   uint32_t                ext_ch_add_value,
   uint16_t                usi,
   uint8_t                 *p_manuf_data)
{
   int                     ret = -1;
   pf_device_t             *p_dev = NULL;
   pf_subslot_t            *p_subslot = NULL;
   pf_diag_item_t          *p_item = NULL;
   pf_diag_item_t          old_item;
   uint16_t                item_ix = PF_DIAG_IX_NULL;

   if (usi > PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS)
   {
      LOG_ERROR(PNET_LOG, "DIAG(%d): Bad USI\n", __LINE__);
   }
   else if (pf_cmdev_get_device(net, &p_dev) == 0)
   {
      os_mutex_lock(p_dev->diag_mutex);

      pf_diag_find_entry(net, api_id, slot_nbr, subslot_nbr, ch_nbr, ch_properties, ch_error_code, usi,
         &p_subslot, &item_ix);
      if (p_subslot != NULL)
      {
         if (pf_cmdev_get_diag_item(net, item_ix, &p_item) == 0)
         {
            /* Make a copy of it so it can be remove after inserting the new entry */
            old_item = *p_item;

            /* Update diag entry */
            if (p_item->usi < PF_USI_CHANNEL_DIAGNOSIS)
            {
               memcpy(p_item->fmt.usi.manuf_data, p_manuf_data, sizeof(p_item->fmt.usi.manuf_data));
            }
            else
            {
               PNET_DIAG_CH_PROP_SPEC_SET(p_item->fmt.std.ch_properties, PNET_DIAG_CH_PROP_SPEC_APPEARS);
               p_item->fmt.std.ext_ch_add_value = ext_ch_add_value;
            }

            ret = pf_alarm_send_diagnosis(net, p_ar, api_id, slot_nbr, subslot_nbr, p_item);
            if (ret == 0)
            {
               /* Link it into the sub-slot diag list */
               p_item->next = p_subslot->diag_list;
               p_subslot->diag_list = item_ix;

               if (old_item.usi >= PF_USI_CHANNEL_DIAGNOSIS)
               {
                  PNET_DIAG_CH_PROP_SPEC_SET(old_item.fmt.std.ch_properties, PNET_DIAG_CH_PROP_SPEC_DIS_OTHERS_REMAIN);
               }
               /* Remove the old diag by sending a disappear alarm */
               ret = pf_alarm_send_diagnosis(net, p_ar, api_id, slot_nbr, subslot_nbr, &old_item);
            }

            pf_diag_update_station_problem_indicator(net, p_ar, p_subslot);
         }
         else
         {
            /* Diag not found */
            LOG_ERROR(PNET_LOG, "DIAG(%d): Diag not found\n", __LINE__);
         }
      }
      else
      {
         LOG_ERROR(PNET_LOG, "DIAG(%d): Unknown sub-slot\n", __LINE__);
      }

      os_mutex_unlock(p_dev->diag_mutex);
   }

   return ret;
}

int pf_diag_remove(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   uint16_t                ch_nbr,
   uint16_t                ch_properties,
   uint16_t                ch_error_type,
   uint16_t                usi)
{
   int                     ret = -1;
   pf_device_t             *p_dev = NULL;
   pf_subslot_t            *p_subslot = NULL;
   uint16_t                item_ix = PF_DIAG_IX_NULL;
   pf_diag_item_t          *p_item = NULL;

   if (usi > PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS)
   {
      LOG_ERROR(PNET_LOG, "DIAG(%d): Bad USI\n", __LINE__);
   }
   else if (pf_cmdev_get_device(net, &p_dev) == 0)
   {
      os_mutex_lock(p_dev->diag_mutex);

      pf_diag_find_entry(net, api_id, slot_nbr, subslot_nbr, ch_nbr, ch_properties, ch_error_type, usi,
         &p_subslot, &item_ix);

      os_mutex_unlock(p_dev->diag_mutex);

      if ((p_subslot != NULL) && (item_ix != PF_DIAG_IX_NULL))
      {
         if (pf_cmdev_get_diag_item(net, item_ix, &p_item) == 0)
         {
            if (p_subslot->diag_list != PF_DIAG_IX_NULL)
            {
               /*
                * If diagnosis information is removed a diagnosis alarm with specifier DISAPPEARS is queued into the alarm queue.
                * ToDo: If the channel has other diagnosis information of the same severity, the specifier PNET_DIAG_CH_SPEC_DIS_OTHERS_REMAIN
                * is used instead.
                */
               if (p_item->usi >= PF_USI_CHANNEL_DIAGNOSIS)
               {
                  PNET_DIAG_CH_PROP_SPEC_SET(p_item->fmt.std.ch_properties, PNET_DIAG_CH_PROP_SPEC_DISAPPEARS);
               }
               ret = pf_alarm_send_diagnosis(net, p_ar, api_id, slot_nbr, subslot_nbr, p_item);
            }
            else
            {
               /* If the channel has no diagnosis information of any severity, the specifier PNET_DIAG_CH_SPEC_ALL_DISAPPEARS is used. */
               /* Overwrite the item to remove with correct cr_properties for this diag message. */
               p_item->usi = PF_USI_CHANNEL_DIAGNOSIS;
               PNET_DIAG_CH_PROP_MAINT_SET(p_item->fmt.std.ch_properties, PNET_DIAG_CH_PROP_MAINT_FAULT);
               PNET_DIAG_CH_PROP_SPEC_SET(p_item->fmt.std.ch_properties, PNET_DIAG_CH_PROP_SPEC_ALL_DISAPPEARS);

               ret = pf_alarm_send_diagnosis(net, p_ar, api_id, slot_nbr, subslot_nbr, p_item);
            }
         }

         /* Free diag entry */
         pf_cmdev_free_diag(net, item_ix);

         pf_diag_update_station_problem_indicator(net, p_ar, p_subslot);
      }
      else
      {
         LOG_ERROR(PNET_LOG, "DIAG(%d): Removable item not found\n", __LINE__);
      }
   }

   return ret;
}
