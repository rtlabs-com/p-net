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
 * @brief Implements the Fieldbus Application Layer Service Protocol Machine
 * (FSPM)
 *
 * Stores the user-defined configuration, and calls the user-defined callbacks.
 * Create logbook entries.
 * Reads and writes identification & maintenance (I&M) records.
 */

#ifdef UNIT_TEST
#define os_get_current_time_us   mock_os_get_current_time_us
#define pf_file_clear            mock_pf_file_clear
#define pf_file_load             mock_pf_file_load
#define pf_file_save_if_modified mock_pf_file_save_if_modified
#define pf_bg_worker_start_job   mock_pf_bg_worker_start_job
#endif

#include <string.h>
#include <inttypes.h>

#include "pf_includes.h"
#include "pf_block_reader.h"

void pf_fspm_logbook_show (const pnet_t * net)
{
   uint16_t ix;

   printf ("Logbook. Has wrapped: %u\n", net->fspm_log_book.wrap);
   printf ("ix   Next   Error code            Detail       Timestamp (s)\n");

   for (ix = 0; ix < NELEMENTS (net->fspm_log_book.entries); ix++)
   {
      printf ("[%02u] %s      ", ix, (ix == net->fspm_log_book.put) ? "*" : " ");

      if (
         (net->fspm_log_book.entries[ix].ar_uuid.time_low != 0) ||
         (net->fspm_log_book.entries[ix].ar_uuid.time_mid != 0) ||
         (net->fspm_log_book.entries[ix].ar_uuid.time_hi_and_version != 0))
      {
         printf (
            "0x%02X 0x%02X 0x%02X 0x%02X   0x%08" PRIX32 "   %" PRIu32 "\n",
            net->fspm_log_book.entries[ix].pnio_status.error_code,
            net->fspm_log_book.entries[ix].pnio_status.error_decode,
            net->fspm_log_book.entries[ix].pnio_status.error_code_1,
            net->fspm_log_book.entries[ix].pnio_status.error_code_2,
            net->fspm_log_book.entries[ix].entry_detail,
            net->fspm_log_book.entries[ix].time_ts.sec_lo);
      }
      else
      {
         printf ("\n");
      }
   }
}

void pf_fspm_option_show (const pnet_t * net)
{
   printf ("Compile time options affecting memory consumption:\n");
   printf (
      "PNET_MAX_IO_AR                                 : %d\n",
      PNET_MAX_IO_AR);
   printf (
      "PNET_MAX_DA_AR                                 : %d\n",
      PNET_MAX_DA_AR);
   printf (
      "PNET_MAX_API                                   : %d\n",
      PNET_MAX_API);
   printf ("PNET_MAX_CR                                    : %d\n", PNET_MAX_CR);
   printf (
      "PNET_MAX_SLOTS                                 : %d\n",
      PNET_MAX_SLOTS);
   printf (
      "PNET_MAX_SUBSLOTS                              : %d\n",
      PNET_MAX_SUBSLOTS);
   printf (
      "PNET_MAX_DFP_IOCR                              : %d\n",
      PNET_MAX_DFP_IOCR);
   printf (
      "PNET_MAX_PHYSICAL_PORTS                        : %d\n",
      PNET_MAX_PHYSICAL_PORTS);
   printf (
      "PNET_MAX_LOG_BOOK_ENTRIES                      : %d\n",
      PNET_MAX_LOG_BOOK_ENTRIES);
   printf (
      "PNET_MAX_ALARMS                                : %d\n",
      PNET_MAX_ALARMS);
   printf (
      "PNET_MAX_ALARM_PAYLOAD_DATA_SIZE               : %d\n",
      PNET_MAX_ALARM_PAYLOAD_DATA_SIZE);
   printf (
      "PNET_MAX_DIAG_ITEMS                            : %d\n",
      PNET_MAX_DIAG_ITEMS);
   printf (
      "PNET_MAX_DIAG_MANUF_DATA_SIZE                  : %d\n",
      PNET_MAX_DIAG_MANUF_DATA_SIZE);
   printf (
      "PNET_MAX_DIRECTORYPATH_SIZE                    : %d\n",
      PNET_MAX_DIRECTORYPATH_SIZE);
   printf (
      "PNET_MAX_FILENAME_SIZE                         : %d\n",
      PNET_MAX_FILENAME_SIZE);
   printf (
      "PNET_MAX_SESSION_BUFFER_SIZE                   : %d\n",
      PNET_MAX_SESSION_BUFFER_SIZE);
   printf (
      "PNET_OPTION_MC_CR                              : %d\n",
      PNET_OPTION_MC_CR);
#if PNET_OPTION_MC_CR
   printf (
      "PNET_MAX_MC_CR                                 : %d\n",
      PNET_MAX_MC_CR);
#endif
   printf (
      "PNET_OPTION_AR_VENDOR_BLOCKS                   : %d\n",
      PNET_OPTION_AR_VENDOR_BLOCKS);
#if PNET_OPTION_AR_VENDOR_BLOCKS
   printf (
      "PNET_MAX_AR_VENDOR_BLOCKS                      : %d\n",
      PNET_MAX_AR_VENDOR_BLOCKS);
   printf (
      "PNET_MAX_AR_VENDOR_BLOCK_DATA_LENGTH           : %d\n",
      PNET_MAX_AR_VENDOR_BLOCK_DATA_LENGTH);
#endif
   printf (
      "PNET_OPTION_PARAMETER_SERVER                   : %d\n",
      PNET_OPTION_PARAMETER_SERVER);
   printf (
      "PNET_OPTION_IR                                 : %d\n",
      PNET_OPTION_IR);
   printf (
      "PNET_OPTION_SR                                 : %d\n",
      PNET_OPTION_SR);
   printf (
      "PNET_OPTION_REDUNDANCY                         : %d\n",
      PNET_OPTION_REDUNDANCY);
   printf (
      "PNET_OPTION_SNMP                               : %d\n",
      PNET_OPTION_SNMP);
   printf ("LOG_LEVEL (DEBUG=0, FATAL=4)                   : %d\n", LOG_LEVEL);
   printf (
      "sizeof(pnet_t) : %" PRIu32 " bytes (for currently selected option "
      "values)\n",
      (uint32_t)sizeof (pnet_t));
}

/**
 * @internal
 * Validate the configuration from the user.
 *
 * @param p_cfg            In:    Configuration object
 * @return  0  if the configuration is valid.
 *          -1 if the configuration is invalid.
 */
int pf_fspm_validate_configuration (const pnet_cfg_t * p_cfg)
{
   if (p_cfg == NULL)
   {
      LOG_ERROR (
         PNET_LOG,
         "FSPM(%d): You must provide a configuration, but it is NULL.\n",
         __LINE__);
      return -1;
   }

   if (p_cfg->tick_us == 0)
   {
      LOG_ERROR (
         PNET_LOG,
         "FSPM(%d): Configured tick interval must not be 0. By default, sample "
         "application uses 1000 micro seconds tick interval\n",
         __LINE__);
      return -1;
   }

   if (strlen (p_cfg->if_cfg.main_netif_name) == 0)
   {
      LOG_ERROR (
         PNET_LOG,
         "FSPM(%d): Length of network interface name must not be 0.\n",
         __LINE__);
      return -1;
   }

   if (
      p_cfg->num_physical_ports == 0 ||
      p_cfg->num_physical_ports > PNET_MAX_PHYSICAL_PORTS)
   {
      LOG_ERROR (
         PNET_LOG,
         "FSPM(%d): The number of ports setting is wrong. Given: %d  "
         "PNET_MAX_PHYSICAL_PORTS: %d\n",
         __LINE__,
         p_cfg->num_physical_ports,
         PNET_MAX_PHYSICAL_PORTS);
      return -1;
   }

   if (p_cfg->min_device_interval == 0)
   {
      LOG_ERROR (
         PNET_LOG,
         "FSPM(%d): The min_device_interval in the config must not be 0. It "
         "should typically be 32, corresponding to 1 ms.\n",
         __LINE__);
      return -1;
   }
   else if (p_cfg->min_device_interval > 0x1000)
   {
      LOG_ERROR (
         PNET_LOG,
         "FSPM(%d): The min_device_interval in the config is too large. Max is "
         "4096, corresponding to 128 ms.\n",
         __LINE__);
      return -1;
   }

   return 0;
}

int pf_fspm_init (pnet_t * net, const pnet_cfg_t * p_cfg)
{
   if (pf_fspm_validate_configuration (p_cfg) != 0)
   {
      return -1;
   }

   /* Use a copy of the configuration. For example the I&M data might be updated
    * at runtime. */
   net->fspm_cfg = *p_cfg;

   /* Reference to the default settings (used at factory reset) */
   net->p_fspm_default_cfg = p_cfg;

   /* Log book */
   if (net->fspm_log_book_mutex == NULL)
   {
      net->fspm_log_book_mutex = os_mutex_create();
   }

   /* Turn LED off */
   if (pf_fspm_signal_led_ind (net, false) != 0)
   {
      LOG_ERROR (
         PNET_LOG,
         "FSPM(%d): Could not turn signal LED off\n",
         __LINE__);
   }

   return 0;
}

void pf_fspm_create_log_book_entry (
   pnet_t * net,
   uint32_t arep,
   const pnet_pnio_status_t * p_pnio_status,
   uint32_t entry_detail)
{
   uint16_t put;
   uint32_t time = 0;
   pf_ar_t * p_ar = NULL;
   pf_uuid_t * p_ar_uuid = NULL;

   if (pf_ar_find_by_arep (net, arep, &p_ar) == 0)
   {
      p_ar_uuid = &p_ar->ar_param.ar_uuid;
      os_mutex_lock (net->fspm_log_book_mutex);
      put = net->fspm_log_book.put;
      net->fspm_log_book.put++;
      if (net->fspm_log_book.put >= NELEMENTS (net->fspm_log_book.entries))
      {
         net->fspm_log_book.put = 0;
         net->fspm_log_book.wrap = true;
      }

      time = os_get_current_time_us();
      net->fspm_log_book.entries[put].time_ts.status = PF_TS_STATUS_LOCAL_ARB;
      net->fspm_log_book.entries[put].time_ts.sec_hi = 0;
      net->fspm_log_book.entries[put].time_ts.sec_lo = time / 1000000;
      net->fspm_log_book.entries[put].time_ts.nano_sec =
         (time % 1000000) * 1000;

      net->fspm_log_book.entries[put].ar_uuid = *p_ar_uuid;
      net->fspm_log_book.entries[put].pnio_status = *p_pnio_status;
      net->fspm_log_book.entries[put].entry_detail = entry_detail;
      os_mutex_unlock (net->fspm_log_book_mutex);

      LOG_DEBUG (
         PNET_LOG,
         "FSPM(%d): Added logbook entry to position %u. Error: "
         "0x%02X 0x%02X 0x%02X 0x%02X Detail: %" PRIu32 "\n",
         __LINE__,
         put,
         p_pnio_status->error_code,
         p_pnio_status->error_decode,
         p_pnio_status->error_code_1,
         p_pnio_status->error_code_2,
         entry_detail);
   }
}

void pf_fspm_get_cfg (pnet_t * net, pnet_cfg_t ** pp_cfg)
{
   if (pp_cfg != NULL)
   {
      *pp_cfg = &net->fspm_cfg;
   }
}

void pf_fspm_get_default_cfg (pnet_t * net, const pnet_cfg_t ** pp_cfg)
{
   if (pp_cfg != NULL)
   {
      *pp_cfg = net->p_fspm_default_cfg;
   }
}

int16_t pf_fspm_get_min_device_interval (const pnet_t * net)
{
   return net->fspm_cfg.min_device_interval;
}

/******************* Execute user callbacks *********************************/

int pf_fspm_exp_module_ind (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint32_t module_ident)
{
   int ret = -1;

   if (net->fspm_cfg.exp_module_cb != NULL)
   {
      ret =
         net->fspm_cfg
            .exp_module_cb (net, net->fspm_cfg.cb_arg, api, slot, module_ident);
   }

   return ret;
}

int pf_fspm_exp_submodule_ind (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint32_t module_ident,
   uint32_t submodule_ident,
   const pnet_data_cfg_t * p_exp_data)
{
   int ret = -1;

   if (net->fspm_cfg.exp_submodule_cb != NULL)
   {
      ret = net->fspm_cfg.exp_submodule_cb (
         net,
         net->fspm_cfg.cb_arg,
         api,
         slot,
         subslot,
         module_ident,
         submodule_ident,
         p_exp_data);
   }

   return ret;
}

bool pf_fspm_is_substitute_req (
   pnet_t * net,
   uint32_t expected_ident,
   uint32_t submodule_ident)
{
   bool ret = false;

   if (net->fspm_cfg.is_substitute_cb != NULL)
   {
      ret = net->fspm_cfg.is_substitute_cb (
         net,
         net->fspm_cfg.cb_arg,
         expected_ident,
         submodule_ident);
   }

   return ret;
}

void pf_fspm_data_status_changed (
   pnet_t * net,
   const pf_ar_t * p_ar,
   const pf_iocr_t * p_iocr,
   uint8_t changes,
   uint8_t data_status)
{
   if (net->fspm_cfg.new_data_status_cb != NULL)
   {
      LOG_DEBUG (
         PNET_LOG,
         "FSPM(%d): Triggering data status change callback for AREP %u. Status "
         "0x%02x changes 0x%02x\n",
         __LINE__,
         p_ar->arep,
         data_status,
         changes);

      (void)net->fspm_cfg.new_data_status_cb (
         net,
         net->fspm_cfg.cb_arg,
         p_ar->arep,
         p_iocr->crep,
         changes,
         data_status);
   }
}

void pf_fspm_ccontrol_cnf (
   pnet_t * net,
   const pf_ar_t * p_ar,
   pnet_result_t * p_result)
{
   if (net->fspm_cfg.ccontrol_cb != NULL)
   {
      LOG_DEBUG (
         PNET_LOG,
         "FSPM(%d): Triggering CControl confirmation callback for AREP %u\n",
         __LINE__,
         p_ar->arep);

      (void)net->fspm_cfg
         .ccontrol_cb (net, net->fspm_cfg.cb_arg, p_ar->arep, p_result);
   }
}

int pf_fspm_cm_read_ind (
   pnet_t * net,
   const pf_ar_t * p_ar,
   const pf_iod_read_request_t * p_read_request,
   uint8_t ** pp_read_data,
   uint16_t * p_read_length,
   pnet_result_t * p_read_status)
{
   int ret = -1;
   uint16_t slot = p_read_request->slot_number;
   uint16_t subslot = p_read_request->subslot_number;
   uint16_t index = p_read_request->index;

   if (pf_index_is_user_specific (p_read_request->index))
   {
      /* Trigger callback for application-specific data records */
      if (net->fspm_cfg.read_cb != NULL)
      {
         LOG_DEBUG (
            PNET_LOG,
            "FSPM(%d): Triggering read callback for AREP %" PRIu32
            ". Slot %u subslot %u index %u\n",
            __LINE__,
            ((p_ar == NULL) ? UINT32_MAX : p_ar->arep),
            slot,
            subslot,
            index);

         ret = net->fspm_cfg.read_cb (
            net,
            net->fspm_cfg.cb_arg,
            ((p_ar == NULL) ? UINT32_MAX : p_ar->arep),
            p_read_request->api,
            slot,
            subslot,
            index,
            p_read_request->sequence_number,
            pp_read_data,
            p_read_length,
            p_read_status);
      }
      else
      {
         LOG_DEBUG (
            PNET_LOG,
            "FSPM(%d): No user callback for reading records implemented.\n",
            __LINE__);

         p_read_status->pnio_status.error_code = PNET_ERROR_CODE_READ;
         p_read_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
         p_read_status->pnio_status.error_code_1 =
            PNET_ERROR_CODE_1_APP_NOT_SUPPORTED;
         p_read_status->pnio_status.error_code_2 = PNET_ERROR_CODE_2_PNIORW (0);
      }
   }
   else
   {
      /* Nothing if data not available here */
   }

   return ret;
}

int pf_fspm_read_im_0 (
   pnet_t * net,
   pf_iod_read_request_t const * request,
   pnet_im_0_t * im0,
   pnet_result_t * result)
{
   if (net->fspm_cfg.read_im_0_cb == NULL)
   {
      pf_set_error (
         result,
         PNET_ERROR_CODE_READ,
         PNET_ERROR_DECODE_PNIORW,
         PNET_ERROR_CODE_1_ACC_INVALID_INDEX,
         PNET_ERROR_CODE_2_PNIORW (10));
      return -1;
   }
   return net->fspm_cfg.read_im_0_cb (
      net,
      net->fspm_cfg.cb_arg,
      request->api,
      request->slot_number,
      request->subslot_number,
      im0,
      result);
}

int pf_fspm_read_im_1 (
   pnet_t * net,
   pf_iod_read_request_t const * request,
   pnet_im_1_t * im1,
   pnet_result_t * result)
{
   if (net->fspm_cfg.read_im_1_cb == NULL)
   {
      pf_set_error (
         result,
         PNET_ERROR_CODE_READ,
         PNET_ERROR_DECODE_PNIORW,
         PNET_ERROR_CODE_1_ACC_INVALID_INDEX,
         PNET_ERROR_CODE_2_PNIORW (10));
      return -1;
   }
   return net->fspm_cfg.read_im_1_cb (
      net,
      net->fspm_cfg.cb_arg,
      request->api,
      request->slot_number,
      request->subslot_number,
      im1,
      result);
}

int pf_fspm_read_im_2 (
   pnet_t * net,
   pf_iod_read_request_t const * request,
   pnet_im_2_t * im2,
   pnet_result_t * result)
{
   if (net->fspm_cfg.read_im_2_cb == NULL)
   {
      pf_set_error (
         result,
         PNET_ERROR_CODE_READ,
         PNET_ERROR_DECODE_PNIORW,
         PNET_ERROR_CODE_1_ACC_INVALID_INDEX,
         PNET_ERROR_CODE_2_PNIORW (10));
      return -1;
   }
   return net->fspm_cfg.read_im_2_cb (
      net,
      net->fspm_cfg.cb_arg,
      request->api,
      request->slot_number,
      request->subslot_number,
      im2,
      result);
}

int pf_fspm_read_im_3 (
   pnet_t * net,
   pf_iod_read_request_t const * request,
   pnet_im_3_t * im3,
   pnet_result_t * result)
{
   if (net->fspm_cfg.read_im_3_cb == NULL)
   {
      pf_set_error (
         result,
         PNET_ERROR_CODE_READ,
         PNET_ERROR_DECODE_PNIORW,
         PNET_ERROR_CODE_1_ACC_INVALID_INDEX,
         PNET_ERROR_CODE_2_PNIORW (10));
      return -1;
   }
   return net->fspm_cfg.read_im_3_cb (
      net,
      net->fspm_cfg.cb_arg,
      request->api,
      request->slot_number,
      request->subslot_number,
      im3,
      result);
}

int pf_fspm_read_im_4 (
   pnet_t * net,
   pf_iod_read_request_t const * request,
   pnet_im_4_t * im4,
   pnet_result_t * result)
{
   if (net->fspm_cfg.read_im_4_cb == NULL)
   {
      pf_set_error (
         result,
         PNET_ERROR_CODE_READ,
         PNET_ERROR_DECODE_PNIORW,
         PNET_ERROR_CODE_1_ACC_INVALID_INDEX,
         PNET_ERROR_CODE_2_PNIORW (10));
      return -1;
   }
   return net->fspm_cfg.read_im_4_cb (
      net,
      net->fspm_cfg.cb_arg,
      request->api,
      request->slot_number,
      request->subslot_number,
      im4,
      result);
}

int pf_fspm_cm_write_ind (
   pnet_t * net,
   const pf_ar_t * p_ar,
   const pf_iod_write_request_t * p_write_request,
   uint16_t write_length,
   const uint8_t * p_write_data,
   pnet_result_t * p_write_status)
{
   int ret = -1;

   if (pf_index_is_user_specific (p_write_request->index))
   {
      /* Trigger callback for application-specific data records */
      if (net->fspm_cfg.write_cb != NULL)
      {
         LOG_DEBUG (
            PNET_LOG,
            "FSPM(%d): Triggering write callback for AREP %u. Slot %u subslot "
            "%u index %u len %u\n",
            __LINE__,
            p_ar->arep,
            p_write_request->slot_number,
            p_write_request->subslot_number,
            p_write_request->index,
            write_length);

         ret = net->fspm_cfg.write_cb (
            net,
            net->fspm_cfg.cb_arg,
            p_ar->arep,
            p_write_request->api,
            p_write_request->slot_number,
            p_write_request->subslot_number,
            p_write_request->index,
            pf_cmdev_prm_flag (
               p_ar,
               p_write_request->api,
               p_write_request->slot_number,
               p_write_request->subslot_number),
            p_write_request->sequence_number,
            write_length,
            p_write_data,
            p_write_status);
      }
      else
      {
         LOG_DEBUG (
            PNET_LOG,
            "FSPM(%d): No user callback for writing records implemented.\n",
            __LINE__);
         pf_set_error (
            p_write_status,
            PNET_ERROR_CODE_WRITE,
            PNET_ERROR_DECODE_PNIORW,
            PNET_ERROR_CODE_1_APP_NOT_SUPPORTED,
            PNET_ERROR_CODE_2_PNIORW (0));
      }
   }
   else
   {
      /* Nothing if write not possible here */
   }

   return ret;
}

int pf_fspm_write_im_1 (
   pnet_t * net,
   pf_iod_write_request_t const * request,
   bool prm_flag,
   pnet_im_1_t const * im_1,
   pnet_result_t * result)
{
   if (net->fspm_cfg.write_im_1_cb == NULL)
   {
      pf_set_error (
         result,
         PNET_ERROR_CODE_WRITE,
         PNET_ERROR_DECODE_PNIORW,
         PNET_ERROR_CODE_1_ACC_INVALID_INDEX,
         PNET_ERROR_CODE_2_PNIORW (10));
      return -1;
   }
   return net->fspm_cfg.write_im_1_cb (
      net,
      net->fspm_cfg.cb_arg,
      request->api,
      request->slot_number,
      request->subslot_number,
      prm_flag,
      im_1,
      result);
}

int pf_fspm_write_im_2 (
   pnet_t * net,
   pf_iod_write_request_t const * request,
   bool prm_flag,
   pnet_im_2_t const * im_2,
   pnet_result_t * result)
{
   if (net->fspm_cfg.write_im_2_cb == NULL)
   {
      pf_set_error (
         result,
         PNET_ERROR_CODE_WRITE,
         PNET_ERROR_DECODE_PNIORW,
         PNET_ERROR_CODE_1_ACC_INVALID_INDEX,
         PNET_ERROR_CODE_2_PNIORW (10));
      return -1;
   }
   return net->fspm_cfg.write_im_2_cb (
      net,
      net->fspm_cfg.cb_arg,
      request->api,
      request->slot_number,
      request->subslot_number,
      prm_flag,
      im_2,
      result);
}

int pf_fspm_write_im_3 (
   pnet_t * net,
   pf_iod_write_request_t const * request,
   bool prm_flag,
   pnet_im_3_t const * im_3,
   pnet_result_t * result)
{
   if (net->fspm_cfg.write_im_3_cb == NULL)
   {
      pf_set_error (
         result,
         PNET_ERROR_CODE_WRITE,
         PNET_ERROR_DECODE_PNIORW,
         PNET_ERROR_CODE_1_ACC_INVALID_INDEX,
         PNET_ERROR_CODE_2_PNIORW (10));
      return -1;
   }
   return net->fspm_cfg.write_im_3_cb (
      net,
      net->fspm_cfg.cb_arg,
      request->api,
      request->slot_number,
      request->subslot_number,
      prm_flag,
      im_3,
      result);
}

int pf_fspm_write_im_4 (
   pnet_t * net,
   pf_iod_write_request_t const * request,
   bool prm_flag,
   pnet_im_4_t const * im_4,
   pnet_result_t * result)
{
   if (net->fspm_cfg.write_im_4_cb == NULL)
   {
      pf_set_error (
         result,
         PNET_ERROR_CODE_WRITE,
         PNET_ERROR_DECODE_PNIORW,
         PNET_ERROR_CODE_1_ACC_INVALID_INDEX,
         PNET_ERROR_CODE_2_PNIORW (10));
      return -1;
   }
   return net->fspm_cfg.write_im_4_cb (
      net,
      net->fspm_cfg.cb_arg,
      request->api,
      request->slot_number,
      request->subslot_number,
      prm_flag,
      im_4,
      result);
}

int pf_fspm_cm_connect_ind (
   pnet_t * net,
   const pf_ar_t * p_ar,
   pnet_result_t * p_result)
{
   int ret = 0;

   if (net->fspm_cfg.connect_cb != NULL)
   {
      LOG_DEBUG (
         PNET_LOG,
         "FSPM(%d): Triggering connect callback for AREP %u\n",
         __LINE__,
         p_ar->arep);

      ret = net->fspm_cfg
               .connect_cb (net, net->fspm_cfg.cb_arg, p_ar->arep, p_result);
   }
   else
   {
      LOG_DEBUG (
         PNET_LOG,
         "FSPM(%d): No user callback for connect implemented.\n",
         __LINE__);
   }

   pf_fspm_create_log_book_entry (
      net,
      p_ar->arep,
      &p_result->pnio_status,
      PF_LOG_BOOK_AR_CONNECT);

   return ret;
}

int pf_fspm_cm_release_ind (
   pnet_t * net,
   const pf_ar_t * p_ar,
   pnet_result_t * p_result)
{
   int ret = 0;

   if (net->fspm_cfg.release_cb != NULL)
   {
      LOG_DEBUG (
         PNET_LOG,
         "FSPM(%d): Triggering release callback for AREP %u\n",
         __LINE__,
         p_ar->arep);

      ret = net->fspm_cfg
               .release_cb (net, net->fspm_cfg.cb_arg, p_ar->arep, p_result);
   }
   else
   {
      LOG_DEBUG (
         PNET_LOG,
         "FSPM(%d): No user callback for release implemented.\n",
         __LINE__);
   }

   pf_fspm_create_log_book_entry (
      net,
      p_ar->arep,
      &p_result->pnio_status,
      PF_LOG_BOOK_AR_RELEASE);

   return ret;
}

int pf_fspm_cm_dcontrol_ind (
   pnet_t * net,
   const pf_ar_t * p_ar,
   pnet_control_command_t control_command,
   pnet_result_t * p_result)
{
   int ret = 0;

   if (net->fspm_cfg.dcontrol_cb != NULL)
   {
      LOG_DEBUG (
         PNET_LOG,
         "FSPM(%d): Triggering DControl callback with command bitfield 0x%04x "
         "for AREP %u\n",
         __LINE__,
         control_command,
         p_ar->arep);

      ret = net->fspm_cfg.dcontrol_cb (
         net,
         net->fspm_cfg.cb_arg,
         p_ar->arep,
         control_command,
         p_result);
   }
   else
   {
      LOG_DEBUG (
         PNET_LOG,
         "FSPM(%d): No user callback for dcontrol ind implemented.\n",
         __LINE__);
   }

   pf_fspm_create_log_book_entry (
      net,
      p_ar->arep,
      &p_result->pnio_status,
      PF_LOG_BOOK_DCONTROL);

   return ret;
}

void pf_fspm_state_ind (
   pnet_t * net,
   const pf_ar_t * p_ar,
   pnet_event_values_t event)
{
   CC_ASSERT (p_ar != NULL);

   if (net->fspm_cfg.state_cb != NULL)
   {
      LOG_DEBUG (
         PNET_LOG,
         "FSPM(%d): Triggering user state-change callback with event %s for "
         "AREP %u\n",
         __LINE__,
         pf_cmdev_event_to_string (event),
         p_ar->arep);
      (void)
         net->fspm_cfg.state_cb (net, net->fspm_cfg.cb_arg, p_ar->arep, event);
   }
   else
   {
      LOG_DEBUG (
         PNET_LOG,
         "FSPM(%d): No user callback for state change implemented.\n",
         __LINE__);
   }
}

int pf_fspm_alpmr_alarm_ind (
   pnet_t * net,
   const pf_ar_t * p_ar,
   const pnet_alarm_argument_t * p_alarm_arg,
   uint16_t data_len,
   uint16_t data_usi,
   const uint8_t * p_data)
{
   int ret = 0;

   if (net->fspm_cfg.alarm_ind_cb != NULL)
   {
      LOG_DEBUG (
         PNET_LOG,
         "FSPM(%d): Triggering alarm callback with USI %u (%u bytes) for AREP "
         "%u\n",
         __LINE__,
         data_usi,
         data_len,
         p_ar->arep);

      ret = net->fspm_cfg.alarm_ind_cb (
         net,
         net->fspm_cfg.cb_arg,
         p_ar->arep,
         p_alarm_arg,
         data_len,
         data_usi,
         p_data);
   }
   else
   {
      LOG_DEBUG (
         PNET_LOG,
         "FSPM(%d): No user callback for alarm ind implemented.\n",
         __LINE__);
   }

   return ret;
}

void pf_fspm_alpmi_alarm_cnf (
   pnet_t * net,
   const pf_ar_t * p_ar,
   const pnet_pnio_status_t * p_pnio_status)
{
   if (net->fspm_cfg.alarm_cnf_cb != NULL)
   {
      LOG_DEBUG (
         PNET_LOG,
         "FSPM(%d): Triggering alarm confirmation callback for AREP %u\n",
         __LINE__,
         p_ar->arep);

      (void)net->fspm_cfg
         .alarm_cnf_cb (net, net->fspm_cfg.cb_arg, p_ar->arep, p_pnio_status);
   }
   else
   {
      LOG_DEBUG (
         PNET_LOG,
         "FSPM(%d): No user callback for alarm cnf implemented.\n",
         __LINE__);
   }
}

void pf_fspm_alpmr_alarm_ack_cnf (pnet_t * net, const pf_ar_t * p_ar, int res)
{
   if (net->fspm_cfg.alarm_ack_cnf_cb != NULL)
   {
      LOG_DEBUG (
         PNET_LOG,
         "FSPM(%d): Triggering alarm ACK confirmation callback for AREP %u\n",
         __LINE__,
         p_ar->arep);

      (void)net->fspm_cfg
         .alarm_ack_cnf_cb (net, net->fspm_cfg.cb_arg, p_ar->arep, res);
   }
   else
   {
      LOG_DEBUG (
         PNET_LOG,
         "FSPM(%d): No user callback for alarm ack cnf implemented.\n",
         __LINE__);
   }
}

void pf_fspm_reset_ind (
   pnet_t * net,
   bool should_reset_application,
   uint16_t reset_mode)
{
   if (net->fspm_cfg.reset_cb != NULL)
   {
      LOG_DEBUG (
         PNET_LOG,
         "FSPM(%d): Triggering reset callback with mode %u. Application "
         "reset: %s\n",
         __LINE__,
         reset_mode,
         should_reset_application ? "true" : "false");

      (void)net->fspm_cfg.reset_cb (
         net,
         net->fspm_cfg.cb_arg,
         should_reset_application,
         reset_mode);
   }
   else
   {
      LOG_DEBUG (
         PNET_LOG,
         "FSPM(%d): No user callback for reset implemented.\n",
         __LINE__);
   }
}

int pf_fspm_signal_led_ind (pnet_t * net, bool led_state)
{
   int ret = 0;
   if (net->fspm_cfg.signal_led_cb != NULL)
   {
      LOG_DEBUG (
         PNET_LOG,
         "FSPM(%d): Triggering signal LED callback with state %u.\n",
         __LINE__,
         led_state);

      ret = net->fspm_cfg.signal_led_cb (net, net->fspm_cfg.cb_arg, led_state);
   }
   else
   {
      LOG_DEBUG (
         PNET_LOG,
         "FSPM(%d): No user callback for signal LED implemented.\n",
         __LINE__);
   }

   return ret;
}

/**
 * Convert ip address from pnal format to pnet format.
 *
 * @param ip_addr  [in] IP address in pnal format.
 * return         IP address in pnet format.
 */
static pnet_cfg_ip_addr_t pf_fspm_conv_ip_address (pnal_ipaddr_t ip_addr)
{
   pnet_cfg_ip_addr_t ip_cfg_addr;

   ip_cfg_addr.a = (uint8_t)(ip_addr >> 24);
   ip_cfg_addr.b = (uint8_t)(ip_addr >> 16);
   ip_cfg_addr.c = (uint8_t)(ip_addr >> 8);
   ip_cfg_addr.d = (uint8_t)ip_addr;

   return ip_cfg_addr;
}

void pf_fspm_cfg_ind (pnet_t * net)
{
   if (net->fspm_cfg.cfg_cb != NULL)
   {
      pnet_ip_cfg_t ip_cfg;
      pnet_cfg_ind_data_t cfg;

      ip_cfg.dhcp_enable = net->cmina_current_dcp_ase.dhcp_enable;
      ip_cfg.ip_addr = pf_fspm_conv_ip_address (
         net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_addr);
      ip_cfg.ip_mask = pf_fspm_conv_ip_address (
         net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_mask);
      ip_cfg.ip_gateway = pf_fspm_conv_ip_address (
         net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_gateway);

      cfg.ip_cfg = &ip_cfg;
      cfg.station_name = net->cmina_current_dcp_ase.station_name;

      net->fspm_cfg.cfg_cb (net, net->fspm_cfg.cb_arg, &cfg);
   }
}

void pf_fspm_sm_set_default_params (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot)
{
   if (net->fspm_cfg.sm_set_default_params_cb != NULL)
   {
      switch (net->fspm_cfg.sm_set_default_params_cb (
         net,
         net->fspm_cfg.cb_arg,
         api,
         slot,
         subslot))
      {
      case 0:
         break;
      case -1:
         LOG_DEBUG (
            PNET_LOG,
            "FSPM(%d): Default parameterization of submodule in subslot %u "
            "of slot %u failed.\n",
            __LINE__,
            subslot,
            slot);
         break;
      default:
         LOG_ERROR (
            PNET_LOG,
            "FSPM(%d): Invalid return value of sm_set_default_params "
            "callback.\n",
            __LINE__);
         break;
      }
   }
#if PNET_OPTION_DR
   else
   {
      LOG_ERROR (
         PNET_LOG,
         "FSPM(%d): Missing sm_set_default_params callback\n",
         __LINE__);
   }
#endif
}
