#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <pnet_api.h>
#include "osal.h"
#include "log.h"

#define IP_INVALID              0

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
#define APP_DEFAULT_ETHERNET_INTERFACE "eth0"
#define APP_PRIORITY                   15
#define APP_STACKSIZE                  4096        /* bytes */
#define APP_MAIN_SLEEPTIME_US          5000*1000
#define APP_ALARM_USI                  1


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

static void main_timer_tick(
   os_timer_t              *timer,
   void                    *arg)
{
   app_data_t              *p_appdata = (app_data_t*)arg;

   os_event_set(p_appdata->main_events, EVENT_TIMER);
}

void show_usage()
{
   printf("\nDemo application for p-net Profinet device stack.\n");
   printf("\n");
   printf("Wait for connection from IO-controller.\n");
   printf("Then read buttons (input) and send to controller.\n");
   printf("Listen for LED output (from controller) and set LED state.\n");
   printf("It will also send a counter value (useful also without\n");
   printf("buttons and LED).\n");
   printf("Button1 value is sent in the periodic data. Button2 triggers an alarm.\n");
   printf("\n");
   printf("The LED is controlled by writing '1' or '0' to the control file,\n");
   printf("for example /sys/class/gpio/gpio17/value\n");
   printf("A pressed button should be indicated by that the first character in \n");
   printf("the control file is '1'. \n");
   printf("Make sure to activate the appropriate GPIO files by exporting them. \n");
   printf("If no hardware-controlling files in /sys are available, you can\n");
   printf("still try the functionality by using plain text files.\n");
   printf("\n");
   printf("Assumes the default gateway is found on .1 on same subnet as the IP address.\n");
   printf("\n");
   printf("Optional arguments:\n");
   printf("   --help       Show this help text and exit\n");
   printf("   -h           Show this help text and exit\n");
   printf("   -v           Incresase verbosity\n");
   printf("   -i INTERF    Set Ethernet interface name. Defaults to %s\n", APP_DEFAULT_ETHERNET_INTERFACE);
   printf("   -s NAME      Set station name. Defaults to %s\n", APP_DEFAULT_STATION_NAME);
   printf("   -l FILE      Path to control LED. Defaults to not control any LED.\n");
   printf("   -b FILE      Path to read button1. Defaults to not read button1.\n");
   printf("   -d FILE      Path to read button2. Defaults to not read button2.\n");
}


/**
 * Parse command line arguments
 *
 * @param argc      In: Number of arguments
 * @param argv      In: Arguments
 * @return Parsed arguments
*/
struct cmd_args parse_commandline_arguments(int argc, char *argv[])
{
   /* Special handling of long argument */
   if (argc > 1)
   {
      if (strcmp(argv[1], "--help") == 0)
      {
         show_usage();
         exit(EXIT_CODE_ERROR);
      }
   }

   /* Default values */
   struct cmd_args output_arguments;
   strcpy(output_arguments.path_led, "");
   strcpy(output_arguments.path_button1, "");
   strcpy(output_arguments.path_button2, "");
   strcpy(output_arguments.station_name, APP_DEFAULT_STATION_NAME);
   strcpy(output_arguments.eth_interface, APP_DEFAULT_ETHERNET_INTERFACE);
   output_arguments.verbosity = 0;

   int option;
   while ((option = getopt(argc, argv, "hvi:s:l:b:d:")) != -1) {
      switch (option) {
      case 'v':
         output_arguments.verbosity++;
         break;
      case 'i':
         strcpy(output_arguments.eth_interface, optarg);
         break;
      case 's':
         strcpy(output_arguments.station_name, optarg);
         break;
      case 'l':
         strcpy(output_arguments.path_led, optarg);
         break;
      case 'b':
         strcpy(output_arguments.path_button1, optarg);
         break;
      case 'd':
         strcpy(output_arguments.path_button2, optarg);
         break;
      case 'h':
      case '?':
      default:
         show_usage();
         exit(EXIT_CODE_ERROR);
      }
   }

   return output_arguments;
}

/**
 * Check if network interface exists
 *
 * @param name      In: Name of network interface
 * @return true if interface exists
*/
bool does_network_interface_exist(const char* name)
{
   uint32_t index = if_nametoindex(name);

   return index != 0;
}

/**
 * Check if file exists
 *
 * @param filepath      In: Path to file
 * @return true if file exists
*/
bool does_file_exist(const char*  filepath)
{
   struct stat statbuffer;

   return (stat (filepath, &statbuffer) == 0);
}

/**
 * Read a bool from a file
 *
 * @param filepath      In: Path to file
 * @return true if file exists and the first character is '1'
*/
bool read_bool_from_file(const char*  filepath)
{
   FILE *fp;
   char ch;
   int eof_indicator;

   fp = fopen(filepath, "r");
   if (fp == NULL)
   {
      return false;
   }

   ch = fgetc(fp);
   eof_indicator = feof(fp);
   fclose (fp);

   if (eof_indicator)
   {
      return false;
   }
   return ch == '1';
}

/**
 * Write a bool to a file
 *
 * Writes 1 for true and 0 for false. Adds a newline.
 *
 * @param filepath      In: Path to file
 * @param value         In: The value to write
 * @return 0 on success and
 *         -1 if an error occured
*/
int write_bool_to_file(const char*  filepath, bool value)
{
   FILE *fp;
   char* outputstring = value ? "1\n" : "0\n";
   fp = fopen(filepath, "w");
   if (fp == NULL)
   {
      return -1;
   }
   int result = fputs(outputstring, fp);
   fclose (fp);
   if (result == EOF){
      return -1;
   }
   return 0;
}

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

/**
 * Copy an IP address (as an integer) to a struct
 *
 * @param destination_struct  Out: destination
 * @param ip                  In: IP address
*/
void copy_ip_to_struct(pnet_cfg_ip_addr_t* destination_struct, uint32_t ip)
{
   destination_struct->a = (ip & 0xFF);
   destination_struct->b = ((ip >> 8) & 0xFF);
   destination_struct->c = ((ip >> 16) & 0xFF);
   destination_struct->d = ((ip >> 24) & 0xFF);
}

/**
 * Print an IPv4 address (without newline)
 *
 * @param ip      In: IP address
*/
void print_ip_address(uint32_t ip){
   printf("%d.%d.%d.%d",
      (ip & 0xFF),
      ((ip >> 8) & 0xFF),
      ((ip >> 16) & 0xFF),
      ((ip >> 24) & 0xFF)
   );
}

/**
 * Read the IP address as an integer. For IPv4.
 *
 * @param interface_name      In: Name of network interface
 * @return IP address on success and
 *         0 if an error occured
*/
uint32_t read_ip_address(char* interface_name)
{
   int fd;
   struct ifreq ifr;
   uint32_t ip;

   fd = socket (AF_INET, SOCK_DGRAM, 0);
   ifr.ifr_addr.sa_family = AF_INET;
   strncpy (ifr.ifr_name, interface_name, IFNAMSIZ - 1);
   ioctl (fd, SIOCGIFADDR, &ifr);
   ip = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
   close (fd);
   return ip;
}

/**
 * Read the MAC address.
 *
 * @param interface_name      In: Name of network interface
 * @param mac_addr            Out: MAC address
 *
 * @return 0 on success and
 *         -1 if an error occured
*/
int read_mac_address(char *interface_name, pnet_ethaddr_t *mac_addr)
{
   int fd;
   int ret = 0;
   struct ifreq ifr;

   fd = socket (AF_INET, SOCK_DGRAM, 0);

   ifr.ifr_addr.sa_family = AF_INET;
   strncpy (ifr.ifr_name, interface_name, IFNAMSIZ - 1);

   ret = ioctl(fd, SIOCGIFHWADDR, &ifr);
   if (ret == 0){
      memcpy(mac_addr->addr, ifr.ifr_hwaddr.sa_data, 6);
   }
   close (fd);
   return ret;
 }

/**
 * Read the netmask as an integer. For IPv4.
 *
 * @param interface_name      In: Name of network interface
 * @return netmask
*/
uint32_t read_netmask(char* interface_name)
{

   int fd;
   struct ifreq ifr;
   uint32_t netmask;

   fd = socket (AF_INET, SOCK_DGRAM, 0);

   ifr.ifr_addr.sa_family = AF_INET;
   strncpy (ifr.ifr_name, interface_name, IFNAMSIZ - 1);
   ioctl (fd, SIOCGIFNETMASK, &ifr);
   netmask = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
   close (fd);

   return netmask;
}

/**
 * Read the default gateway address as an integer. For IPv4.
 *
 * Assumes the default gateway is found on .1 on same subnet as the IP address.
 *
 * @param interface_name      In: Name of network interface
 * @return netmask
*/
uint32_t read_default_gateway(char* interface_name)
{
   /* TODO Read the actual default gateway (somewhat complicated) */

   uint32_t ip;
   uint32_t gateway;

   ip = read_ip_address(interface_name);
   gateway = (ip & 0x00FFFFFF) | 0x01000000;

   return gateway;
}

void pn_main (void * arg)
{
   app_data_t     *p_appdata;
   app_data_and_stack_t *appdata_and_stack;
   pnet_t         *net;
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


   appdata_and_stack = (app_data_and_stack_t*)arg;
   p_appdata = appdata_and_stack->appdata;
   net = appdata_and_stack->net;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Waiting for connect request from IO-controller\n");
   }

   /* Main loop */
   for (;;)
   {
      os_event_wait(p_appdata->main_events, mask, &flags, OS_WAIT_FOREVER);
      if (flags & EVENT_READY_FOR_DATA)
      {
         os_event_clr(p_appdata->main_events, EVENT_READY_FOR_DATA); /* Re-arm */

         /* Send appl ready to profinet stack. */
         printf("Application will signal that it is ready for data.\n");
         ret = pnet_application_ready(net, p_appdata->main_arep);
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

         os_event_clr(p_appdata->main_events, EVENT_ALARM); /* Re-arm */

         ret = pnet_alarm_send_ack(net, p_appdata->main_arep, &pnio_status);
         if (ret != 0)
         {
            printf("Error when sending alarm ACK. Error: %d\n", ret);
         }
         else if (p_appdata->arguments.verbosity > 0)
         {
            printf("Alarm ACK sent\n");
         }
      }
      else if (flags & EVENT_TIMER)
      {
         os_event_clr(p_appdata->main_events, EVENT_TIMER); /* Re-arm */
         tick_ctr_buttons++;
         tick_ctr_update_data++;

         /* Read buttons every 100 ms */
         if ((p_appdata->main_arep != UINT32_MAX) && (tick_ctr_buttons > 100))
         {
            tick_ctr_buttons = 0;
            if (p_appdata->arguments.path_button1[0] != '\0')
            {
               button1_pressed = read_bool_from_file(p_appdata->arguments.path_button1);
            }
            if (p_appdata->arguments.path_button2[0] != '\0')
            {
               button2_pressed = read_bool_from_file(p_appdata->arguments.path_button2);
            }
         }

         /* Set input and output data every 10ms */
         if ((p_appdata->main_arep != UINT32_MAX) && (tick_ctr_update_data > 10))
         {
            tick_ctr_update_data = 0;

            /* Input data (for sending to IO-controller) */
            /* Lowest 7 bits: Counter    Most significant bit: Button1 */
            p_appdata->inputdata[0] = data_ctr++;
            if (button1_pressed)
            {
               p_appdata->inputdata[0] |= 0x80;
            }
            else
            {
               p_appdata->inputdata[0] &= 0x7F;
            }

            /* Set data for custom input modules, if any */
            for (slot = 0; slot < PNET_MAX_MODULES; slot++)
            {
               if (p_appdata->custom_input_slots[slot] == true)
               {
                  (void)pnet_input_set_data_and_iops(net, APP_API, slot, PNET_SUBMOD_CUSTOM_IDENT, p_appdata->inputdata,  sizeof(p_appdata->inputdata), PNET_IOXS_GOOD);
               }
            }

            /* Output data, for LED output */
            if (p_appdata->arguments.path_led[0] != '\0')
            {

               /* Read data from first of the custom output modules, if any */
               for (slot = 0; slot < PNET_MAX_MODULES; slot++)
               {
                  if (p_appdata->custom_output_slots[slot] == true)
                  {
                     outputdata_length = sizeof(outputdata);
                     pnet_output_get_data_and_iops(net, APP_API, slot, PNET_SUBMOD_CUSTOM_IDENT, &outputdata_is_updated, outputdata, &outputdata_length, &outputdata_iops);
                     break;
                  }
               }

               /* Set LED state */
               if (outputdata_is_updated == true && outputdata_iops == PNET_IOXS_GOOD)
               {
                  if (outputdata_length == APP_DATASIZE_OUTPUT)
                  {
                     received_led_state = (outputdata[0] & 0x80) > 0;  /* Use most significant bit */
                     if (received_led_state != led_state)
                     {
                        led_state = received_led_state;
                        if (p_appdata->arguments.verbosity > 0)
                        {
                           printf("Changing LED state: %d\n", led_state);
                        }
                        if (write_bool_to_file(p_appdata->arguments.path_led, led_state) != 0)
                        {
                           printf("Failed to write to LED file: %s\n", p_appdata->arguments.path_led);
                        };
                     }
                  }
                  else
                  {
                     printf("Wrong outputdata length: %u\n", outputdata_length);
                  }

               }
            }
         }

         /* Create an alarm on first input slot (if any) when button 2 is pressed/released */
         if (p_appdata->main_arep != UINT32_MAX)
         {
            if ((button2_pressed == true) && (button2_pressed_previous == false) && (p_appdata->alarm_allowed == true))
            {
               alarm_payload[0]++;
               for (slot = 0; slot < PNET_MAX_MODULES; slot++)
               {
                  if (p_appdata->custom_input_slots[slot] == true)
                  {
                     printf("Sending process alarm from slot %u subslot %u to IO-controller. Payload: 0x%x\n",
                        slot,
                        PNET_SUBMOD_CUSTOM_IDENT,
                        alarm_payload[0]);
                     pnet_alarm_send_process_alarm(
                        net,
                        p_appdata->main_arep,
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
         }
         button2_pressed_previous = button2_pressed;

         pnet_handle_periodic(net);
      }
      else if (flags & EVENT_ABORT)
      {
         /* Reset main */
         p_appdata->main_arep = UINT32_MAX;
         p_appdata->alarm_allowed = true;
         button1_pressed = false;
         button2_pressed = false;
         os_event_clr(p_appdata->main_events, EVENT_ABORT); /* Re-arm */
         if (p_appdata->arguments.verbosity > 0)
         {
            printf("Aborting the application\n");
         }
      }
   }
   os_timer_destroy(p_appdata->main_timer);
   os_event_destroy(p_appdata->main_events);
   printf("Ending the application\n");
}

/****************************** Main ******************************************/

int main(int argc, char *argv[])
{
   pnet_t *net;
   app_data_and_stack_t appdata_and_stack;

   app_data_t appdata;
   appdata.app_param_1 = 0;
   appdata.app_param_2 = 0;
   appdata.alarm_allowed = true;
   appdata.main_arep = UINT32_MAX;
   appdata.main_events = NULL;
   appdata.main_timer = NULL;
   memset(appdata.inputdata, 0, sizeof(appdata.inputdata));
   memset(appdata.custom_input_slots, 0, sizeof(appdata.custom_input_slots));
   memset(appdata.custom_output_slots, 0, sizeof(appdata.custom_output_slots));

   /* Parse and display command line arguments */
   appdata.arguments = parse_commandline_arguments(argc, argv);

   printf("\n** Starting Profinet demo application **\n");
   if (appdata.arguments.verbosity > 0)
   {
      printf("Number of slots:     %u (incl slot for DAP module)\n", PNET_MAX_MODULES);
      printf("P-net log level:     %u (DEBUG=0, ERROR=3)\n", LOG_LEVEL);
      printf("App verbosity level: %u\n", appdata.arguments.verbosity);
      printf("Ethernet interface:  %s\n", appdata.arguments.eth_interface);
      printf("Station name:        %s\n", appdata.arguments.station_name);
      printf("LED file:            %s\n", appdata.arguments.path_led);
      printf("Button1 file:        %s\n", appdata.arguments.path_button1);
      printf("Button2 file:        %s\n", appdata.arguments.path_button2);
   }

   /* Read IP, netmask, gateway and MAC address from operating system */
   if (!does_network_interface_exist(appdata.arguments.eth_interface))
   {
      printf("Error: The given Ethernet interface does not exist: %s\n", appdata.arguments.eth_interface);
      exit(EXIT_CODE_ERROR);
   }

   uint32_t ip_int = read_ip_address(appdata.arguments.eth_interface);
   if (ip_int == IP_INVALID)
   {
      printf("Error: Invalid IP address.\n");
      exit(EXIT_CODE_ERROR);
   }

   uint32_t netmask_int = read_netmask(appdata.arguments.eth_interface);

   uint32_t gateway_ip_int = read_default_gateway(appdata.arguments.eth_interface);
   if (gateway_ip_int == IP_INVALID)
   {
      printf("Error: Invalid gateway IP address.\n");
      exit(EXIT_CODE_ERROR);
   }

   pnet_ethaddr_t macbuffer;
   int ret = read_mac_address(appdata.arguments.eth_interface, &macbuffer);
   if (ret != 0)
   {
      printf("Error: Can not read MAC address for Ethernet interface %s\n", appdata.arguments.eth_interface);
      exit(EXIT_CODE_ERROR);
   }

   if (appdata.arguments.verbosity > 0)
   {
      printf("MAC address:        %02x:%02x:%02x:%02x:%02x:%02x\n",
         macbuffer.addr[0],
         macbuffer.addr[1],
         macbuffer.addr[2],
         macbuffer.addr[3],
         macbuffer.addr[4],
         macbuffer.addr[5]);
      printf("IP address:         ");
      print_ip_address(ip_int);
      printf("\nNetmask:            ");
      print_ip_address(netmask_int);
      printf("\nGateway:            ");
      print_ip_address(gateway_ip_int);
      printf("\n\n");
   }

   /* Set IP, gateway and station name */
   strcpy(pnet_default_cfg.im_0_data.order_id, "12345");
   strcpy(pnet_default_cfg.im_0_data.im_serial_number, "00001");
   copy_ip_to_struct(&pnet_default_cfg.ip_addr, ip_int);
   copy_ip_to_struct(&pnet_default_cfg.ip_gateway, gateway_ip_int);
   copy_ip_to_struct(&pnet_default_cfg.ip_mask, netmask_int);
   strcpy(pnet_default_cfg.station_name, appdata.arguments.station_name);
   memcpy(pnet_default_cfg.eth_addr.addr, macbuffer.addr, sizeof(pnet_ethaddr_t));
   pnet_default_cfg.cb_arg = (void*) &appdata;

   /* Paths for LED and button control files */
   if (appdata.arguments.path_led[0] != '\0')
   {
      if (!does_file_exist(appdata.arguments.path_led))
      {
         printf("Error: The given LED output file does not exist: %s\n",
            appdata.arguments.path_led);
         exit(EXIT_CODE_ERROR);
      }

      /* Turn off LED initially */
      if (write_bool_to_file(appdata.arguments.path_led, false) != 0)
      {
         printf("Error: Could not write to LED output file: %s\n",
            appdata.arguments.path_led);
         exit(EXIT_CODE_ERROR);
      }
   }

   if (appdata.arguments.path_button1[0] != '\0')
   {
      if (!does_file_exist(appdata.arguments.path_button1))
      {
         printf("Error: The given input file for button1 does not exist: %s\n",
            appdata.arguments.path_button1);
         exit(EXIT_CODE_ERROR);
      }
   }

   if (appdata.arguments.path_button2[0] != '\0')
   {
      if (!does_file_exist(appdata.arguments.path_button2))
      {
         printf("Error: The given input file for button2 does not exist: %s\n",
            appdata.arguments.path_button2);
         exit(EXIT_CODE_ERROR);
      }
   }

   /* Initialize profinet stack */
   net = pnet_init(appdata.arguments.eth_interface, TICK_INTERVAL_US, &pnet_default_cfg);
   if (net == NULL)
   {
      printf("Failed to initialize p-net. Do you have enough Ethernet interface permission?\n");
      exit(EXIT_CODE_ERROR);
   }
   appdata_and_stack.appdata = &appdata;
   appdata_and_stack.net = net;

   /* Initialize timer and Profinet stack */
   appdata.main_events = os_event_create();
   appdata.main_timer  = os_timer_create(TICK_INTERVAL_US, main_timer_tick, (void*)&appdata, false);

   os_thread_create("pn_main", APP_PRIORITY, APP_STACKSIZE, pn_main, (void*)&appdata_and_stack);
   os_timer_start(appdata.main_timer);

   for(;;)
      os_usleep(APP_MAIN_SLEEPTIME_US);

   return 0;
}
