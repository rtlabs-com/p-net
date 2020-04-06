#include <string.h>

#include <kern.h>
#include <shell.h>

#include <pnet_api.h>
#include "osal.h"
#include "log.h"


#include <lwip/netif.h>

/********************* Call-back function declarations ************************/

static int app_exp_module_ind(pnet_t *net, void *arg, uint16_t api, uint16_t slot, uint32_t module_ident_number);
static int app_exp_submodule_ind(pnet_t *net, void *arg, uint16_t api, uint16_t slot, uint16_t subslot, uint32_t module_ident_number, uint32_t submodule_ident_number);
static int app_new_data_status_ind(pnet_t *net, void *arg, uint32_t arep, uint32_t crep, uint8_t changes, uint8_t data_status);
static int app_connect_ind(pnet_t *net, void *arg, uint32_t arep, pnet_result_t *p_result);
static int app_state_ind(pnet_t *net, void *arg, uint32_t arep, pnet_event_values_t state);
static int app_release_ind(pnet_t *net, void *arg, uint32_t arep, pnet_result_t *p_result);
static int app_dcontrol_ind(pnet_t *net, void *arg, uint32_t arep, pnet_control_command_t control_command, pnet_result_t *p_result);
static int app_ccontrol_cnf(pnet_t *net, void *arg, uint32_t arep, pnet_result_t *p_result);
static int app_write_ind(pnet_t *net, void *arg, uint32_t arep, uint16_t api, uint16_t slot, uint16_t subslot, uint16_t idx, uint16_t sequence_number, uint16_t write_length, uint8_t *p_write_data, pnet_result_t *p_result);
static int app_read_ind(pnet_t *net, void *arg, uint32_t arep, uint16_t api, uint16_t slot, uint16_t subslot, uint16_t idx, uint16_t sequence_number, uint8_t **pp_read_data, uint16_t *p_read_length, pnet_result_t *p_result);
static int app_alarm_cnf(pnet_t *net, void *arg, uint32_t arep, pnet_pnio_status_t *p_pnio_status);
static int app_alarm_ind(pnet_t *net, void *arg, uint32_t arep, uint32_t api, uint16_t slot, uint16_t subslot, uint16_t data_len, uint16_t data_usi, uint8_t *p_data);
static int app_alarm_ack_cnf(pnet_t *net, void *arg, uint32_t arep, int res);
static int app_reset_ind(pnet_t  *net, void *arg, bool should_reset_application, uint16_t reset_mode);
void print_bytes(uint8_t *bytes, int32_t len);

/********************** Settings **********************************************/

#define EVENT_READY_FOR_DATA           BIT(0)
#define EVENT_TIMER                    BIT(1)
#define EVENT_ALARM                    BIT(2)
#define EVENT_ABORT                    BIT(15)

#define EXIT_CODE_ERROR                1
#define TICK_INTERVAL_US               1000        /* 1 ms */
#define APP_DEFAULT_ETHERNET_INTERFACE "en1"
#define APP_ALARM_USI                  1
#define APP_LED_ID                     0           /* "LED1" on circuit board */


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

static const uint32_t            cfg_available_module_types[] =
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

/************************ App data storage ***********************************/

struct cmd_args {
   char path_led[256];
   char path_button1[256];
   char path_button2[256];
   char station_name[64];
   char eth_interface[64];
   int  verbosity;
};

typedef struct app_data_obj
{
   os_timer_t                *main_timer;
   os_event_t                *main_events;
   uint32_t                  main_arep;
   bool                      alarm_allowed;
   struct cmd_args           arguments;
   uint32_t                  app_param_1;
   uint32_t                  app_param_2;
   uint8_t                   inputdata[APP_DATASIZE_INPUT];
   uint8_t                   custom_input_slots[PNET_MAX_MODULES];
   uint8_t                   custom_output_slots[PNET_MAX_MODULES];
   bool                      init_done;
} app_data_t;


typedef struct app_data_and_stack_obj
{
   app_data_t           *appdata;
   pnet_t               *net;
} app_data_and_stack_t;


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
      .reset_cb = app_reset_ind,
      .cb_arg = NULL,

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
      .station_name = "",   /* Override by command line argument */
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
      .send_hello = 1,                    /* Send HELLO */
      .dhcp_enable = 0,
      .ip_addr = { 0 },                   /* Read from kernel */
      .ip_mask = { 0 },                   /* Read from kernel */
      .ip_gateway = { 0 },                /* Read from kernel */
      .eth_addr = { 0 },                  /* Read from kernel */
};


/********************************** Globals ***********************************/

static app_data_t                  *gp_appdata;
static pnet_t                      *g_net;

/*********************************** Callbacks ********************************/

static int app_connect_ind(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   pnet_result_t           *p_result)
{
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Connect call-back. AREP: %u  Status codes: %d %d %d %d\n",
         arep,
         p_result->pnio_status.error_code,
         p_result->pnio_status.error_decode,
         p_result->pnio_status.error_code_1,
         p_result->pnio_status.error_code_2);
   }
   /*
    *  Handle the request on an application level.
    *  This is a very simple application which does not need to handle anything.
    *  All the needed information is in the AR data structure.
    */

   return 0;
}

static int app_release_ind(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   pnet_result_t           *p_result)
{
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Release (disconnect) call-back. AREP: %u  Status codes: %d %d %d %d\n",
         arep,
         p_result->pnio_status.error_code,
         p_result->pnio_status.error_decode,
         p_result->pnio_status.error_code_1,
         p_result->pnio_status.error_code_2);
   }

   return 0;
}

static int app_dcontrol_ind(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   pnet_control_command_t  control_command,
   pnet_result_t           *p_result)
{
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Dcontrol call-back. AREP: %u  Command: %d  Status codes: %d %d %d %d\n",
         arep,
         control_command,
         p_result->pnio_status.error_code,
         p_result->pnio_status.error_decode,
         p_result->pnio_status.error_code_1,
         p_result->pnio_status.error_code_2);
   }

   return 0;
}

static int app_ccontrol_cnf(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   pnet_result_t           *p_result)
{
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Ccontrol confirmation call-back. AREP: %u  Status codes: %d %d %d %d\n",
         arep,
         p_result->pnio_status.error_code,
         p_result->pnio_status.error_decode,
         p_result->pnio_status.error_code_1,
         p_result->pnio_status.error_code_2);
   }

   return 0;
}

static int app_write_ind(
   pnet_t                  *net,
   void                    *arg,
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
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Parameter write call-back. AREP: %u API: %u Slot: %u Subslot: %u Index: %u Sequence: %u Length: %u\n",
         arep,
         api,
         slot,
         subslot,
         (unsigned)idx,
         sequence_number,
         write_length);
   }
   if (idx == APP_PARAM_IDX_1)
   {
      if (write_length == sizeof(p_appdata->app_param_1))
      {
         memcpy(&p_appdata->app_param_1, p_write_data, sizeof(p_appdata->app_param_1));
         if (p_appdata->arguments.verbosity > 0)
         {
            print_bytes(p_write_data, sizeof(p_appdata->app_param_1));
         }
      }
      else
      {
         printf("Wrong length in write call-back. Index: %u Length: %u Expected length: %u\n",
            (unsigned)idx,
            (unsigned)write_length,
            (unsigned)sizeof(p_appdata->app_param_1));
      }
   }
   else if (idx == APP_PARAM_IDX_2)
   {
      if (write_length == sizeof(p_appdata->app_param_2))
      {
         memcpy(&p_appdata->app_param_2, p_write_data, sizeof(p_appdata->app_param_2));
         if (p_appdata->arguments.verbosity > 0)
         {
            print_bytes(p_write_data, sizeof(p_appdata->app_param_2));
         }
      }
      else
      {
         printf("Wrong length in write call-back. Index: %u Length: %u Expected length: %u\n",
            (unsigned)idx,
            (unsigned)write_length,
            (unsigned)sizeof(p_appdata->app_param_2));
      }
   }
   else
   {
      printf("Wrong index in write call-back: %u\n", (unsigned)idx);
   }

   return 0;
}

static int app_read_ind(
   pnet_t                  *net,
   void                    *arg,
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
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Parameter read call-back. AREP: %u API: %u Slot: %u Subslot: %u Index: %u Sequence: %u  Max length: %u\n",
         arep,
         api,
         slot,
         subslot,
         (unsigned)idx,
         sequence_number,
         (unsigned)*p_read_length);
   }
   if ((idx == APP_PARAM_IDX_1) &&
       (*p_read_length >= sizeof(p_appdata->app_param_1)))
   {
      *pp_read_data = (uint8_t*)&p_appdata->app_param_1;
      *p_read_length = sizeof(p_appdata->app_param_1);
      if (p_appdata->arguments.verbosity > 0)
      {
         print_bytes(*pp_read_data, *p_read_length);
      }
   }
   if ((idx == APP_PARAM_IDX_2) &&
       (*p_read_length >= sizeof(p_appdata->app_param_2)))
   {
      *pp_read_data = (uint8_t*)&p_appdata->app_param_2;
      *p_read_length = sizeof(p_appdata->app_param_2);
      if (p_appdata->arguments.verbosity > 0)
      {
         print_bytes(*pp_read_data, *p_read_length);
      }
   }

   return 0;
}

static int app_state_ind(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   pnet_event_values_t     state)
{
   uint16_t                err_cls = 0;
   uint16_t                err_code = 0;
   uint16_t                slot = 0;
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (state == PNET_EVENT_ABORT)
   {
      if (pnet_get_ar_error_codes(net, arep, &err_cls, &err_code) == 0)
      {
         if (p_appdata->arguments.verbosity > 0)
         {
               printf("Callback on event PNET_EVENT_ABORT. Error class: %u Error code: %u\n",
                  (unsigned)err_cls, (unsigned)err_code);
         }
      }
      else
      {
         if (p_appdata->arguments.verbosity > 0)
         {
               printf("Callback on event PNET_EVENT_ABORT. No error status available\n");
         }
      }
      /* Only abort AR with correct session key */
      os_event_set(p_appdata->main_events, EVENT_ABORT);
   }
   else if (state == PNET_EVENT_PRMEND)
   {
      if (p_appdata->arguments.verbosity > 0)
      {
         printf("Callback on event PNET_EVENT_PRMEND. AREP: %u\n", arep);
      }

      /* Save the arep for later use */
      p_appdata->main_arep = arep;
      os_event_set(p_appdata->main_events, EVENT_READY_FOR_DATA);

      /* Set IOPS for DAP slot (has same numbering as the module identifiers) */
      (void)pnet_input_set_data_and_iops(net, APP_API, PNET_SLOT_DAP_IDENT, PNET_SUBMOD_DAP_IDENT,                    NULL, 0, PNET_IOXS_GOOD);
      (void)pnet_input_set_data_and_iops(net, APP_API, PNET_SLOT_DAP_IDENT, PNET_SUBMOD_DAP_INTERFACE_1_IDENT,        NULL, 0, PNET_IOXS_GOOD);
      (void)pnet_input_set_data_and_iops(net, APP_API, PNET_SLOT_DAP_IDENT, PNET_SUBMOD_DAP_INTERFACE_1_PORT_0_IDENT, NULL, 0, PNET_IOXS_GOOD);

      /* Set initial data and IOPS for custom input modules, and IOCS for custom output modules */
      for (slot = 0; slot < PNET_MAX_MODULES; slot++)
      {
         if (p_appdata->custom_input_slots[slot] == true)
         {
            if (p_appdata->arguments.verbosity > 0)
            {
               printf("  Setting input data and IOPS for slot %u subslot %u\n", slot, PNET_SUBMOD_CUSTOM_IDENT);
            }
            (void)pnet_input_set_data_and_iops(net, APP_API, slot, PNET_SUBMOD_CUSTOM_IDENT, p_appdata->inputdata,  sizeof(p_appdata->inputdata), PNET_IOXS_GOOD);
         }
         if (p_appdata->custom_output_slots[slot] == true)
         {
            if (p_appdata->arguments.verbosity > 0)
            {
               printf("  Setting output IOCS for slot %u subslot %u\n", slot, PNET_SUBMOD_CUSTOM_IDENT);
            }
            (void)pnet_output_set_iocs(net, APP_API, slot, PNET_SUBMOD_CUSTOM_IDENT, PNET_IOXS_GOOD);
         }
      }

      (void)pnet_set_provider_state(net, true);
   }
   else if (state == PNET_EVENT_DATA)
   {
      if (p_appdata->arguments.verbosity > 0)
      {
         printf("Callback on event PNET_EVENT_DATA\n");
      }
   }
   else if (state == PNET_EVENT_STARTUP)
   {
      if (p_appdata->arguments.verbosity > 0)
      {
         printf("Callback on event PNET_EVENT_STARTUP\n");
      }
   }
   else if (state == PNET_EVENT_APPLRDY)
   {
      if (p_appdata->arguments.verbosity > 0)
      {
         printf("Callback on event PNET_EVENT_APPLRDY\n");
      }
   }

   return 0;
}

static int app_reset_ind(
   pnet_t                  *net,
   void                    *arg,
   bool                    should_reset_application,
   uint16_t                reset_mode)
{
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Reset call-back. Application reset mandatory: %u  Reset mode: %d\n",
         should_reset_application,
         reset_mode);
   }

   return 0;
}

static int app_exp_module_ind(
   pnet_t                  *net,
   void                    *arg,
   uint16_t                api,
   uint16_t                slot,
   uint32_t                module_ident)
{
   int                     ret = -1;   /* Not supported in specified slot */
   uint16_t                ix;
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Module plug call-back\n");
   }

   /* Find it in the list of supported modules */
   ix = 0;
   while ((ix < NELEMENTS(cfg_available_module_types)) &&
          (cfg_available_module_types[ix] != module_ident))
   {
      ix++;
   }

   if (ix < NELEMENTS(cfg_available_module_types))
   {
      printf("  Pull old module.    API: %u Slot: 0x%x",
         api,
         slot
      );
      if (pnet_pull_module(net, api, slot) != 0)
      {
         printf("    Slot was empty.\n");
      }
      else
      {
         printf("\n");
      }

      /* For now support any of the known modules in any slot */
      if (p_appdata->arguments.verbosity > 0)
      {
         printf("  Plug module.        API: %u Slot: 0x%x Module ID: 0x%x Index in supported modules: %u\n", api, slot, (unsigned)module_ident, ix);
      }
      ret = pnet_plug_module(net, api, slot, module_ident);
      if (ret != 0)
      {
         printf("Plug module failed. Ret: %u API: %u Slot: %u Module ID: 0x%x Index in list of supported modules: %u\n", ret, api, slot, (unsigned)module_ident, ix);
      }
      else
      {
         /* Remember what is plugged in each slot */
         if (slot < PNET_MAX_MODULES)
         {
            if (module_ident == PNET_MOD_8_0_IDENT || module_ident == PNET_MOD_8_8_IDENT)
            {
               p_appdata->custom_input_slots[slot] = true;
            }
            if (module_ident == PNET_MOD_8_8_IDENT || module_ident == PNET_MOD_0_8_IDENT)
            {
               p_appdata->custom_output_slots[slot] = true;
            }
         }
         else
         {
            printf("Wrong slot number recieved: %u  It should be less than %u\n", slot, PNET_MAX_MODULES);
         }
      }

   }
   else
   {
      printf("  Module ID %08x not found. API: %u Slot: %u\n",
         (unsigned)module_ident,
         api,
         slot);
   }

   return ret;
}

static int app_exp_submodule_ind(
   pnet_t                  *net,
   void                    *arg,
   uint16_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint32_t                module_ident,
   uint32_t                submodule_ident)
{
   int                     ret = -1;
   uint16_t                ix = 0;
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Submodule plug call-back.\n");
   }

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
      printf("  Pull old submodule. API: %u Slot: 0x%x                   Subslot: 0x%x ",
         api,
         slot,
         subslot
      );
      if (pnet_pull_submodule(net, api, slot, subslot) != 0)
      {
         printf("     Subslot was empty.\n");
      } else {
         printf("\n");
      }

      if (p_appdata->arguments.verbosity > 0)
      {
         printf("  Plug submodule.     API: %u Slot: 0x%x Module ID: 0x%-4x Subslot: 0x%x Submodule ID: 0x%x Index in supported submodules: %u Dir: %u In: %u Out: %u bytes\n",
            api,
            slot,
            (unsigned)module_ident,
            subslot,
            (unsigned)submodule_ident,
            ix,
            cfg_available_submodule_types[ix].data_dir,
            cfg_available_submodule_types[ix].insize,
            cfg_available_submodule_types[ix].outsize
            );
      }
      ret = pnet_plug_submodule(net, api, slot, subslot,
         module_ident,
         submodule_ident,
         cfg_available_submodule_types[ix].data_dir,
         cfg_available_submodule_types[ix].insize,
         cfg_available_submodule_types[ix].outsize);
      if (ret != 0)
      {
         printf("  Plug submodule failed. Ret: %u API: %u Slot: %u Subslot 0x%x Module ID: 0x%x Submodule ID: 0x%x Index in list of supported modules: %u\n",
            ret,
            api,
            slot,
            subslot,
            (unsigned)module_ident,
            (unsigned)submodule_ident,
            ix);
      }
   }
   else
   {
      printf("  Submodule ID 0x%x in module ID 0x%x not found. API: %u Slot: %u Subslot %u \n",
         (unsigned)submodule_ident,
         (unsigned)module_ident,
         api,
         slot,
         subslot);
   }

   return ret;
}

static int app_new_data_status_ind(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   uint32_t                crep,
   uint8_t                 changes,
   uint8_t                 data_status)
{
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("New data status callback. AREP: %u  Status changes: 0x%02x  Status: 0x%02x\n", arep, changes, data_status);
   }

   return 0;
}

static int app_alarm_ind(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint16_t                data_len,
   uint16_t                data_usi,
   uint8_t                 *p_data)
{
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Alarm indicated callback. AREP: %u  API: %d  Slot: %d  Subslot: %d  Length: %d  USI: %d",
         arep,
         api,
         slot,
         subslot,
         data_len,
         data_usi);
   }
   os_event_set(p_appdata->main_events, EVENT_ALARM);

   return 0;
}

static int app_alarm_cnf(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   pnet_pnio_status_t      *p_pnio_status)
{
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Alarm confirmed (by controller) callback. AREP: %u  Status code %u, %u, %u, %u\n",
         arep,
         p_pnio_status->error_code,
         p_pnio_status->error_decode,
         p_pnio_status->error_code_1,
         p_pnio_status->error_code_2);
   }
   p_appdata->alarm_allowed = true;

   return 0;
}

static int app_alarm_ack_cnf(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   int                     res)
{
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Alarm ACK confirmation (from controller) callback. AREP: %u  Result: %d\n", arep, res);
   }

   return 0;
}

/************************* Utilities ******************************************/

/**
 * Print contents of a buffer
 *
 * @param bytes      In: inputbuffer
 * @param len        In: Number of bytes to print
*/
void print_bytes(uint8_t *bytes, int32_t len)
{
   printf("  Bytes: ");
   for (int i = 0; i < len; i++)
   {
      printf("%02X ", bytes[i]);
   }
   printf("\n");
}

static int _cmd_pnio_alarm_ack(
      int                  argc,
      char                 *argv[])
{
#if 1
   /* Make it synchronous to periodic */
   os_event_set(gp_appdata->main_events, EVENT_ALARM);
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
   strcpy(pnet_default_cfg.station_name, gp_appdata->arguments.station_name);
   pnet_default_cfg.cb_arg = (void*)gp_appdata;

   g_net = pnet_init(gp_appdata->arguments.eth_interface, TICK_INTERVAL_US, &pnet_default_cfg);
   if (g_net != NULL)
   {
      if (gp_appdata->arguments.verbosity > 0)
      {
         printf("Initialized p-net application.\n\n");
         printf("Waiting for connect request from IO-controller\n");
      }

      if (gp_appdata->init_done == true)
      {
         os_timer_stop(gp_appdata->main_timer);
      }
      os_timer_start(gp_appdata->main_timer);
      gp_appdata->init_done = true;
   }
   else
   {
      printf("Failed to initialize p-net application.\n");
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
   pnet_show(g_net, level);
   printf("ar_param_1 = 0x%08x\n", (unsigned)ntohl(gp_appdata->app_param_1));
   printf("ar_param_2 = 0x%08x\n", (unsigned)ntohl(gp_appdata->app_param_2));

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
   os_event_set(gp_appdata->main_events, EVENT_TIMER);
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
   struct cmd_args cmdline_arguments;
   app_data_t     appdata;

   g_net = NULL;

   strcpy(cmdline_arguments.eth_interface, APP_DEFAULT_ETHERNET_INTERFACE);
   strcpy(cmdline_arguments.station_name, APP_DEFAULT_STATION_NAME);
   cmdline_arguments.verbosity = (LOG_LEVEL <= LOG_LEVEL_WARNING) ? 1 : 0;

   gp_appdata = &appdata;

   appdata.app_param_1 = 0;
   appdata.app_param_2 = 0;
   appdata.alarm_allowed = true;
   appdata.main_arep = UINT32_MAX;
   appdata.main_events = NULL;
   appdata.main_timer = NULL;
   appdata.init_done = false;
   memset(appdata.inputdata, 0, sizeof(appdata.inputdata));
   memset(appdata.custom_input_slots, 0, sizeof(appdata.custom_input_slots));
   memset(appdata.custom_output_slots, 0, sizeof(appdata.custom_output_slots));
   appdata.arguments = cmdline_arguments;

   appdata.main_events = os_event_create();
   appdata.main_timer = os_timer_create(TICK_INTERVAL_US, main_timer_tick, NULL, false);

   printf("\n** Profinet demo application ready to start **\n");
   if (appdata.arguments.verbosity > 0)
   {
      printf("Number of slots:     %u (incl slot for DAP module)\n", PNET_MAX_MODULES);
      printf("P-net log level:     %u (DEBUG=0, ERROR=3)\n", LOG_LEVEL);
      printf("App verbosity level: %u\n", appdata.arguments.verbosity);
      printf("Ethernet interface:  %s\n", appdata.arguments.eth_interface);
      printf("Station name:        %s\n", appdata.arguments.station_name);
   }

   /* Main loop */
   for (;;)
   {
      os_event_wait(appdata.main_events, mask, &flags, OS_WAIT_FOREVER);
      if (flags & EVENT_READY_FOR_DATA)
      {
         os_event_clr(appdata.main_events, EVENT_READY_FOR_DATA); /* Re-arm */
         printf("Application will signal that it is ready for data.\n");
         ret = pnet_application_ready(g_net, appdata.main_arep);
         if (ret != 0)
         {
            printf("Error returned when application telling that it is ready for data. Have you set IOCS or IOPS for all subslots?\n");
         }
         /*
          * cm_ccontrol_cnf(+/-) is indicated later (app_state_ind(DATA)), when the
          * confirmation arrives from the controller.
          */
      }
      else if (flags & EVENT_ALARM)
      {
         pnet_pnio_status_t      pnio_status = { 0,0,0,0 };

         os_event_clr(appdata.main_events, EVENT_ALARM); /* Re-arm */

         ret = pnet_alarm_send_ack(g_net, appdata.main_arep, &pnio_status);
         if (ret != 0)
         {
            printf("Error when sending alarm ACK. Error: %d\n", ret);
         }
         else if (appdata.arguments.verbosity > 0)
         {
            printf("Alarm ACK sent\n");
         }


      }
      else if (flags & EVENT_TIMER)
      {
         os_event_clr(appdata.main_events, EVENT_TIMER); /* Re-arm */
         tick_ctr_buttons++;
         tick_ctr_update_data++;

         /* Read buttons every 100 ms */
         if ((appdata.main_arep != UINT32_MAX) && (tick_ctr_buttons > 100))
         {
            tick_ctr_buttons = 0;

            os_get_button(0, &button1_pressed);
            os_get_button(1, &button2_pressed);
         }

         /* Set input and output data every 10ms */
         if ((appdata.main_arep != UINT32_MAX) && (tick_ctr_update_data > 10))
         {
            tick_ctr_update_data = 0;

            /* Input data (for sending to IO-controller) */
            /* Lowest 7 bits: Counter    Most significant bit: Button1 */
            appdata.inputdata[0] = data_ctr++;
            if (button1_pressed)
            {
               appdata.inputdata[0] |= 0x80;
            }
            else
            {
               appdata.inputdata[0] &= 0x7F;
            }

            /* Set data for custom input modules, if any */
            for (slot = 0; slot < PNET_MAX_MODULES; slot++)
            {
               if (appdata.custom_input_slots[slot] == true)
               {
                  (void)pnet_input_set_data_and_iops(g_net, APP_API, slot, PNET_SUBMOD_CUSTOM_IDENT, appdata.inputdata,  sizeof(appdata.inputdata), PNET_IOXS_GOOD);
               }
            }

            /* Read data from first of the custom output modules, if any */
            for (slot = 0; slot < PNET_MAX_MODULES; slot++)
            {
               if (appdata.custom_output_slots[slot] == true)
               {
                  outputdata_length = sizeof(outputdata);
                  pnet_output_get_data_and_iops(g_net, APP_API, slot, PNET_SUBMOD_CUSTOM_IDENT, &outputdata_is_updated, outputdata, &outputdata_length, &outputdata_iops);
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
                 printf("Wrong outputdata length: %u\n", outputdata_length);
               }
            }
         }

         /* Create an alarm on first input slot (if any) when button 2 is pressed/released */
         if (appdata.main_arep != UINT32_MAX)
         {
            if ((button2_pressed == true) && (button2_pressed_previous == false) && (appdata.alarm_allowed == true))
            {
               alarm_payload[0]++;
               for (slot = 0; slot < PNET_MAX_MODULES; slot++)
               {
                  if (appdata.custom_input_slots[slot] == true)
                  {
                     printf("Sending process alarm from slot %u subslot %u to IO-controller. Payload: 0x%x\n",
                        slot,
                        PNET_SUBMOD_CUSTOM_IDENT,
                        alarm_payload[0]);
                     pnet_alarm_send_process_alarm(
                        g_net,
                        appdata.main_arep,
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

         pnet_handle_periodic(g_net);
      }
      else if (flags & EVENT_ABORT)
      {
         /* Reset main */
         appdata.main_arep = UINT32_MAX;
         appdata.alarm_allowed = true;
         button1_pressed = false;
         button2_pressed = false;

         os_event_clr(appdata.main_events, EVENT_ABORT); /* Re-arm */
         if (appdata.arguments.verbosity > 0)
         {
            printf("Aborting the application\n");
         }
      }
   }
   os_timer_destroy(appdata.main_timer);
   os_event_destroy(appdata.main_events);
   printf("Ending the application\n");

   return 0;
}
