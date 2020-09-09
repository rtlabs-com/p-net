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
 * @brief Implements the Fieldbus Application Layer Service Protocol Machine (FSPM)
 *
 * Stores the user-defined configuration, and calls the user-defined callbacks.
 * Create logbook entries.
 * Reads and writes identification & maintenance (I&M) records.
 */


#ifdef UNIT_TEST
#define os_get_current_time_us mock_os_get_current_time_us
#endif

#include <string.h>

#include "pf_includes.h"
#include "pf_block_reader.h"

void pf_fspm_im_show(
   pnet_t                  *net
)
{
   printf("Identification & Maintainance\n");
   printf("I&M1.im_tag_function     : <%s>\n", net->fspm_cfg.im_1_data.im_tag_function);
   printf("I&M1.im_tag_location     : <%s>\n", net->fspm_cfg.im_1_data.im_tag_location);
   printf("I&M2.date                : <%s>\n", net->fspm_cfg.im_2_data.im_date);
   printf("I&M3.im_descriptor       : <%s>\n", net->fspm_cfg.im_3_data.im_descriptor);
   printf("I&M4.im_signature        : <%s>\n", net->fspm_cfg.im_4_data.im_signature);    /* Should be binary data, but works for now */
}

void pf_fspm_option_show(
   pnet_t                  *net
)
{
   printf("Compile time options affecting memory consumption:\n");
   printf("PNET_MAX_AR                                    : %d\n", PNET_MAX_AR);
   printf("PNET_MAX_API                                   : %d\n", PNET_MAX_API);
   printf("PNET_MAX_CR                                    : %d\n", PNET_MAX_CR);
   printf("PNET_MAX_MODULES                               : %d\n", PNET_MAX_MODULES);
   printf("PNET_MAX_SUBMODULES                            : %d\n", PNET_MAX_SUBMODULES);
   printf("PNET_MAX_CHANNELS                              : %d\n", PNET_MAX_CHANNELS);
   printf("PNET_MAX_DFP_IOCR                              : %d\n", PNET_MAX_DFP_IOCR);
   printf("PNET_MAX_PORT                                  : %d\n", PNET_MAX_PORT);
   printf("PNET_MAX_LOG_BOOK_ENTRIES                      : %d\n", PNET_MAX_LOG_BOOK_ENTRIES);
   printf("PNET_MAX_ALARMS                                : %d\n", PNET_MAX_ALARMS);
   printf("PNET_MAX_DIAG_ITEMS                            : %d\n", PNET_MAX_DIAG_ITEMS);
   printf("PNET_MAX_DIRECTORYPATH_LENGTH                  : %d\n", PNET_MAX_DIRECTORYPATH_LENGTH);
   printf("PNET_MAX_FILENAME_LENGTH                       : %d\n", PNET_MAX_FILENAME_LENGTH);
   printf("PNET_MAX_SESSION_BUFFER_SIZE                   : %d\n", PNET_MAX_SESSION_BUFFER_SIZE);
   printf("PNET_MAX_MAN_SPECIFIC_FAST_STARTUP_DATA_LENGTH : %d\n", PNET_MAX_MAN_SPECIFIC_FAST_STARTUP_DATA_LENGTH);
   printf("PNET_OPTION_MC_CR                              : %d\n", PNET_OPTION_MC_CR);
#if PNET_OPTION_MC_CR
   printf("PNET_MAX_MC_CR                                 : %d\n", PNET_MAX_MC_CR);
#endif
   printf("PNET_OPTION_AR_VENDOR_BLOCKS                   : %d\n", PNET_OPTION_AR_VENDOR_BLOCKS);
#if PNET_OPTION_AR_VENDOR_BLOCKS
   printf("PNET_MAX_AR_VENDOR_BLOCKS                      : %d\n", PNET_MAX_AR_VENDOR_BLOCKS);
   printf("PNET_MAX_AR_VENDOR_BLOCK_DATA_LENGTH           : %d\n", PNET_MAX_AR_VENDOR_BLOCK_DATA_LENGTH);
#endif
   printf("PNET_OPTION_FAST_STARTUP                       : %d\n", PNET_OPTION_FAST_STARTUP);
   printf("PNET_OPTION_PARAMETER_SERVER                   : %d\n", PNET_OPTION_PARAMETER_SERVER);
   printf("PNET_OPTION_IR                                 : %d\n", PNET_OPTION_IR);
   printf("PNET_OPTION_SR                                 : %d\n", PNET_OPTION_SR);
   printf("PNET_OPTION_REDUNDANCY                         : %d\n", PNET_OPTION_REDUNDANCY);
   printf("PNET_OPTION_RS                                 : %d\n", PNET_OPTION_RS);
   printf("PNET_OPTION_SRL                                : %d\n", PNET_OPTION_SRL);
   printf("LOG_LEVEL (DEBUG=0, ERROR=3)                   : %d\n", LOG_LEVEL);
   printf("sizeof(pnet_t) : %" PRIu32 " bytes (for currently selected option values)\n", (uint32_t)sizeof(pnet_t));
}

/**
 * @internal
 * Validate the configuration from the user.
 *
 * @param p_cfg            In:    Configuration object
 * @return  0  if the configuration is valid.
 *          -1 if the configuration is invalid.
 */
int pf_fspm_validate_configuration(
   const pnet_cfg_t        *p_cfg)
{
   const uint16_t          im_mask = ~(PNET_SUPPORTED_IM1 | PNET_SUPPORTED_IM2 | PNET_SUPPORTED_IM3 | PNET_SUPPORTED_IM4);

   if (p_cfg == NULL)
   {
      LOG_ERROR(PNET_LOG, "FSPM(%d): You must provide a configuration, but it is NULL.\n", __LINE__);
      return -1;
   }

   if (p_cfg->min_device_interval == 0)
   {
      LOG_ERROR(PNET_LOG, "FSPM(%d): The min_device_interval in the config must not be 0. It should typically be 32, corresponding to 1 ms.\n", __LINE__);
      return -1;
   }
   else if (p_cfg->min_device_interval > 0x1000)
   {
      LOG_ERROR(PNET_LOG, "FSPM(%d): The min_device_interval in the config is too large. Max is 4096, corresponding to 128 ms.\n", __LINE__);
      return -1;
   }

   if ((p_cfg->im_0_data.im_supported & im_mask) > 0)
   {
      LOG_ERROR(PNET_LOG, "FSPM(%d): The I&M supported setting is wrong. Given: 0x%04x\n", __LINE__, p_cfg->im_0_data.im_supported);
      return -1;
   }

   return 0;
}

/**
 * @internal
 * Load the I&M settings from nonvolatile memory, if existing.
 *
 * @param net              InOut: The p-net stack instance
 */
static void pf_fspm_load_im(
   pnet_t                  *net)
{
   pnet_im_nvm_t           file_im;
   const char              *p_file_directory = NULL;

   (void)pf_cmina_get_file_directory(net, &p_file_directory);

   if (pf_file_load(p_file_directory, PNET_FILENAME_IM, &file_im, sizeof(pnet_im_nvm_t)) == 0)
   {
      LOG_DEBUG(PNET_LOG,"FSPM(%d): Did read I&M settings from nvm.\n", __LINE__);
      memcpy(&net->fspm_cfg.im_1_data, &file_im.im1, sizeof(pnet_im_1_t));
      memcpy(&net->fspm_cfg.im_2_data, &file_im.im2, sizeof(pnet_im_2_t));
      memcpy(&net->fspm_cfg.im_3_data, &file_im.im3, sizeof(pnet_im_3_t));
      memcpy(&net->fspm_cfg.im_4_data, &file_im.im4, sizeof(pnet_im_4_t));
   }
   else
   {
      LOG_DEBUG(PNET_LOG,"FSPM(%d): Could not yet read I&M settings from nvm. Use values from user configuration.\n", __LINE__);
   }
}

/**
 * @internal
 * Save the I&M settings to nonvolatile memory, if necessary.
 *
 * Compares with the content of already stored settings (in order not to
 * wear out the flash chip)
 *
 * @param net              InOut: The p-net stack instance
 */
static void pf_fspm_save_im_if_modified(
   pnet_t                  *net)
{
   pnet_im_nvm_t           output_im = {0};
   pnet_im_nvm_t           current_file_im = {0};
   bool                    save = false;
   const char              *p_file_directory = NULL;

   memcpy(&output_im.im1, &net->fspm_cfg.im_1_data, sizeof(pnet_im_1_t));
   memcpy(&output_im.im2, &net->fspm_cfg.im_2_data, sizeof(pnet_im_2_t));
   memcpy(&output_im.im3, &net->fspm_cfg.im_3_data, sizeof(pnet_im_3_t));
   memcpy(&output_im.im4, &net->fspm_cfg.im_4_data, sizeof(pnet_im_4_t));

   (void)pf_cmina_get_file_directory(net, &p_file_directory);

   if (pf_file_load(p_file_directory, PNET_FILENAME_IM, &current_file_im, sizeof(pnet_im_nvm_t)) == 0)
   {
      if (memcmp(&current_file_im, &output_im, sizeof(pnet_im_nvm_t)) != 0)
      {
         LOG_INFO(PNET_LOG,"FSPM(%d): Updating nvm stored I&M settings.\n", __LINE__);
         save = true;
      }
   }
   else
   {
      LOG_INFO(PNET_LOG,"FSPM(%d): First nvm saving of I&M settings.\n", __LINE__);
      save = true;
   }

   if (save == true)
   {
      if (pf_file_save(p_file_directory, PNET_FILENAME_IM, &output_im, sizeof(pnet_im_nvm_t)) != 0)
      {
         LOG_ERROR(PNET_LOG,"FSPM(%d): Failed to store nvm I&M settings.\n", __LINE__);
      }
   }
   else
   {
      LOG_DEBUG(PNET_LOG,"FSPM(%d): No storing of nvm I&M settings (no changes).\n", __LINE__);
   }
}

int pf_fspm_init(
   pnet_t                  *net,
   const pnet_cfg_t        *p_cfg)
{
   if (pf_fspm_validate_configuration(p_cfg) != 0)
   {
      return -1;
   }

   /* Use a copy of the configuration. For example the I&M data might be updated at runtime. */
   net->fspm_cfg = *p_cfg;

   /* Reference to the default settings (used at factory reset) */
   net->p_fspm_default_cfg = p_cfg;

   /* Load I&M data modifications from file, if any */
   pf_fspm_load_im(net);
   pf_fspm_save_im_if_modified(net);

   /* Log book */
   if (net->fspm_log_book_mutex == NULL)
   {
      net->fspm_log_book_mutex = os_mutex_create();
   }

   /* Turn LED off */
   if (pf_fspm_signal_led_ind(net, false) != 0){
      LOG_ERROR(PNET_LOG, "FSPM(%d): Could not turn signal LED off\n", __LINE__);
   }

   return 0;
}

void pf_fspm_create_log_book_entry(
   pnet_t                     *net,
   uint32_t                   arep,
   const pnet_pnio_status_t   *p_pnio_status,
   uint32_t                   entry_detail)
{
   uint16_t put;
   uint32_t time = 0;
   pf_ar_t     *p_ar = NULL;
   pf_uuid_t   *p_ar_uuid = NULL;

   if (pf_ar_find_by_arep(net, arep, &p_ar) == 0)
   {
      p_ar_uuid = &p_ar->ar_param.ar_uuid;
      os_mutex_lock(net->fspm_log_book_mutex);
      put = net->fspm_log_book.put;
      net->fspm_log_book.put++;
      if (net->fspm_log_book.put >= NELEMENTS(net->fspm_log_book.entries))
      {
         net->fspm_log_book.put = 0;
         net->fspm_log_book.wrap = true;
      }

      time = os_get_current_time_us();
      net->fspm_log_book.entries[put].time_ts.status = PF_TS_STATUS_LOCAL_ARB;
      net->fspm_log_book.entries[put].time_ts.sec_hi = 0;
      net->fspm_log_book.entries[put].time_ts.sec_lo = time/1000000;
      net->fspm_log_book.entries[put].time_ts.nano_sec = (time%1000000)*1000;

      net->fspm_log_book.entries[put].ar_uuid = *p_ar_uuid;
      net->fspm_log_book.entries[put].pnio_status = *p_pnio_status;
      net->fspm_log_book.entries[put].entry_detail = entry_detail;
      os_mutex_unlock(net->fspm_log_book_mutex);
   }
}

int pf_fspm_clear_im_data(
   pnet_t                  *net)
{
   memset(net->fspm_cfg.im_1_data.im_tag_function, ' ', sizeof(net->fspm_cfg.im_1_data.im_tag_function));
   net->fspm_cfg.im_1_data.im_tag_function[sizeof(net->fspm_cfg.im_1_data.im_tag_function) - 1] = '\0';
   memset(net->fspm_cfg.im_1_data.im_tag_location, ' ', sizeof(net->fspm_cfg.im_1_data.im_tag_location));
   net->fspm_cfg.im_1_data.im_tag_location[sizeof(net->fspm_cfg.im_1_data.im_tag_location) - 1] = '\0';
   memset(net->fspm_cfg.im_2_data.im_date, ' ', sizeof(net->fspm_cfg.im_2_data.im_date));
   net->fspm_cfg.im_2_data.im_date[sizeof(net->fspm_cfg.im_2_data.im_date) - 1] = '\0';
   memset(net->fspm_cfg.im_3_data.im_descriptor, ' ', sizeof(net->fspm_cfg.im_3_data.im_descriptor));
   net->fspm_cfg.im_3_data.im_descriptor[sizeof(net->fspm_cfg.im_3_data.im_descriptor) - 1] = '\0';
   memset(net->fspm_cfg.im_4_data.im_signature, 0, sizeof(net->fspm_cfg.im_4_data.im_signature));

   pf_fspm_save_im_if_modified(net);

   return 0;
}

void pf_fspm_get_cfg(
   pnet_t                  *net,
   pnet_cfg_t              **pp_cfg)
{
   if (pp_cfg != NULL)
   {
      *pp_cfg = &net->fspm_cfg;
   }
}

void pf_fspm_get_default_cfg(
   pnet_t                  *net,
   const pnet_cfg_t        **pp_cfg)
{
   if (pp_cfg != NULL)
   {
      *pp_cfg = net->p_fspm_default_cfg;
   }
}

int16_t pf_cmina_get_min_device_interval(
   pnet_t                  *net)
{
   return net->fspm_cfg.min_device_interval;
}

/******************* Execute user callbacks *********************************/

int pf_fspm_exp_module_ind(
   pnet_t                  *net,
   uint32_t                api,
   uint16_t                slot,
   uint32_t                module_ident)
{
   int ret = -1;

   if (net->fspm_cfg.exp_module_cb != NULL)
   {
      ret = net->fspm_cfg.exp_module_cb(net, net->fspm_cfg.cb_arg, api, slot, module_ident);
   }

   return ret;
}

int pf_fspm_exp_submodule_ind(
   pnet_t                  *net,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint32_t                module_ident,
   uint32_t                submodule_ident)
{
   int ret = -1;

   if (net->fspm_cfg.exp_submodule_cb != NULL)
   {
      ret = net->fspm_cfg.exp_submodule_cb(net, net->fspm_cfg.cb_arg, api, slot, subslot, module_ident, submodule_ident);
   }

   return ret;
}

int pf_fspm_data_status_changed(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pf_iocr_t               *p_iocr,
   uint8_t                 changes,
   uint8_t                 data_status)
{
   int ret = -1;

   if (net->fspm_cfg.new_data_status_cb != NULL)
   {
      ret = net->fspm_cfg.new_data_status_cb(net, net->fspm_cfg.cb_arg, p_ar->arep, p_iocr->crep, changes, data_status);
   }

   return ret;
}

int pf_fspm_ccontrol_cnf(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_result_t           *p_result)
{
   int ret = 0;

   if (net->fspm_cfg.ccontrol_cb != NULL)
   {
      ret = net->fspm_cfg.ccontrol_cb(net, net->fspm_cfg.cb_arg, p_ar->arep, p_result);
   }

   return ret;
}

int pf_fspm_cm_read_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pf_iod_read_request_t   *p_read_request,
   uint8_t                 **pp_read_data,
   uint16_t                *p_read_length,
   pnet_result_t           *p_read_status)
{
   int ret = -1;

   if (p_read_request->index <= PF_IDX_USER_MAX)
   {
      /* Trigger callback for application-specific data records */
      if (net->fspm_cfg.read_cb != NULL)
      {
         ret = net->fspm_cfg.read_cb(net, net->fspm_cfg.cb_arg,
            p_ar->arep, p_read_request->api,
            p_read_request->slot_number, p_read_request->subslot_number,
            p_read_request->index, p_read_request->sequence_number,
            pp_read_data, p_read_length, p_read_status);
      }
      else
      {
         p_read_status->pnio_status.error_code = PNET_ERROR_CODE_READ;
         p_read_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
         p_read_status->pnio_status.error_code_1 = PNET_ERROR_CODE_1_APP_NOT_SUPPORTED;
         p_read_status->pnio_status.error_code_2 = 0;
      }
   }
   else if ((PF_IDX_SUB_IM_0 <= p_read_request->index) && (p_read_request->index <= PF_IDX_SUB_IM_15))
   {
      /* Read I&M data - This is handled internally. */
      switch (p_read_request->index)
      {
      case PF_IDX_SUB_IM_0:

         if (*p_read_length >= sizeof(net->fspm_cfg.im_0_data))
         {
            LOG_INFO(PNET_LOG, "FSPM(%d): PLC is reading I&M0 data.\n", __LINE__);
            *pp_read_data = (uint8_t *)&net->fspm_cfg.im_0_data;
            *p_read_length = sizeof(net->fspm_cfg.im_0_data);
            ret = 0;
         }
         else
         {
            LOG_ERROR(PNET_LOG, "FSPM(%d): Failed to read I&M0 data\n", __LINE__);
            p_read_status->pnio_status.error_code = PNET_ERROR_CODE_READ;
            p_read_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
            p_read_status->pnio_status.error_code_1 = PNET_ERROR_CODE_1_APP_READ_ERROR;
            p_read_status->pnio_status.error_code_2 = 0;
         }
         break;
      case PF_IDX_SUB_IM_1:
         if ((net->fspm_cfg.im_0_data.im_supported & PNET_SUPPORTED_IM1) > 0)
         {
            LOG_INFO(PNET_LOG, "FSPM(%d): PLC is reading I&M1 data. Function: %s Location: %s\n",
               __LINE__, net->fspm_cfg.im_1_data.im_tag_function, net->fspm_cfg.im_1_data.im_tag_location);
            *pp_read_data = (uint8_t *)&net->fspm_cfg.im_1_data;
            *p_read_length = sizeof(net->fspm_cfg.im_1_data);
            ret = 0;
         }
         else
         {
            LOG_INFO(PNET_LOG, "FSPM(%d): PLC is trying to read I&M1 data, but we have not enabled it.\n",__LINE__);
         }
         break;
      case PF_IDX_SUB_IM_2:
         if ((net->fspm_cfg.im_0_data.im_supported & PNET_SUPPORTED_IM2) > 0)
         {
            LOG_INFO(PNET_LOG, "FSPM(%d): PLC is reading I&M2 data. Date: %s\n", __LINE__, net->fspm_cfg.im_2_data.im_date);
            *pp_read_data = (uint8_t *)&net->fspm_cfg.im_2_data;
            *p_read_length = sizeof(net->fspm_cfg.im_2_data);
            ret = 0;
         }
         else
         {
            LOG_INFO(PNET_LOG, "FSPM(%d): PLC is trying to read I&M2 data, but we have not enabled it.\n",__LINE__);
         }
         break;
      case PF_IDX_SUB_IM_3:
         if ((net->fspm_cfg.im_0_data.im_supported & PNET_SUPPORTED_IM3) > 0)
         {
            LOG_INFO(PNET_LOG, "FSPM(%d): PLC is reading I&M3 data. Descriptor: %s\n", __LINE__, net->fspm_cfg.im_3_data.im_descriptor);
            *pp_read_data = (uint8_t *)&net->fspm_cfg.im_3_data;
            *p_read_length = sizeof(net->fspm_cfg.im_3_data);
            ret = 0;
         }
         else
         {
            LOG_INFO(PNET_LOG, "FSPM(%d): PLC is trying to read I&M3 data, but we have not enabled it.\n",__LINE__);
         }
         break;
      case PF_IDX_SUB_IM_4:
         if ((net->fspm_cfg.im_0_data.im_supported & PNET_SUPPORTED_IM4) > 0)
         {
            LOG_INFO(PNET_LOG, "FSPM(%d): PLC is reading I&M4 data\n", __LINE__);
            *pp_read_data = (uint8_t *)&net->fspm_cfg.im_4_data;
            *p_read_length = sizeof(net->fspm_cfg.im_4_data);
            ret = 0;
         }
         else
         {
            LOG_INFO(PNET_LOG, "FSPM(%d): PLC is trying to read I&M4 data, but we have not enabled it.\n",__LINE__);
         }
         break;
      default:
         /* Nothing if data not available here */
         LOG_ERROR(PNET_LOG, "FSPM(%d): Request to read non-implemented I&M data. Index %u\n", __LINE__, p_read_request->index);
         break;
      }
   }
   else if (p_read_request->index == PF_IDX_DEV_LOGBOOK_DATA)
   {
      LOG_INFO(PNET_LOG, "FSPM(%d): Read logbook data\n", __LINE__);
      *pp_read_data = (uint8_t *)&net->fspm_log_book;
      *p_read_length = sizeof(net->fspm_log_book);
      ret = 0;
   }
   else
   {
      /* Nothing if data not available here */
   }

   return ret;
}

int pf_fspm_cm_write_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pf_iod_write_request_t  *p_write_request,
   uint16_t                write_length,
   uint8_t                 *p_write_data,
   pnet_result_t           *p_write_status)
{
   int                     ret = -1;
   pf_get_info_t           get_info;
   uint16_t                pos = 0;

   if (p_write_request->index <= PF_IDX_USER_MAX)
   {
      /* Trigger callback for application-specific data records */
      if (net->fspm_cfg.write_cb != NULL)
      {
         ret = net->fspm_cfg.write_cb(net, net->fspm_cfg.cb_arg,
            p_ar->arep, p_write_request->api,
            p_write_request->slot_number, p_write_request->subslot_number,
            p_write_request->index, p_write_request->sequence_number,
            write_length, p_write_data, p_write_status);
      }
      else
      {
         p_write_status->pnio_status.error_code = PNET_ERROR_CODE_WRITE;
         p_write_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
         p_write_status->pnio_status.error_code_1 = PNET_ERROR_CODE_1_APP_NOT_SUPPORTED;
         p_write_status->pnio_status.error_code_2 = 0;
      }
   }
   else if ((PF_IDX_SUB_IM_0 <= p_write_request->index) && (p_write_request->index <= PF_IDX_SUB_IM_15))
   {
      /* Write I&M data - This is handled internally. */
      switch (p_write_request->index)
      {
      case PF_IDX_SUB_IM_0:      /* read-only */
         LOG_ERROR(PNET_LOG, "FSPM(%d): Request to write I&M0 data, but it is read-only.\n", __LINE__);
         p_write_status->pnio_status.error_code = PNET_ERROR_CODE_WRITE;
         p_write_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
         p_write_status->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_ACCESS_DENIED;
         p_write_status->pnio_status.error_code_2 = 0;
         ret = 0;
         break;
      case PF_IDX_SUB_IM_1:
         if ((net->fspm_cfg.im_0_data.im_supported & PNET_SUPPORTED_IM1) > 0)
         {
            get_info.result = PF_PARSE_OK;
            get_info.p_buf = p_write_data;
            get_info.is_big_endian = true;
            get_info.len = write_length;      /* Bytes in input buffer */

            pf_get_im_1(&get_info, &pos, &net->fspm_cfg.im_1_data);
            if ((get_info.result == PF_PARSE_OK) && (pos == write_length))
            {
               LOG_INFO(PNET_LOG, "FSPM(%d): PLC is writing I&M1 data. Function: %s Location: %s\n",
                  __LINE__, net->fspm_cfg.im_1_data.im_tag_function, net->fspm_cfg.im_1_data.im_tag_location);
               ret = 0;
            }
            else
            {
               LOG_ERROR(PNET_LOG, "FSPM(%d): Wrong length of incoming I&M1 data\n", __LINE__);
               p_write_status->pnio_status.error_code = PNET_ERROR_CODE_WRITE;
               p_write_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
               p_write_status->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_WRITE_LENGTH_ERROR;
               p_write_status->pnio_status.error_code_2 = 0;
            }
         }
         else
         {
            LOG_ERROR(PNET_LOG, "FSPM(%d): PLC is trying to write I&M1 data, but we have not enabled it.\n",__LINE__);
            p_write_status->pnio_status.error_code = PNET_ERROR_CODE_WRITE;
            p_write_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
            p_write_status->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_INVALID_INDEX;
            p_write_status->pnio_status.error_code_2 = 0;
         }
         break;
      case PF_IDX_SUB_IM_2:
         if ((net->fspm_cfg.im_0_data.im_supported & PNET_SUPPORTED_IM2) > 0)
         {
            /* Do not count the terminator byte */
            if (write_length == (sizeof(net->fspm_cfg.im_2_data) - 1))
            {
               memcpy(&net->fspm_cfg.im_2_data, p_write_data, sizeof(net->fspm_cfg.im_2_data) - 1);
               net->fspm_cfg.im_2_data.im_date[sizeof(net->fspm_cfg.im_2_data) - 1] = '\0';
               LOG_INFO(PNET_LOG, "FSPM(%d): PLC is writing I&M2 data. Date: %s\n", __LINE__, net->fspm_cfg.im_2_data.im_date);
               ret = 0;
            }
            else
            {
               LOG_ERROR(PNET_LOG, "FSPM(%d): Wrong length of incoming I&M2 data\n", __LINE__);
               p_write_status->pnio_status.error_code = PNET_ERROR_CODE_WRITE;
               p_write_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
               p_write_status->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_WRITE_LENGTH_ERROR;
               p_write_status->pnio_status.error_code_2 = 0;
            }
         }
         else
         {
            LOG_ERROR(PNET_LOG, "FSPM(%d): PLC is trying to write I&M2 data, but we have not enabled it.\n",__LINE__);
            p_write_status->pnio_status.error_code = PNET_ERROR_CODE_WRITE;
            p_write_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
            p_write_status->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_INVALID_INDEX;
            p_write_status->pnio_status.error_code_2 = 0;
         }
         break;
      case PF_IDX_SUB_IM_3:
         if ((net->fspm_cfg.im_0_data.im_supported & PNET_SUPPORTED_IM3) > 0)
         {
            /* Do not count the terminator byte */
            if (write_length == (sizeof(net->fspm_cfg.im_3_data) - 1))
            {
               memcpy(&net->fspm_cfg.im_3_data, p_write_data, sizeof(net->fspm_cfg.im_3_data) - 1);
               net->fspm_cfg.im_3_data.im_descriptor[sizeof(net->fspm_cfg.im_3_data) - 1] = '\0';
               LOG_INFO(PNET_LOG, "FSPM(%d): PLC is writing I&M3 data. Descriptor: %s\n", __LINE__, net->fspm_cfg.im_3_data.im_descriptor);
               ret = 0;
            }
            else
            {
               LOG_ERROR(PNET_LOG, "FSPM(%d): Wrong length of incoming I&M3 data\n", __LINE__);
               p_write_status->pnio_status.error_code = PNET_ERROR_CODE_WRITE;
               p_write_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
               p_write_status->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_WRITE_LENGTH_ERROR;
               p_write_status->pnio_status.error_code_2 = 0;
            }
         }
         else
         {
            LOG_ERROR(PNET_LOG, "FSPM(%d): PLC is trying to write I&M3 data, but we have not enabled it.\n",__LINE__);
            p_write_status->pnio_status.error_code = PNET_ERROR_CODE_WRITE;
            p_write_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
            p_write_status->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_INVALID_INDEX;
            p_write_status->pnio_status.error_code_2 = 0;
         }
         break;
      case PF_IDX_SUB_IM_4:
         if ((net->fspm_cfg.im_0_data.im_supported & PNET_SUPPORTED_IM4) > 0)
         {
            if (write_length == sizeof(net->fspm_cfg.im_4_data))
            {
               LOG_INFO(PNET_LOG, "FSPM(%d): PLC is writing I&M4 data\n", __LINE__);
               memcpy(&net->fspm_cfg.im_4_data, p_write_data, sizeof(net->fspm_cfg.im_4_data));
               ret = 0;
            }
            else
            {
               LOG_ERROR(PNET_LOG, "FSPM(%d): Wrong length of incoming I&M4 data\n", __LINE__);
               p_write_status->pnio_status.error_code = PNET_ERROR_CODE_WRITE;
               p_write_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
               p_write_status->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_WRITE_LENGTH_ERROR;
               p_write_status->pnio_status.error_code_2 = 0;
            }
         }
         else
         {
            LOG_ERROR(PNET_LOG, "FSPM(%d): PLC is trying to write I&M4 data, but we have not enabled it.\n",__LINE__);
            p_write_status->pnio_status.error_code = PNET_ERROR_CODE_WRITE;
            p_write_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
            p_write_status->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_INVALID_INDEX;
            p_write_status->pnio_status.error_code_2 = 0;
         }
         break;
      default:
         LOG_ERROR(PNET_LOG, "FSPM(%d): Trying to write non-implemented I&M data. Index %u\n", __LINE__, p_write_request->index);
         p_write_status->pnio_status.error_code = PNET_ERROR_CODE_WRITE;
         p_write_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
         p_write_status->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_INVALID_INDEX;
         p_write_status->pnio_status.error_code_2 = 0;
         break;
      }

      pf_fspm_save_im_if_modified(net);

   }
   else
   {
      /* Nothing if write not possible here */
   }

   return ret;
}

int pf_fspm_cm_connect_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_result_t           *p_result)
{
   int ret = 0;

   if (net->fspm_cfg.connect_cb != NULL)
   {
      ret = net->fspm_cfg.connect_cb(net, net->fspm_cfg.cb_arg, p_ar->arep, p_result);
   }

   return ret;
}

int pf_fspm_cm_release_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_result_t           *p_result)
{
   int ret = 0;

   if (net->fspm_cfg.release_cb != NULL)
   {
      ret = net->fspm_cfg.release_cb(net, net->fspm_cfg.cb_arg, p_ar->arep, p_result);
   }

   return ret;
}

int pf_fspm_cm_dcontrol_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_control_command_t  control_command,
   pnet_result_t           *p_result)
{
   int ret = 0;

   if (net->fspm_cfg.dcontrol_cb != NULL)
   {
      ret = net->fspm_cfg.dcontrol_cb(net, net->fspm_cfg.cb_arg, p_ar->arep, control_command, p_result);
   }

   return ret;
}

int pf_fspm_state_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_event_values_t     event)
{
   int ret = 0;

   CC_ASSERT(p_ar != NULL);

   LOG_DEBUG(PNET_LOG, "FSPM(%d): Triggering user state-change callback with %s\n",  __LINE__, pf_cmdev_event_to_string(event));

   if (net->fspm_cfg.state_cb != NULL)
   {
      ret = net->fspm_cfg.state_cb(net, net->fspm_cfg.cb_arg, p_ar->arep, event);
   }

   return ret;
}

int pf_fspm_alpmr_alarm_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint16_t                data_len,
   uint16_t                data_usi,
   uint8_t                 *p_data)
{
   int ret = 0;

   if (net->fspm_cfg.alarm_ind_cb != NULL)
   {
      ret = net->fspm_cfg.alarm_ind_cb(net, net->fspm_cfg.cb_arg, p_ar->arep, api, slot, subslot, data_len, data_usi, p_data);
   }

   return ret;
}

int pf_fspm_alpmi_alarm_cnf(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_pnio_status_t      *p_pnio_status)
{
   int ret = 0;

   if (net->fspm_cfg.alarm_cnf_cb != NULL)
   {
      ret = net->fspm_cfg.alarm_cnf_cb(net, net->fspm_cfg.cb_arg, p_ar->arep, p_pnio_status);
   }

   return ret;
}

int pf_fspm_alpmr_alarm_ack_cnf(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   int                     res)
{
   int ret = 0;

   if (net->fspm_cfg.alarm_ack_cnf_cb != NULL)
   {
      ret = net->fspm_cfg.alarm_ack_cnf_cb(net, net->fspm_cfg.cb_arg, p_ar->arep, res);
   }

   return ret;
}

int pf_fspm_reset_ind(
   pnet_t                  *net,
   bool                    should_reset_application,
   uint16_t                reset_mode)
{
   int ret = 0;

   if (net->fspm_cfg.reset_cb != NULL)
   {
      ret = net->fspm_cfg.reset_cb(net, net->fspm_cfg.cb_arg, should_reset_application, reset_mode);
   }

   return ret;
}

int pf_fspm_signal_led_ind(
   pnet_t                  *net,
   bool                    led_state)
{
   int ret = 0;
   if (net->fspm_cfg.signal_led_cb != NULL)
   {
      ret = net->fspm_cfg.signal_led_cb(net, net->fspm_cfg.cb_arg, led_state);
   }

   return ret;
}
