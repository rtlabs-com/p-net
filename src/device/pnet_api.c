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
#define os_udp_recvfrom mock_os_udp_recvfrom
#define os_udp_sendto mock_os_udp_sendto
#define os_udp_open mock_os_udp_open
#define os_udp_close mock_os_udp_close

#define os_eth_init mock_os_eth_init
#endif

#include <string.h>
#include "osal.h"
#include "pf_includes.h"
#include "pf_block_reader.h"

int pnet_init(
   const char              *netif,
   uint32_t                tick_us,
   const pnet_cfg_t        *p_cfg)
{
   int                     ret = 0;
   int                     if_id;

   /* pnet_cm_init_req */
   pf_fspm_init(p_cfg);    /* Init cfg */

   /* Initialize everything (and the DCP protocol) */
   /* First initialize the network interface */
   if_id = os_eth_init(netif);
   pf_eth_init();
   pf_scheduler_init(tick_us);
   pf_cmina_init();  /* Read from permanent pool */

   pf_dcp_exit();    /* Prepare for re-init. */
   pf_dcp_init(if_id);    /* Start DCP */

   if (ret == 0)
   {
      pf_lldp_init(if_id);

      pf_cmdev_exit();     /* Prepare for re-init */
      pf_cmdev_init();

      pf_cmrpc_init(if_id);
   }

   return ret;
}

void pnet_handle_periodic(void)
{
   pf_cmrpc_periodic();
   pf_alarm_periodic();

   /* Handle expired timeout events */
   pf_scheduler_tick();
}

void pnet_show(
   unsigned                level)
{
   pnet_cfg_t              *p_cfg = NULL;

   pf_cmdev_show_device();
   pf_cmrpc_show(level);

   if (level & 0x2000)
   {
      printf("\n");
      pf_cmina_show();
      printf("\n");
   }
   if (level & 0x4000)
   {
      pf_scheduler_show();
      printf("\n");
   }
   if (level & 0x8000)
   {
      pf_fspm_get_cfg(&p_cfg);

      printf("I&M1.im_tag_function     : <%s>\n", p_cfg->im_1_data.im_tag_function);
      printf("I&M1.im_tag_location     : <%s>\n", p_cfg->im_1_data.im_tag_location);
      printf("I&M2.date                : <%s>\n", p_cfg->im_2_data.im_date);
      printf("I&M3.im_descriptor       : <%s>\n", p_cfg->im_3_data.im_descriptor);
      printf("I&M4.im_signature        : <%s>\n", p_cfg->im_4_data.im_signature);    /* Should be binary data, but works for now */
      printf("\n");
   }

}

void pnet_create_log_book_entry(
   uint32_t                   arep,
   const pnet_pnio_status_t   *p_pnio_status,
   uint32_t                   entry_detail)
{
   pf_fspm_create_log_book_entry(arep, p_pnio_status, entry_detail);
}

int pnet_input_set_data_and_iops(
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint8_t                 *p_data,
   uint16_t                data_len,
   uint8_t                 iops)
{
   uint8_t                 iops_len = 1;

   return pf_ppm_set_data_and_iops(api, slot, subslot, p_data, data_len, &iops, iops_len);
}

int pnet_input_get_iocs(
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint8_t                 *p_iocs)
{
   uint8_t                 iocs_len = 1;

   return pf_cpm_get_iocs(api, slot, subslot, p_iocs, &iocs_len);
}

int pnet_output_get_data_and_iops(
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   bool                    *p_new_flag,
   uint8_t                 *p_data,
   uint16_t                *p_data_len,
   uint8_t                 *p_iops)
{
   uint8_t                 iops_len = 1;

   return pf_cpm_get_data_and_iops(api, slot, subslot, p_new_flag, p_data, p_data_len, p_iops, &iops_len);
}

int pnet_output_set_iocs(
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint8_t                 iocs)
{
   uint8_t                 iocs_len = 1;

   return pf_ppm_set_iocs(api, slot, subslot, &iocs, iocs_len);
}

int pnet_plug_module(
   uint32_t                api,
   uint16_t                slot,
   uint32_t                module_ident)
{
   return pf_cmdev_plug_module(api, slot, module_ident);
}

int pnet_plug_submodule(
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint32_t                module_ident,
   uint32_t                submodule_ident,
   pnet_submodule_dir_t    direction,
   uint16_t                length_input,
   uint16_t                length_output)
{
   return pf_cmdev_plug_submodule(api, slot, subslot, module_ident, submodule_ident, direction, length_input, length_output, false);
}

int pnet_pull_module(
   uint32_t                api,
   uint16_t                slot)
{
   return pf_cmdev_pull_module(api, slot);
}

int pnet_pull_submodule(
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot)
{
   return pf_cmdev_pull_submodule(api, slot, subslot);
}

int pnet_set_state(
   bool                    primary)
{
   int                     ret = 0;    /* Assume all goes well */
   uint16_t                ar_ix;
   uint16_t                cr_ix;
   pf_ar_t                 *p_ar = NULL;

   for (ar_ix = 0; ar_ix < PNET_MAX_AR; ar_ix++)
   {
      p_ar = pf_ar_find_by_index(ar_ix);
      if ((p_ar != NULL) && (p_ar->in_use == true))
      {
         for (cr_ix = 0; cr_ix < p_ar->nbr_iocrs; cr_ix++)
         {
            if (pf_ppm_set_data_status_state(p_ar, cr_ix, primary) != 0)
            {
               ret = -1;
            }
         }
      }
   }

   return ret;
}

int pnet_set_redundancy_state(
   bool                    redundant)
{
   int                     ret = 0;    /* Assume all goes well */
   uint16_t                ar_ix;
   uint16_t                cr_ix;
   pf_ar_t                 *p_ar = NULL;

   for (ar_ix = 0; ar_ix < PNET_MAX_AR; ar_ix++)
   {
      p_ar = pf_ar_find_by_index(ar_ix);
      if ((p_ar != NULL) && (p_ar->in_use == true))
      {
         for (cr_ix = 0; cr_ix < p_ar->nbr_iocrs; cr_ix++)
         {
            if (pf_ppm_set_data_status_redundancy(p_ar, cr_ix, redundant) != 0)
            {
               ret = -1;
            }
         }
      }
   }

   return ret;
}

int pnet_set_provider_state(
   bool                    run)
{
   int                     ret = 0;    /* Assume all goes well */
   uint16_t                ar_ix;
   uint16_t                cr_ix;
   pf_ar_t                 *p_ar = NULL;

   for (ar_ix = 0; ar_ix < PNET_MAX_AR; ar_ix++)
   {
      p_ar = pf_ar_find_by_index(ar_ix);
      if ((p_ar != NULL) && (p_ar->in_use == true))
      {
         for (cr_ix = 0; cr_ix < p_ar->nbr_iocrs; cr_ix++)
         {
            if (pf_ppm_set_data_status_provider(p_ar, cr_ix, run) != 0)
            {
               ret = -1;
            }
         }
      }
   }

   return ret;
}

int pnet_application_ready(
   uint32_t                arep)
{
   int                     ret = -1;
   pf_ar_t                 *p_ar = NULL;

   if (pf_ar_find_by_arep(arep, &p_ar) == 0)
   {
      ret = pf_cmdev_cm_ccontrol_req(p_ar);
   }

   return ret;
}

int pnet_ar_abort(
   uint32_t                arep)
{
   int                     ret = -1;
   pf_ar_t                 *p_ar = NULL;

   if (pf_ar_find_by_arep(arep, &p_ar) == 0)
   {
      ret = pf_cmdev_cm_abort(p_ar);
   }

   return ret;
}

int pnet_get_ar_error_codes(
   uint32_t                arep,
   uint16_t                *p_err_cls,
   uint16_t                *p_err_code)
{
   int                     ret = -1;
   pf_ar_t                 *p_ar = NULL;

   if (pf_ar_find_by_arep(arep, &p_ar) == 0)
   {
      *p_err_cls = p_ar->err_cls;
      *p_err_code = p_ar->err_code;

      ret = 0;
   }

   return ret;
}

int pnet_alarm_send_process_alarm(
   uint32_t                arep,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint16_t                payload_len,
   uint16_t                payload_usi,
   uint8_t                 *p_payload)
{
   int                     ret = -1;
   pf_ar_t                 *p_ar = NULL;

   if (pf_ar_find_by_arep(arep, &p_ar) == 0)
   {
      ret = pf_alarm_send_process(p_ar, api, slot, subslot,
         payload_usi, payload_len, p_payload);
   }

   return ret;
}

int pnet_alarm_send_ack(
   uint32_t                arep,
   pnet_pnio_status_t      *p_pnio_status)
{
   int                     ret = -1;
   pf_ar_t                 *p_ar = NULL;

   if (pf_ar_find_by_arep(arep, &p_ar) == 0)
   {
      ret = pf_alarm_alpmr_alarm_ack(p_ar, p_pnio_status);
   }

   return ret;
}

int pnet_diag_add(
   uint32_t                arep,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint16_t                ch,
   uint16_t                ch_properties,
   uint16_t                ch_error_type,
   uint16_t                ext_ch_error_type,
   uint32_t                ext_ch_add_value,
   uint32_t                qual_ch_qualifier,
   uint16_t                usi,
   uint8_t                 *p_manuf_data)
{
   int                     ret = -1;
   pf_ar_t                 *p_ar = NULL;

   if (pf_ar_find_by_arep(arep, &p_ar) == 0)
   {
      ret = pf_diag_add(p_ar, api, slot, subslot, ch, ch_properties, ch_error_type, ext_ch_error_type, ext_ch_add_value,
         qual_ch_qualifier, usi, p_manuf_data);
   }

   return ret;
}

int pnet_diag_update(
   uint32_t                arep,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint16_t                ch,
   uint16_t                ch_properties,
   uint16_t                ch_error_type,
   uint32_t                ext_ch_add_value,
   uint16_t                usi,
   uint8_t                 *p_manuf_data)
{
   int                     ret = -1;
   pf_ar_t                 *p_ar = NULL;

   if (pf_ar_find_by_arep(arep, &p_ar) == 0)
   {
      ret = pf_diag_update(p_ar, api, slot, subslot, ch, ch_properties, ch_error_type, ext_ch_add_value, usi, p_manuf_data);
   }

   return ret;
}

int pnet_diag_remove(
   uint32_t                arep,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint16_t                ch,
   uint16_t                ch_properties,
   uint16_t                ch_error_type,
   uint16_t                usi)
{
   int                     ret = -1;
   pf_ar_t                 *p_ar = NULL;

   if (pf_ar_find_by_arep(arep, &p_ar) == 0)
   {
      ret = pf_diag_remove(p_ar, api, slot, subslot, ch, ch_properties, ch_error_type, usi);
   }

   return ret;
}
