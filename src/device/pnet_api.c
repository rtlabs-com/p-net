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
#define pnal_eth_init  mock_pnal_eth_init
#define pnal_snmp_init mock_pnal_snmp_init
#endif

#include <stdlib.h>
#include <string.h>

#include "pf_includes.h"
#include "pf_block_reader.h"

int pnet_init_only (
   pnet_t * net,
   const char * netif,
   uint32_t tick_us,
   const pnet_cfg_t * p_cfg)
{
   memset (net, 0, sizeof (*net));

   if (strlen (netif) >= PNET_INTERFACE_NAME_MAX_SIZE)
   {
      LOG_ERROR (
         PNET_LOG,
         "Too long interface name. Given: %s  Max size incl termination: %d\n",
         netif,
         PNET_INTERFACE_NAME_MAX_SIZE);
      return -1;
   }
   strcpy (net->interface_name, netif);

   net->cmdev_initialized = false; /* TODO How to handle that pf_cmdev_exit() is
                                      used before pf_cmdev_init()? */

   pf_cmsu_init (net);
   pf_cmwrr_init (net);
   pf_cpm_init (net);
   pf_ppm_init (net);
   pf_alarm_init (net);

   /* initialize configuration */
   if (pf_fspm_init (net, p_cfg) != 0)
   {
      return -1;
   }

   /* Initialize everything (and the DCP protocol) */
   /* First initialize the network interface */
   net->eth_handle = pnal_eth_init (netif, pf_eth_recv, (void *)net);
   if (net->eth_handle == NULL)
   {
      return -1;
   }

   pf_eth_init (net);
   pf_scheduler_init (net, tick_us);
   pf_cmina_init (net); /* Read from permanent pool */

   pf_dcp_exit (net); /* Prepare for re-init. */
   pf_dcp_init (net); /* Start DCP */
   pf_port_init (net);
   pf_lldp_init (net);
   pf_pdport_init (net);

   /* Configure SNMP server if enabled */
#if PNET_OPTION_SNMP == 1
   if (pnal_snmp_init (net) != 0)
   {
      LOG_ERROR (PNET_LOG, "Failed to configure SNMP\n");
      return -1;
   }
#endif

   pf_cmdev_exit (net); /* Prepare for re-init */
   pf_cmdev_init (net);

   pf_cmrpc_init (net);

   return 0;
}

pnet_t * pnet_init (
   const char * netif,
   uint32_t tick_us,
   const pnet_cfg_t * p_cfg)
{
   pnet_t * net = NULL;

   net = os_malloc (sizeof (*net));
   if (net == NULL)
   {
      LOG_ERROR (
         PNET_LOG,
         "Failed to allocate memory for pnet_t (%zu bytes)\n",
         sizeof (*net));
      return NULL;
   }

   if (pnet_init_only (net, netif, tick_us, p_cfg) != 0)
   {
      free (net);
      return NULL;
   }

   return net;
}

void pnet_handle_periodic (pnet_t * net)
{
   pf_cmrpc_periodic (net);
   pf_alarm_periodic (net);

   /* Handle expired timeout events */
   pf_scheduler_tick (net);

   pf_pdport_periodic (net);
}

void pnet_show (pnet_t * net, unsigned level)
{
   if (net != NULL)
   {
      if (level & 0x0010)
      {
         pf_fspm_option_show (net);
      }

      if (level & 0x0020)
      {
         pf_cmdev_device_show (net);
      }

      pf_cmrpc_show (net, level);

      if (level & 0x0200)
      {
         pf_cmdev_diag_show (net);
      }

      if (level & 0x0400)
      {
         pf_fspm_logbook_show (net);
      }

      if (level & 0x2000)
      {
         printf ("\n\n");
         pf_cmina_show (net);
      }
      if (level & 0x4000)
      {
         printf ("\n\n");
         pf_scheduler_show (net);
      }
      if (level & 0x8000)
      {
         printf ("\n\n");
         pf_fspm_im_show (net);
      }
   }
   else
   {
      printf ("p-net not yet initialized.\n");
   }
}

void pnet_create_log_book_entry (
   pnet_t * net,
   uint32_t arep,
   const pnet_pnio_status_t * p_pnio_status,
   uint32_t entry_detail)
{
   pf_fspm_create_log_book_entry (net, arep, p_pnio_status, entry_detail);
}

int pnet_input_set_data_and_iops (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   const uint8_t * p_data,
   uint16_t data_len,
   uint8_t iops)
{
   uint8_t iops_len = 1;

   return pf_ppm_set_data_and_iops (
      net,
      api,
      slot,
      subslot,
      p_data,
      data_len,
      &iops,
      iops_len);
}

int pnet_input_get_iocs (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint8_t * p_iocs)
{
   uint8_t iocs_len = 1;

   return pf_cpm_get_iocs (net, api, slot, subslot, p_iocs, &iocs_len);
}

int pnet_output_get_data_and_iops (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   bool * p_new_flag,
   uint8_t * p_data,
   uint16_t * p_data_len,
   uint8_t * p_iops)
{
   uint8_t iops_len = 1;

   return pf_cpm_get_data_and_iops (
      net,
      api,
      slot,
      subslot,
      p_new_flag,
      p_data,
      p_data_len,
      p_iops,
      &iops_len);
}

int pnet_output_set_iocs (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint8_t iocs)
{
   uint8_t iocs_len = 1;

   return pf_ppm_set_iocs (net, api, slot, subslot, &iocs, iocs_len);
}

int pnet_plug_module (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint32_t module_ident)
{
   return pf_cmdev_plug_module (net, api, slot, module_ident);
}

int pnet_plug_submodule (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint32_t module_ident,
   uint32_t submodule_ident,
   pnet_submodule_dir_t direction,
   uint16_t length_input,
   uint16_t length_output)
{
   return pf_cmdev_plug_submodule (
      net,
      api,
      slot,
      subslot,
      module_ident,
      submodule_ident,
      direction,
      length_input,
      length_output,
      false);
}

int pnet_pull_module (pnet_t * net, uint32_t api, uint16_t slot)
{
   return pf_cmdev_pull_module (net, api, slot);
}

int pnet_pull_submodule (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot)
{
   return pf_cmdev_pull_submodule (net, api, slot, subslot);
}

int pnet_set_primary_state (pnet_t * net, bool primary)
{
   int ret = 0; /* Assume all goes well */
   uint16_t ar_ix;
   uint16_t cr_ix;
   pf_ar_t * p_ar = NULL;

   for (ar_ix = 0; ar_ix < PNET_MAX_AR; ar_ix++)
   {
      p_ar = pf_ar_find_by_index (net, ar_ix);
      if ((p_ar != NULL) && (p_ar->in_use == true))
      {
         for (cr_ix = 0; cr_ix < p_ar->nbr_iocrs; cr_ix++)
         {
            if (pf_ppm_set_data_status_state (p_ar, cr_ix, primary) != 0)
            {
               ret = -1;
            }
         }
      }
   }

   return ret;
}

int pnet_set_redundancy_state (pnet_t * net, bool redundant)
{
   int ret = 0; /* Assume all goes well */
   uint16_t ar_ix;
   uint16_t cr_ix;
   pf_ar_t * p_ar = NULL;

   for (ar_ix = 0; ar_ix < PNET_MAX_AR; ar_ix++)
   {
      p_ar = pf_ar_find_by_index (net, ar_ix);
      if ((p_ar != NULL) && (p_ar->in_use == true))
      {
         for (cr_ix = 0; cr_ix < p_ar->nbr_iocrs; cr_ix++)
         {
            if (pf_ppm_set_data_status_redundancy (p_ar, cr_ix, redundant) != 0)
            {
               ret = -1;
            }
         }
      }
   }

   return ret;
}

int pnet_set_provider_state (pnet_t * net, bool run)
{
   int ret = 0; /* Assume all goes well */
   uint16_t ar_ix;
   uint16_t cr_ix;
   pf_ar_t * p_ar = NULL;

   for (ar_ix = 0; ar_ix < PNET_MAX_AR; ar_ix++)
   {
      p_ar = pf_ar_find_by_index (net, ar_ix);
      if ((p_ar != NULL) && (p_ar->in_use == true))
      {
         for (cr_ix = 0; cr_ix < p_ar->nbr_iocrs; cr_ix++)
         {
            if (pf_ppm_set_data_status_provider (p_ar, cr_ix, run) != 0)
            {
               ret = -1;
            }
         }
      }
   }

   return ret;
}

int pnet_application_ready (pnet_t * net, uint32_t arep)
{
   int ret = -1;
   pf_ar_t * p_ar = NULL;

   if (pf_ar_find_by_arep (net, arep, &p_ar) == 0)
   {
      ret = pf_cmdev_cm_ccontrol_req (net, p_ar);
   }

   return ret;
}

int pnet_ar_abort (pnet_t * net, uint32_t arep)
{
   int ret = -1;
   pf_ar_t * p_ar = NULL;

   if (pf_ar_find_by_arep (net, arep, &p_ar) == 0)
   {
      ret = pf_cmdev_cm_abort (net, p_ar);
   }

   return ret;
}

int pnet_factory_reset (pnet_t * net)
{
   uint16_t ix;
   pf_ar_t * p_ar = NULL;

   /* Look for active connections */
   for (ix = 0; ix < PNET_MAX_AR; ix++)
   {
      p_ar = pf_ar_find_by_index (net, ix);
      if ((p_ar != NULL) && (p_ar->in_use == true))
      {
         (void)pf_cmdev_cm_abort (net, p_ar);
      }
   }

   (void)pf_cmina_set_default_cfg (net, 99);
   pf_cmina_dcp_set_commit (net);

   return 0;
}

int pnet_remove_data_files (const char * file_directory)
{
   return pf_cmina_remove_all_data_files (file_directory);
}

int pnet_get_ar_error_codes (
   pnet_t * net,
   uint32_t arep,
   uint16_t * p_err_cls,
   uint16_t * p_err_code)
{
   int ret = -1;
   pf_ar_t * p_ar = NULL;

   if (pf_ar_find_by_arep (net, arep, &p_ar) == 0)
   {
      *p_err_cls = p_ar->err_cls;
      *p_err_code = p_ar->err_code;

      ret = 0;
   }

   return ret;
}

int pnet_alarm_send_process_alarm (
   pnet_t * net,
   uint32_t arep,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint16_t payload_usi,
   uint16_t payload_len,
   const uint8_t * p_payload)
{
   int ret = -1;
   pf_ar_t * p_ar = NULL;

   if (pf_ar_find_by_arep (net, arep, &p_ar) == 0)
   {
      ret = pf_alarm_send_process (
         net,
         p_ar,
         api,
         slot,
         subslot,
         payload_usi,
         payload_len,
         p_payload);
   }

   return ret;
}

int pnet_alarm_send_ack (
   pnet_t * net,
   uint32_t arep,
   const pnet_alarm_argument_t * p_alarm_argument,
   const pnet_pnio_status_t * p_pnio_status)
{
   int ret = -1;
   pf_ar_t * p_ar = NULL;

   if (pf_ar_find_by_arep (net, arep, &p_ar) == 0)
   {
      ret =
         pf_alarm_alpmr_alarm_ack (net, p_ar, p_alarm_argument, p_pnio_status);
   }

   return ret;
}

/************************** Diagnosis in standard format *********************/

int pnet_diag_std_add (
   pnet_t * net,
   const pnet_diag_source_t * p_diag_source,
   pnet_diag_ch_prop_type_values_t ch_bits,
   pnet_diag_ch_prop_maint_values_t severity,
   uint16_t ch_error_type,
   uint16_t ext_ch_error_type,
   uint32_t ext_ch_add_value,
   uint32_t qual_ch_qualifier)
{
   return pf_diag_std_add (
      net,
      p_diag_source,
      ch_bits,
      severity,
      ch_error_type,
      ext_ch_error_type,
      ext_ch_add_value,
      qual_ch_qualifier);
}

int pnet_diag_std_update (
   pnet_t * net,
   const pnet_diag_source_t * p_diag_source,
   uint16_t ch_error_type,
   uint16_t ext_ch_error_type,
   uint32_t ext_ch_add_value)
{
   return pf_diag_std_update (
      net,
      p_diag_source,
      ch_error_type,
      ext_ch_error_type,
      ext_ch_add_value);
}

int pnet_diag_std_remove (
   pnet_t * net,
   const pnet_diag_source_t * p_diag_source,
   uint16_t ch_error_type,
   uint16_t ext_ch_error_type)
{
   return pf_diag_std_remove (
      net,
      p_diag_source,
      ch_error_type,
      ext_ch_error_type);
}

/************************** Diagnosis in USI format **************************/

int pnet_diag_usi_add (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint16_t usi,
   const uint8_t * p_manuf_data)
{
   return pf_diag_usi_add (net, api, slot, subslot, usi, p_manuf_data);
}

int pnet_diag_usi_update (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint16_t usi,
   const uint8_t * p_manuf_data)
{
   return pf_diag_usi_update (net, api, slot, subslot, usi, p_manuf_data);
}

int pnet_diag_usi_remove (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint16_t usi)
{
   return pf_diag_usi_remove (net, api, slot, subslot, usi);
}
