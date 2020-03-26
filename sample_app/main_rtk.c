#include <string.h>

#include <kern.h>
#include <shell.h>

#include <pnet_api.h>
#include "osal.h"
#include "log.h"


#include <lwip/netif.h>

/********************* Call-back function declarations ************************/

static int app_exp_module_ind(uint16_t api, uint16_t slot, uint32_t module_ident_number);
static int app_exp_submodule_ind(uint16_t api, uint16_t slot, uint16_t subslot, uint32_t module_ident_number, uint32_t submodule_ident_number);
static int app_new_data_status_ind(uint32_t arep, uint32_t crep, uint8_t changes);
static int app_connect_ind(uint32_t arep, pnet_result_t *p_result);
static int app_state_ind(uint32_t arep, pnet_event_values_t state);
static int app_release_ind(uint32_t arep, pnet_result_t *p_result);
static int app_dcontrol_ind(uint32_t arep, pnet_control_command_t control_command, pnet_result_t *p_result);
static int app_ccontrol_cnf(uint32_t arep, pnet_result_t *p_result);
static int app_write_ind(uint32_t arep, uint16_t api, uint16_t slot, uint16_t subslot, uint16_t idx, uint16_t sequence_number, uint16_t write_length, uint8_t *p_write_data, pnet_result_t *p_result);
static int app_read_ind(uint32_t arep, uint16_t api, uint16_t slot, uint16_t subslot, uint16_t idx, uint16_t sequence_number, uint8_t **pp_read_data, uint16_t *p_read_length, pnet_result_t *p_result);
static int app_alarm_cnf(uint32_t arep, pnet_pnio_status_t *p_pnio_status);
static int app_alarm_ind(uint32_t arep, uint32_t api, uint16_t slot, uint16_t subslot, uint16_t data_len, uint16_t data_usi, uint8_t *p_data);
static int app_alarm_ack_cnf(uint32_t arep, int res);


/********************** Settings **********************************************/

#define EVENT_READY_FOR_DATA           BIT(0)
#define EVENT_TIMER                    BIT(1)
#define EVENT_ALARM                    BIT(2)
#define EVENT_ABORT                    BIT(15)

#define EXIT_CODE_ERROR                1
#define TICK_INTERVAL_US               1000     /* 1 ms */
#define APP_DEFAULT_ETHERNET_INTERFACE "en1"
#define APP_ALARM_USI                  1
#define APP_LED_ID                     0        /* "LED1" on circuit board */


/**************** From the GSDML file ****************************************/

#define APP_DEFAULT_STATION_NAME "rt-labs-dev"
#define APP_PARAM_IDX_1          123
#define APP_PARAM_IDX_2          124
#define APP_API                  0

/*
 * Module and submodule ident number for the DAP module.
 * The DAP module and submodules must be plugged by the application after the call to pnet_init.
 */
#define PNET_SLOT_DAP_IDENT                        0x00000000
#define PNET_MOD_DAP_IDENT                         0x00000001     /* For use in slot 0 */
#define PNET_SUBMOD_DAP_IDENT                      0x00000001     /* For use in subslot 1 */
#define PNET_SUBMOD_DAP_INTERFACE_1_IDENT          0x00008000     /* For use in subslot 0x8000 */
#define PNET_SUBMOD_DAP_INTERFACE_1_PORT_0_IDENT   0x00008001     /* For use in subslot 0x8001 */

/*
 * I/O Modules. These modules and their sub-modules must be plugged by the
 * application after the call to pnet_init.
 *
 * Assume that all modules only have a single submodule, with same number.
 */
#define PNET_MOD_8_0_IDENT          0x00000030     /* 8 bit input */
#define PNET_MOD_0_8_IDENT          0x00000031     /* 8 bit output */
#define PNET_MOD_8_8_IDENT          0x00000032     /* 8 bit input, 8 bit output */
#define PNET_SUBMOD_CUSTOM_IDENT    0x00000001

#define APP_DATASIZE_INPUT       1     /* bytes, for digital inputs data */
#define APP_DATASIZE_OUTPUT      1     /* bytes, for digital outputs data */
#define APP_ALARM_PAYLOAD_SIZE   1     /* bytes */


/*** Example on how to keep lists of supported modules and submodules ********/

static uint32_t            cfg_available_module_types[] =
{
   PNET_MOD_DAP_IDENT,
   PNET_MOD_8_0_IDENT,
   PNET_MOD_0_8_IDENT,
   PNET_MOD_8_8_IDENT
};

static const struct
{
   uint32_t                api;
   uint32_t                module_ident_nbr;
   uint32_t                submodule_ident_nbr;
   pnet_submodule_dir_t    data_dir;
   uint16_t                insize;     /* bytes */
   uint16_t                outsize;    /* bytes */
} cfg_available_submodule_types[] =
{
   {APP_API, PNET_MOD_DAP_IDENT, PNET_SUBMOD_DAP_IDENT,                    PNET_DIR_NO_IO,  0,                  0},
   {APP_API, PNET_MOD_DAP_IDENT, PNET_SUBMOD_DAP_INTERFACE_1_IDENT,        PNET_DIR_NO_IO,  0,                  0},
   {APP_API, PNET_MOD_DAP_IDENT, PNET_SUBMOD_DAP_INTERFACE_1_PORT_0_IDENT, PNET_DIR_NO_IO,  0,                  0},
   {APP_API, PNET_MOD_8_0_IDENT, PNET_SUBMOD_CUSTOM_IDENT,                 PNET_DIR_INPUT,  APP_DATASIZE_INPUT, 0},
   {APP_API, PNET_MOD_0_8_IDENT, PNET_SUBMOD_CUSTOM_IDENT,                 PNET_DIR_OUTPUT, 0,                  APP_DATASIZE_OUTPUT},
   {APP_API, PNET_MOD_8_8_IDENT, PNET_SUBMOD_CUSTOM_IDENT,                 PNET_DIR_IO,     APP_DATASIZE_INPUT, APP_DATASIZE_OUTPUT},
};


/************ Configuration of product ID, software version etc **************/

static pnet_cfg_t                pnet_default_cfg =
{
      /* Call-backs */
      .state_cb = app_state_ind,
      .connect_cb = app_connect_ind,
      .release_cb = app_release_ind,
      .dcontrol_cb = app_dcontrol_ind,
      .ccontrol_cb = app_ccontrol_cnf,
      .read_cb = app_read_ind,
      .write_cb = app_write_ind,
      .exp_module_cb = app_exp_module_ind,
      .exp_submodule_cb = app_exp_submodule_ind,
      .new_data_status_cb = app_new_data_status_ind,
      .alarm_ind_cb = app_alarm_ind,
      .alarm_cnf_cb = app_alarm_cnf,
      .alarm_ack_cnf_cb = app_alarm_ack_cnf,

      .im_0_data =
      {
       .vendor_id_hi = 0xfe,
       .vendor_id_lo = 0xed,
       .order_id = "<orderid>           ",
       .im_serial_number = "<serial nbr>    ",
       .im_hardware_revision = 1,
       .sw_revision_prefix = 'P', /* 'V', 'R', 'P', 'U', or 'T' */
       .im_sw_revision_functional_enhancement = 0,
       .im_sw_revision_bug_fix = 0,
       .im_sw_revision_internal_change = 0,
       .im_revision_counter = 0,
       .im_profile_id = 0x1234,
       .im_profile_specific_type = 0x5678,
       .im_version_major = 1,
       .im_version_minor = 1,
       .im_supported = 0x001e,         /* Only I&M0..I&M4 supported */
      },
      .im_1_data =
      {
       .im_tag_function = "",
       .im_tag_location = ""
      },
      .im_2_data =
      {
       .im_date = ""
      },
      .im_3_data =
      {
       .im_descriptor = ""
      },
      .im_4_data =
      {
       .im_signature = ""
      },

      /* Device configuration */
      .device_id =
      {  /* device id: vendor_id_hi, vendor_id_lo, device_id_hi, device_id_lo */
         0xfe, 0xed, 0xbe, 0xef,
      },
      .oem_device_id =
      {  /* OEM device id: vendor_id_hi, vendor_id_lo, device_id_hi, device_id_lo */
         0xc0, 0xff, 0xee, 0x01,
      },
      .station_name = APP_DEFAULT_STATION_NAME,
      .device_vendor = "rt-labs",
      .manufacturer_specific_string = "PNET demo",

      .lldp_cfg =
      {
       .chassis_id = "rt-labs1",   /* Is this a valid name? '-' allowed?*/
       .port_id = "port-001",
       .ttl = 20,          /* seconds */
       .rtclass_2_status = 0,
       .rtclass_3_status = 0,
       .cap_aneg = 3,      /* Supported (0x01) + enabled (0x02) */
       .cap_phy = 0x0C00,  /* Unknown (0x8000) */
       .mau_type = 0x0010, /* Default (copper): 100BaseTXFD */
      },

      /* Network configuration */
      .send_hello = 1,                /* Send HELLO */
      .dhcp_enable = 0,
      .ip_addr = { 0 },                   /* Read from lwip */
      .ip_mask = { 0 },                   /* Read from lwip */
      .ip_gateway = { 0 },                /* Read from lwip */
      .eth_addr = { 0 },                  /* Read from lwip */
};


/********************************** Globals ***********************************/

static os_timer_t                *main_timer = NULL;
static os_event_t                *main_events = NULL;
static uint8_t                   data_ctr = 0;
static uint32_t                  main_arep = UINT32_MAX;
static bool                      alarm_allowed = true;
static uint32_t                  app_param_1 = 0x0;
static uint32_t                  app_param_2 = 0x0;
static bool                      init_done = false;
static uint8_t                   inputdata[APP_DATASIZE_INPUT] = { 0 };
static uint8_t                   custom_input_slots[PNET_MAX_MODULES] = { 0 };
static uint8_t                   custom_output_slots[PNET_MAX_MODULES] = { 0 };

static int                       verbosity = 0;


/*********************************** Callbacks ********************************/

static int app_connect_ind(
   uint32_t                arep,
   pnet_result_t           *p_result)
{
   LOG_INFO(PNET_LOG, "APP: connect call-back %d %d %d %d\n",
         p_result->pnio_status.error_code,p_result->pnio_status.error_decode,
         p_result->pnio_status.error_code_1,p_result->pnio_status.error_code_2);

   /*
    *  Handle the request on an application level.
    *  This is a very simple application which does not need to handle anything.
    *  All the needed information is in the AR data structure.
    */

   return 0;
}

static int app_release_ind(
   uint32_t                arep,
   pnet_result_t           *p_result)
{
   LOG_INFO(PNET_LOG, "APP: release call-back %d %d %d %d\n",
         p_result->pnio_status.error_code,p_result->pnio_status.error_decode,
         p_result->pnio_status.error_code_1,p_result->pnio_status.error_code_2);

   return 0;
}

static int app_dcontrol_ind(
   uint32_t                arep,
   pnet_control_command_t  control_command,
   pnet_result_t           *p_result)
{
   LOG_INFO(PNET_LOG, "APP: dcontrol call-back (cmd:%d) %d %d %d %d\n",
         control_command,
         p_result->pnio_status.error_code,p_result->pnio_status.error_decode,
         p_result->pnio_status.error_code_1,p_result->pnio_status.error_code_2);

   return 0;
}

static int app_ccontrol_cnf(
   uint32_t                arep,
   pnet_result_t           *p_result)
{
   LOG_INFO(PNET_LOG, "APP: ccontrol confirmation call-back %d %d %d %d\n",
         p_result->pnio_status.error_code,p_result->pnio_status.error_decode,
         p_result->pnio_status.error_code_1,p_result->pnio_status.error_code_2);

   return 0;
}

static int app_write_ind(
   uint32_t                arep,
   uint16_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint16_t                idx,
   uint16_t                sequence_number,
   uint16_t                write_length,
   uint8_t                 *p_write_data,
   pnet_result_t           *p_result)
{
   LOG_INFO(PNET_LOG, "APP: write call-back: idx %u\n", (unsigned)idx);
   if (idx == APP_PARAM_IDX_1)
   {
      if (write_length == sizeof(app_param_1))
      {
         memcpy(&app_param_1, p_write_data, sizeof(app_param_1));
      }
      else
      {
          LOG_INFO(PNET_LOG, "APP: write call-back: idx %u  length %u - expected length %u\n",
                (unsigned)idx, (unsigned)write_length, (unsigned)sizeof(app_param_1));
      }
   }
   else if (idx == APP_PARAM_IDX_2)
   {
      if (write_length == sizeof(app_param_2))
      {
         memcpy(&app_param_2, p_write_data, sizeof(app_param_2));
      }
      else
      {
          LOG_INFO(PNET_LOG, "APP: write call-back: idx %u  length %u - expected length %u\n",
                (unsigned)idx, (unsigned)write_length, (unsigned)sizeof(app_param_2));
      }
   }
   else
   {
      LOG_INFO(PNET_LOG, "APP: write call-back: unknown idx %u\n", (unsigned)idx);
   }

   return 0;
}

static int app_read_ind(
   uint32_t                arep,
   uint16_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint16_t                idx,
   uint16_t                sequence_number,
   uint8_t                 **pp_read_data,
   uint16_t                *p_read_length,
   pnet_result_t           *p_result)
{
   LOG_INFO(PNET_LOG, "APP: read call-back: idx %u  Max length %u\n",
         (unsigned)idx, (unsigned)*p_read_length);

   if ((idx == APP_PARAM_IDX_1) &&
       (*p_read_length >= sizeof(app_param_1)))
   {
      *pp_read_data = (uint8_t*)&app_param_1;
      *p_read_length = sizeof(app_param_1);
   }
   if ((idx == APP_PARAM_IDX_2) &&
       (*p_read_length >= sizeof(app_param_2)))
   {
      *pp_read_data = (uint8_t*)&app_param_2;
      *p_read_length = sizeof(app_param_2);
   }

   return 0;
}

static int app_state_ind(
   uint32_t                arep,
   pnet_event_values_t     state)
{
   uint16_t                err_cls = 0;
   uint16_t                err_code = 0;
   uint16_t                slot = 0;

   LOG_DEBUG(PNET_LOG, "APP: state call-back %d\n", state);

   if (state == PNET_EVENT_ABORT)
   {
      if (pnet_get_ar_error_codes(arep, &err_cls, &err_code) == 0)
      {
         printf("ABORT err_cls %u  err_code %u\n", (unsigned)err_cls, (unsigned)err_code);
      }
      else
      {
         printf("ABORT: No error status available\n");
      }
      /* Only abort AR with correct session key */
      os_event_set(main_events, EVENT_ABORT);
   }
   else if (state == PNET_EVENT_PRMEND)
   {
      LOG_DEBUG(PNET_LOG, "APP: Callback on event PNET_EVENT_PRMEND\n");

      /* Save the arep for later use */
      main_arep = arep;
      os_event_set(main_events, EVENT_READY_FOR_DATA);

      /* Set IOPS for DAP slot (has same numbering as the module identifiers) */
      (void)pnet_input_set_data_and_iops(APP_API, PNET_SLOT_DAP_IDENT, PNET_SUBMOD_DAP_IDENT,                    NULL, 0, PNET_IOXS_GOOD);
      (void)pnet_input_set_data_and_iops(APP_API, PNET_SLOT_DAP_IDENT, PNET_SUBMOD_DAP_INTERFACE_1_IDENT,        NULL, 0, PNET_IOXS_GOOD);
      (void)pnet_input_set_data_and_iops(APP_API, PNET_SLOT_DAP_IDENT, PNET_SUBMOD_DAP_INTERFACE_1_PORT_0_IDENT, NULL, 0, PNET_IOXS_GOOD);

      /* Set initial data and IOPS for custom input modules, and IOCS for custom output modules */
      for (slot = 0; slot < PNET_MAX_MODULES; slot++)
      {
         if (custom_input_slots[slot] == true)
         {
            LOG_DEBUG(PNET_LOG, "APP:   Setting input data and IOPS for slot %u subslot %u\n", slot, PNET_SUBMOD_CUSTOM_IDENT);
            (void)pnet_input_set_data_and_iops(APP_API, slot, PNET_SUBMOD_CUSTOM_IDENT, inputdata,  sizeof(inputdata), PNET_IOXS_GOOD);
         }
         if (custom_output_slots[slot] == true)
         {
            LOG_DEBUG(PNET_LOG, "APP:   Setting output IOCS for slot %u subslot %u\n", slot, PNET_SUBMOD_CUSTOM_IDENT);
            (void)pnet_output_set_iocs(APP_API, slot, PNET_SUBMOD_CUSTOM_IDENT, PNET_IOXS_GOOD);
         }
      }

      (void)pnet_set_provider_state(true);
   }
   else if (state == PNET_EVENT_DATA)
   {
      LOG_DEBUG(PNET_LOG, "APP: Callback on event PNET_EVENT_DATA\n");
   }
   else if (state == PNET_EVENT_STARTUP)
   {
      LOG_DEBUG(PNET_LOG, "APP: Callback on event PNET_EVENT_STARTUP\n");
   }
   else if (state == PNET_EVENT_APPLRDY)
   {
      LOG_DEBUG(PNET_LOG, "APP: Callback on event PNET_EVENT_APPLRDY\n");
   }

   return 0;
}

static int app_exp_module_ind(
   uint16_t                api,
   uint16_t                slot,
   uint32_t                module_ident)
{
   int                     ret = -1;   /* Not supported in specified slot */
   uint16_t                ix;

   /* Find it in the list of supported modules */
   ix = 0;
   while ((ix < NELEMENTS(cfg_available_module_types)) &&
          (cfg_available_module_types[ix] != module_ident))
   {
      ix++;
   }

   if (ix < NELEMENTS(cfg_available_module_types))
   {
      if (pnet_pull_module(api, slot) != 0)
      {
         LOG_DEBUG(PNET_LOG, "APP: Pull module failed\n");
      }
      /* For now support any module in any slot */
      ret = pnet_plug_module(api, slot, module_ident);

      if (ret != 0)
      {
         LOG_DEBUG(PNET_LOG, "APP: Plug module failed. Ret: %u API: %u Slot: %u Module ID: 0x%x Index in list of supported modules: %u\n", ret, api, slot, (unsigned)module_ident, ix);
      }
      else
      {
         /* Remember what is plugged in each slot */
         if (slot < PNET_MAX_MODULES)
         {
            if (module_ident == PNET_MOD_8_0_IDENT || module_ident == PNET_MOD_8_8_IDENT)
            {
               custom_input_slots[slot] = true;
            }
            if (module_ident == PNET_MOD_8_8_IDENT || module_ident == PNET_MOD_0_8_IDENT)
            {
               custom_output_slots[slot] = true;
            }
         }
         else
         {
            LOG_DEBUG(PNET_LOG, "APP: Wrong slot number recieved: %u  It should be less than %u\n", slot, PNET_MAX_MODULES);
         }
      }
   }

   LOG_DEBUG(PNET_LOG, "APP: ret %d expected module call-back 0x%08x\n", ret, (unsigned)module_ident);

   return ret;
}

static int app_exp_submodule_ind(
   uint16_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint32_t                module_ident,
   uint32_t                submodule_ident)
{
   int                     ret = -1;
   uint16_t                ix = 0;

   /* Find it in the list of supported submodules */
   ix = 0;
   while ((ix < NELEMENTS(cfg_available_submodule_types)) &&
          ((cfg_available_submodule_types[ix].module_ident_nbr != module_ident) ||
           (cfg_available_submodule_types[ix].submodule_ident_nbr != submodule_ident)))
   {
      ix++;
   }

   if (ix < NELEMENTS(cfg_available_submodule_types))
   {
      if (pnet_pull_submodule(api, slot, subslot) != 0)
      {
         LOG_DEBUG(PNET_LOG, "APP: Pull sub-module failed\n");
      }
      ret = pnet_plug_submodule(api, slot, subslot, module_ident, submodule_ident,
         cfg_available_submodule_types[ix].data_dir,
         cfg_available_submodule_types[ix].insize, cfg_available_submodule_types[ix].outsize);
   }

   LOG_DEBUG(PNET_LOG, "APP: ret %d expected submodule call-back module ident 0x%08x, submodule ident 0x%08x\n", ret, (unsigned)module_ident, (unsigned)submodule_ident);

   return ret;
}

static int app_new_data_status_ind(
   uint32_t                arep,
   uint32_t                crep,
   uint8_t                 changes)
{
   LOG_DEBUG(PNET_LOG, "APP: New data status 0x%02x\n", changes);

   return 0;
}

static int app_alarm_ind(
   uint32_t                arep,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint16_t                data_len,
   uint16_t                data_usi,
   uint8_t                 *p_data)
{
   LOG_DEBUG(PNET_LOG, "APP: Alarm indicated\n");

   os_event_set(main_events, EVENT_ALARM);

   return 0;
}

static int app_alarm_cnf(
   uint32_t                arep,
   pnet_pnio_status_t      *p_pnio_status)
{
   LOG_DEBUG(PNET_LOG, "APP: Alarm confirmed: %u, %u, %u, %u\n",
      p_pnio_status->error_code, p_pnio_status->error_decode,
      p_pnio_status->error_code_1, p_pnio_status->error_code_2);

   alarm_allowed = true;

   return 0;
}

static int app_alarm_ack_cnf(
   uint32_t                arep,
   int                     res)
{
   LOG_DEBUG(PNET_LOG, "APP: Alarm ACK confirmed: %d\n", res);

   return 0;
}

/************************* Utilities ******************************************/

static int _cmd_pnio_alarm_ack(
      int                  argc,
      char                 *argv[])
{
#if 1
   /* Make it synchronous to periodic */
   os_event_set(main_events, EVENT_ALARM);
#else
   int                     ret;
   pnet_pnio_status_t      pnio_status = {0,0,0,0};

   if (main_arep != UINT32_MAX)
   {
      ret = pnet_alarm_send_ack(main_arep, &pnio_status);
      if (ret != 0)
      {
         LOG_ERROR(PNET_LOG, "APP: Alarm ACK error %d\n", ret);
      }
      else
      {
         LOG_INFO(PNET_LOG, "APP: Alarm ACK sent\n");
      }
   }
   else
   {
      LOG_INFO(PNET_LOG, "APP: No AREP\n");
   }
#endif
   return 0;
}

const shell_cmd_t cmd_pnio_alarm_ack =
{
      .cmd = _cmd_pnio_alarm_ack,
      .name = "pnio_alarm_ack",
      .help_short = "Send AlarmAck",
      .help_long ="Send AlarmAck"
};
SHELL_CMD (cmd_pnio_alarm_ack);

static int _cmd_pnio_run(
      int                  argc,
      char                 *argv[])
{
   uint16_t                ix;

   strcpy(pnet_default_cfg.im_0_data.order_id, "12345");
   strcpy(pnet_default_cfg.im_0_data.im_serial_number, "00001");
   pnet_default_cfg.ip_addr.a = ip4_addr1 (&netif_default->ip_addr);
   pnet_default_cfg.ip_addr.b = ip4_addr2 (&netif_default->ip_addr);
   pnet_default_cfg.ip_addr.c = ip4_addr3 (&netif_default->ip_addr);
   pnet_default_cfg.ip_addr.d = ip4_addr4 (&netif_default->ip_addr);
   pnet_default_cfg.ip_mask.a = ip4_addr1 (&netif_default->netmask);
   pnet_default_cfg.ip_mask.b = ip4_addr2 (&netif_default->netmask);
   pnet_default_cfg.ip_mask.c = ip4_addr3 (&netif_default->netmask);
   pnet_default_cfg.ip_mask.d = ip4_addr4 (&netif_default->netmask);
   pnet_default_cfg.ip_gateway.a = pnet_default_cfg.ip_addr.a;
   pnet_default_cfg.ip_gateway.b = pnet_default_cfg.ip_addr.b;
   pnet_default_cfg.ip_gateway.c = pnet_default_cfg.ip_addr.c;
   pnet_default_cfg.ip_gateway.d = 1;
   memcpy (pnet_default_cfg.eth_addr.addr, netif_default->hwaddr, sizeof(pnet_ethaddr_t));

   if (pnet_init(APP_DEFAULT_ETHERNET_INTERFACE, TICK_INTERVAL_US, &pnet_default_cfg) == 0)
   {
      LOG_DEBUG(PNET_LOG, "APP: Call-backs connected\n");

      if (init_done == true)
      {
         os_timer_stop(main_timer);
      }
      os_timer_start(main_timer);
      init_done = true;
   }
   else
   {
      LOG_DEBUG(PNET_LOG, "APP: Could not connect application call-backs\n");
   }

   return 0;
}

const shell_cmd_t cmd_pnio_run =
{
      .cmd = _cmd_pnio_run,
      .name = "pnio_run",
      .help_short = "Exec pnio",
      .help_long ="Exec pnio"
};
SHELL_CMD (cmd_pnio_run);

static int _cmd_pnio_show(
      int                  argc,
      char                 *argv[])
{
   unsigned long level = 0;
   char *end = NULL;

   if (argc == 2)
   {
      level = strtoul(argv[1], &end, 0);
   }
   pnet_show(level);
   printf("ar_param_1 = 0x%08x\n", (unsigned)ntohl(app_param_1));
   printf("ar_param_2 = 0x%08x\n", (unsigned)ntohl(app_param_2));

   return 0;
}

const shell_cmd_t cmd_pnio_show =
{
  .cmd = _cmd_pnio_show,
  .name = "pnio_show",
  .help_short = "Show pnio (level)",
  .help_long ="Show pnio (level)"
};
SHELL_CMD (cmd_pnio_show);

static void main_timer_tick(
   os_timer_t              *timer,
   void                    *arg)
{
   os_event_set(main_events, EVENT_TIMER);
}


/****************************** Main ******************************************/

int main(void)
{
   int            ret = -1;
   uint32_t       mask = EVENT_READY_FOR_DATA | EVENT_TIMER | EVENT_ALARM | EVENT_ABORT;
   uint32_t       flags = 0;
   bool           button1_pressed = false;
   bool           button2_pressed = false;
   bool           button2_pressed_previous = false;
   bool           led_state = false;
   bool           received_led_state = false;
   uint32_t       tick_ctr_buttons = 0;
   uint32_t       tick_ctr_update_data = 0;
   static uint8_t data_ctr = 0;
   uint16_t       slot = 0;
   uint8_t        outputdata[64];
   uint8_t        outputdata_iops;
   uint16_t       outputdata_length;
   bool           outputdata_is_updated = false;
   uint8_t        alarm_payload[APP_ALARM_PAYLOAD_SIZE] = { 0 };

   LOG_INFO(PNET_LOG, "APP: Starting\n");
   main_events = os_event_create();
   main_timer  = os_timer_create(TICK_INTERVAL_US, main_timer_tick, NULL, false);

   /* Main loop */
   for (;;)
   {
      os_event_wait(main_events, mask, &flags, OS_WAIT_FOREVER);
      if (flags & EVENT_READY_FOR_DATA)
      {
         os_event_clr(main_events, EVENT_READY_FOR_DATA); /* Re-arm */
         LOG_DEBUG(PNET_LOG, "APP: Send appl ready to profinet stack.\n");
         ret = pnet_application_ready(main_arep);
         if (ret != 0)
         {
            LOG_ERROR(PNET_LOG, "APP: Error returned when application telling that it is ready for data. Have you set IOCS or IOPS for all subslots?\n");
         }

         /*
          * cm_ccontrol_cnf(+/-) is indicated later (app_state_ind(DATA)), when the
          * confirmation arrives from the controller.
          */
      }
      else if (flags & EVENT_ALARM)
      {
         pnet_pnio_status_t      pnio_status = { 0,0,0,0 };

         os_event_clr(main_events, EVENT_ALARM); /* Re-arm */

         ret = pnet_alarm_send_ack(main_arep, &pnio_status);
         if (ret != 0)
         {
            LOG_ERROR(PNET_LOG, "APP: Alarm ACK error %d\n", ret);
         }
         else
         {
            LOG_INFO(PNET_LOG, "APP: Alarm ACK sent\n");
         }
      }
      else if (flags & EVENT_TIMER)
      {
         os_event_clr(main_events, EVENT_TIMER); /* Re-arm */
         tick_ctr_buttons++;
         tick_ctr_update_data++;

         /* Read buttons every 100 ms */
         if ((main_arep != UINT32_MAX) && (tick_ctr_buttons > 100))
         {
            tick_ctr_buttons = 0;

            os_get_button(0, &button1_pressed);
            os_get_button(1, &button2_pressed);
         }

         /* Set input and output data every 10ms */
         if ((main_arep != UINT32_MAX) && (tick_ctr_update_data > 10))
         {
            tick_ctr_update_data = 0;

            /* Input data (for sending to IO-controller) */
            // Lowest 7 bits: Counter    Most significant bit: Button1
            inputdata[0] = data_ctr++;
            if (button1_pressed)
            {
               inputdata[0] |= 0x80;
            }
            else
            {
               inputdata[0] &= 0x7F;
            }

            /* Set data for custom input modules, if any */
            for (slot = 0; slot < PNET_MAX_MODULES; slot++)
            {
               if (custom_input_slots[slot] == true)
               {
                  (void)pnet_input_set_data_and_iops(APP_API, slot, PNET_SUBMOD_CUSTOM_IDENT, inputdata,  sizeof(inputdata), PNET_IOXS_GOOD);
               }
            }

            /* Read data from first of the custom output modules, if any */
            for (slot = 0; slot < PNET_MAX_MODULES; slot++)
            {
               if (custom_output_slots[slot] == true)
               {
                  outputdata_length = sizeof(outputdata);
                  pnet_output_get_data_and_iops(APP_API, slot, PNET_SUBMOD_CUSTOM_IDENT, &outputdata_is_updated, outputdata, &outputdata_length, &outputdata_iops);
                  break;
               }
            }

            /* Set LED state */
            if (outputdata_is_updated == true && outputdata_iops == PNET_IOXS_GOOD)
            {
               if (outputdata_length == APP_DATASIZE_OUTPUT)
               {
                  received_led_state = (outputdata[0] & 0x80) > 0;  /* Use most significant bit */
                  os_set_led(APP_LED_ID, received_led_state);
               }
               else
               {
                  LOG_INFO(PNET_LOG, "APP: Wrong outputdata length: %u\n", outputdata_length);
               }
            }
         }

         /* Create an alarm on first input slot (if any) when button 2 is pressed/released */
         if (main_arep != UINT32_MAX)
         {
            if ((button2_pressed == true) && (button2_pressed_previous == false) && (alarm_allowed == true))
            {
               alarm_payload[0]++;
               for (slot = 0; slot < PNET_MAX_MODULES; slot++)
               {
                  if (custom_input_slots[slot] == true)
                  {
                     printf("Sending process alarm from slot %u subslot %u to IO-controller. Payload: 0x%x\n",
                        slot,
                        PNET_SUBMOD_CUSTOM_IDENT,
                        alarm_payload[0]);
                     pnet_alarm_send_process_alarm(
                        main_arep,
                        APP_API,
                        slot,
                        PNET_SUBMOD_CUSTOM_IDENT,
                        APP_ALARM_USI,
                        sizeof(alarm_payload),
                        alarm_payload);
                     break;
                  }
               }
            }
            button2_pressed_previous = button2_pressed;
         }

         pnet_handle_periodic();
      }
      else if (flags & EVENT_ABORT)
      {
         /* Reset main */
         main_arep = UINT32_MAX;
         alarm_allowed = true;
         button1_pressed = false;
         button2_pressed = false;

         os_event_clr(main_events, EVENT_ABORT); /* Re-arm */
         LOG_DEBUG(PNET_LOG, "APP: abort\n");
      }
   }
   os_timer_destroy(main_timer);
   os_event_destroy(main_events);

   LOG_INFO(PNET_LOG, "APP: Exiting\n");

   return 0;
}
