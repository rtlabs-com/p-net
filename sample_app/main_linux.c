#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <sys/stat.h>

#include <pnet_api.h>
#include "osal.h"
#include "log.h"

#define IP_INVALID              0

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

#define EVENT_READY_FOR_DATA     BIT(0)
#define EVENT_TIMER              BIT(1)
#define EVENT_ALARM              BIT(2)
#define EVENT_ABORT              BIT(15)

#define EXIT_CODE_ERROR          1

#define TICK_INTERVAL_US         1000 /* 1 ms */

/* From the GSDML file */

/* The following constant must match the GSDML file. */
#define APP_PARAM_IDX_1          123
#define APP_PARAM_IDX_2          124

#define APP_API                  0

/*
 * Module ident number for the DAP module.
 * The DAP module must be plugged by the application after the call to pnet_init.
 */
#define PNET_MOD_DAP_IDENT       0x00000001     /* Slot 0 */

/*
 * Sub-module ident numbers for the DAP sub-slots.
 * The DAP submodules must be plugged by the application after the call to pnet_init.
 */
#define PNET_SUBMOD_DAP_IDENT           0x00000001     /* Subslot 1 */
#define PNET_SUBMOD_DAP_INTERFACE_IDENT 0x00008000     /* Subslot 0x8000 */
#define PNET_SUBMOD_DAP_PORT_0_IDENT    0x00008001     /* Subslot 0x8001 */

/*
 * I/O Modules. These modules and their sub-modules must be plugged by the application after the call to pnet_init.
 * The sub-module ident numbers are all 0x00000001 for these modules.
 */
#define PNET_MOD_8_0_IDENT       0x00000030     /* 8 bit input */
#define PNET_MOD_0_8_IDENT       0x00000031     /* 8 bit output */
#define PNET_MOD_8_8_IDENT       0x00000032     /* 8 bit input, 8 bit output */

#define PNET_SUBMOD_IDENT        0x00000001

/**
 * This is just a simple example on how the application can maintain its list of supported APIs, modules and submodules.
 */
static uint32_t            cfg_module_ident_numbers[] =
{
   PNET_MOD_DAP_IDENT,
   PNET_MOD_8_0_IDENT,
   PNET_MOD_0_8_IDENT,
   PNET_MOD_8_8_IDENT
};

static const struct
{
   uint32_t                api;
   uint16_t                slot_nbr;
   uint16_t                subslot_nbr;
   uint32_t                module_ident_nbr;
   uint32_t                submodule_ident_nbr;
   pnet_submodule_dir_t    data_dir;
   uint16_t                insize;
   uint16_t                outsize;
} submodule_cfg[] =
{
   {APP_API, 0, 1, PNET_MOD_DAP_IDENT, PNET_SUBMOD_DAP_IDENT, PNET_DIR_NO_IO, 0, 0},
   {APP_API, 0, 32768, PNET_MOD_DAP_IDENT, PNET_SUBMOD_DAP_INTERFACE_IDENT, PNET_DIR_NO_IO, 0, 0},
   {APP_API, 0, 32769, PNET_MOD_DAP_IDENT, PNET_SUBMOD_DAP_PORT_0_IDENT, PNET_DIR_NO_IO, 0, 0},
   {APP_API, 1, 1, PNET_MOD_8_8_IDENT, PNET_SUBMOD_IDENT, PNET_DIR_IO, 1, 1},
};

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
      .station_name = "rt-labs-dev",
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
      .ip_addr = { 0, 0, 0, 0 },
      .ip_mask = { 255, 255, 255, 0 },
      .ip_gateway = { 0, 0, 0, 0 },
};


/********************************** Globals ***********************************/

static os_timer_t                *main_timer = NULL;
static os_event_t                *main_events = NULL;
static uint32_t                  main_arep = UINT32_MAX;
static bool                      alarm_allowed = true;
static uint32_t                  app_param_1 = 0x0;
static uint32_t                  app_param_2 = 0x0;
static int                       verbosity = 0;
struct cmd_args                  arguments;
uint8_t                          inputdata[] =
{
   0x00,       /* Slot 1, subslot 1 input data (digital inputs) */
};


/*********************************** Callbacks ********************************/

static int app_connect_ind(
   uint32_t                arep,
   pnet_result_t           *p_result)
{
   if (verbosity > 0)
   {
      printf("Connect call-back. Status codes: %d %d %d %d\n",
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
   uint32_t                arep,
   pnet_result_t           *p_result)
{
   if (verbosity > 0)
   {
      printf("Release (disconnect) call-back. Status codes: %d %d %d %d\n",
         p_result->pnio_status.error_code,
         p_result->pnio_status.error_decode,
         p_result->pnio_status.error_code_1,
         p_result->pnio_status.error_code_2);
   }

   return 0;
}

static int app_dcontrol_ind(
   uint32_t                arep,
   pnet_control_command_t  control_command,
   pnet_result_t           *p_result)
{
   if (verbosity > 0)
   {
      printf("Dcontrol call-back. Command: %d  Status codes: %d %d %d %d\n",
         control_command,
         p_result->pnio_status.error_code,
         p_result->pnio_status.error_decode,
         p_result->pnio_status.error_code_1,
         p_result->pnio_status.error_code_2);
   }

   return 0;
}

static int app_ccontrol_cnf(
   uint32_t                arep,
   pnet_result_t           *p_result)
{
   if (verbosity > 0)
   {
      printf("Ccontrol confirmation call-back. Status codes: %d %d %d %d\n",
         p_result->pnio_status.error_code,
         p_result->pnio_status.error_decode,
         p_result->pnio_status.error_code_1,
         p_result->pnio_status.error_code_2);
   }

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
   if (verbosity > 0)
   {
      printf("Write call-back. API: %u Slot: %u Subslot: %u Index: %u Sequence: %u Length: %u\n",
         api,
         slot,
         subslot,
         (unsigned)idx,
         sequence_number,
         write_length);
   }
   if (idx == APP_PARAM_IDX_1)
   {
      if (write_length == sizeof(app_param_1))
      {
         memcpy(&app_param_1, p_write_data, sizeof(app_param_1));
      }
      else
      {
         printf("Wrong length in write call-back. Index: %u Length: %u Expected length: %u\n",
            (unsigned)idx,
            (unsigned)write_length,
            (unsigned)sizeof(app_param_1));
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
         printf("Wrong length in write call-back. Index: %u Length: %u Expected length: %u\n",
            (unsigned)idx,
            (unsigned)write_length,
            (unsigned)sizeof(app_param_2));
      }
   }
   else
   {
      printf("Wrong index in write call-back: %u\n", (unsigned)idx);
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
   if (verbosity > 0)
   {
      printf("Read call-back. API: %u Slot: %u Subslot: %u Index: %u Sequence: %u Length: %u\n",
         api,
         slot,
         subslot,
         (unsigned)idx,
         sequence_number,
         (unsigned)*p_read_length);
   }
   if ((idx == APP_PARAM_IDX_1) &&
       (*p_read_length == sizeof(app_param_1)))
   {
      *pp_read_data = (uint8_t*)&app_param_1;
      *p_read_length = sizeof(app_param_1);
   }
   if ((idx == APP_PARAM_IDX_2) &&
       (*p_read_length == sizeof(app_param_2)))
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

   if (state == PNET_EVENT_ABORT)
   {
      if (pnet_get_ar_error_codes(arep, &err_cls, &err_code) == 0)
      {
         if (verbosity > 0)
         {
               printf("Callback on event PNET_EVENT_ABORT. Error class: %u Error code: %u\n",
                  (unsigned)err_cls, (unsigned)err_code);
         }
      }
      else
      {
         if (verbosity > 0)
         {
               printf("Callback on event PNET_EVENT_ABORT. No error status available\n");
         }
      }
      /* Only abort AR with correct session key */
      os_event_set(main_events, EVENT_ABORT);
   }
   else if (state == PNET_EVENT_PRMEND)
   {
      if (verbosity > 0)
      {
         printf("Callback on event PNET_EVENT_PRMEND\n");
      }

      /* Save the arep for later use */
      main_arep = arep;
      os_event_set(main_events, EVENT_READY_FOR_DATA);
      (void)pnet_input_set_data_and_iops(APP_API, 0, 1, NULL, 0, PNET_IOXS_GOOD);
      (void)pnet_input_set_data_and_iops(APP_API, 0, 0x8000, NULL, 0, PNET_IOXS_GOOD);
      (void)pnet_input_set_data_and_iops(APP_API, 0, 0x8001, NULL, 0, PNET_IOXS_GOOD);
      (void)pnet_input_set_data_and_iops(APP_API, 1, 1, inputdata, sizeof(inputdata), PNET_IOXS_GOOD);
      (void)pnet_output_set_iocs(APP_API, 0, 1, PNET_IOXS_GOOD);
      (void)pnet_output_set_iocs(APP_API, 0, 0x8000, PNET_IOXS_GOOD);
      (void)pnet_output_set_iocs(APP_API, 0, 0x8001, PNET_IOXS_GOOD);
      (void)pnet_output_set_iocs(APP_API, 1, 1, PNET_IOXS_GOOD);
      (void)pnet_set_provider_state(true);
   }
   else if (state == PNET_EVENT_DATA)
   {
      if (verbosity > 0)
      {
         printf("Callback on event PNET_EVENT_DATA\n");
      }
   }
   else if (state == PNET_EVENT_STARTUP)
   {
      if (verbosity > 0)
      {
         printf("Callback on event PNET_EVENT_STARTUP\n");
      }
   }
   else if (state == PNET_EVENT_APPLRDY)
   {
      if (verbosity > 0)
      {
         printf("Callback on event PNET_EVENT_APPLRDY\n");
      }
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

   if (verbosity > 0)
   {
      printf("Module plug call-back\n");
   }

   /* Find it in the list of supported modules */
   ix = 0;
   while ((ix < NELEMENTS(cfg_module_ident_numbers)) &&
          (cfg_module_ident_numbers[ix] != module_ident))
   {
      ix++;
   }

   if (ix < NELEMENTS(cfg_module_ident_numbers))
   {
      // TODO Here we could do a pnet_pull_module(api, slot) to make sure it is empty,
      // but it prints an ugly error message if the slot is empty.

      /* For now support any module in any slot */
      if (verbosity > 0)
      {
         printf("  Plug module.    API: %u Slot: %u Module ID: 0x%x Index in supported modules: %u\n", api, slot, (unsigned)module_ident, ix);
      }
      ret = pnet_plug_module(api, slot, module_ident);
      if (ret != 0)
      {
         printf("Plug module failed. Ret: %u API: %u Slot: %u Module ID: 0x%x Index in list of supported modules: %u\n", ret, api, slot, (unsigned)module_ident, ix);
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
   uint16_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint32_t                module_ident,
   uint32_t                submodule_ident)
{
   int                     ret = -1;
   uint16_t                ix = 0;

   if (verbosity > 0)
   {
      printf("Submodule plug call-back.\n");
   }

   /* Find it in the list of supported submodules */
   ix = 0;
   while ((ix < NELEMENTS(submodule_cfg)) &&
          ((submodule_cfg[ix].module_ident_nbr != module_ident) ||
           (submodule_cfg[ix].submodule_ident_nbr != submodule_ident)))
   {
      ix++;
   }

   if (ix < NELEMENTS(submodule_cfg))
   {
      // TODO Here we could do a pnet_pull_submodule(api, slot, subslot) to make sure it is empty,
      // but it prints an ugly error message if the subslot is empty.

      if (verbosity > 0)
      {
         printf("  Plug submodule. API: %u Slot: %u Module ID: 0x%x Subslot: 0x%x Submodule ID: 0x%x Index in supported submodules: %u\n",
            api,
            slot,
            (unsigned)module_ident,
            subslot,
            (unsigned)submodule_ident,
            ix);
      }
      ret = pnet_plug_submodule(api, slot, subslot, module_ident, submodule_ident,
         submodule_cfg[ix].data_dir,
         submodule_cfg[ix].insize,
         submodule_cfg[ix].outsize);
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
   uint32_t                arep,
   uint32_t                crep,
   uint8_t                 changes)
{
   if (verbosity > 0)
   {
      printf("New data callback. Status: 0x%02x\n", changes);
   }

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
   if (verbosity > 0)
   {
      printf("Alarm indicated callback. API: %d Slot: %d Subslot: %d Length: %d",
         api,
         slot,
         subslot,
         data_len);
   }
   os_event_set(main_events, EVENT_ALARM);

   return 0;
}

static int app_alarm_cnf(
   uint32_t                arep,
   pnet_pnio_status_t      *p_pnio_status)
{
   if (verbosity > 0)
   {
      printf("Alarm confirmed (by controller) callback. Status code %u, %u, %u, %u\n",
         p_pnio_status->error_code,
         p_pnio_status->error_decode,
         p_pnio_status->error_code_1,
         p_pnio_status->error_code_2);
   }
   alarm_allowed = true;

   return 0;
}

static int app_alarm_ack_cnf(
   uint32_t                arep,
   int                     res)
{
   if (verbosity > 0)
   {
      printf("Alarm ACK confirmation (from controller) callback. Result: %d\n", res);
   }

   return 0;
}

/************************* Utilities ******************************************/

static void main_timer_tick(
   os_timer_t              *timer,
   void                    *arg)
{
   os_event_set(main_events, EVENT_TIMER);
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
   printf("Optional arguments:\n");
   printf("   --help       Show this help text and exit\n");
   printf("   -h           Show this help text and exit\n");
   printf("   -v           Incresase verbosity\n");
   printf("   -i INTERF    Set Ethernet interface name. Defaults to eth0\n");
   printf("   -p IP        Device (this machine) IP address. Defaults to 192.168.1.1\n");
   printf("   -g IP        Gateway IP address. Defaults to .1 in same subnet as -p\n");
   printf("   -l file      Path to control LED. Defaults to not control any LED.\n");
   printf("   -b file      Path to read button1. Defaults to not read button1.\n");
    printf("   -d file      Path to read button2. Defaults to not read button2.\n");
}

struct cmd_args {
   char path_led[256];
   char path_button1[256];
   char path_button2[256];
   char eth_interface[64];
   char ip_address_str[64];
   char gateway_ip_address_str[64];
   int verbosity;
};

/**
 * Parse command line arguments
 *
 * @param argc      In: Number of arguments
 * @param argv      In: Arguments
 * @return Parsed arguments
*/
struct cmd_args parse_commandline_arguments(int argc, char *argv[])
{
   // Special handling of long argument
   if (argc > 1)
   {
      if (strcmp(argv[1], "--help") == 0)
      {
         show_usage();
         exit(EXIT_CODE_ERROR);
      }
   }

   // Default values
   struct cmd_args output_arguments;
   strcpy(output_arguments.path_led, "");
   strcpy(output_arguments.path_button1, "");
   strcpy(output_arguments.path_button2, "");
   strcpy(output_arguments.eth_interface, "eth0");
   strcpy(output_arguments.ip_address_str, "192.168.1.1");
   strcpy(output_arguments.gateway_ip_address_str, "");
   output_arguments.verbosity = 0;

   int option;
   while ((option = getopt(argc, argv, "hvi:p:g:l:b:d:")) != -1) {
      switch (option) {
      case 'v':
         output_arguments.verbosity++;
         break;
      case 'i':
         strcpy(output_arguments.eth_interface, optarg);
         break;
      case 'p':
         strcpy(output_arguments.ip_address_str, optarg);
         break;
      case 'g':
         strcpy(output_arguments.gateway_ip_address_str, optarg);
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
 * Convert IPv4 address string to an integer
 *
 * @param ip_str      In: IPv4 address
 * @return IP address on success
 *         0 on failure
*/
uint32_t ip_str_to_int(const char* ip_str)
{
   struct in_addr addr;
   int res = inet_aton(ip_str, &addr);
   if (res != 1)
   {
      return IP_INVALID;
   }

   return addr.s_addr;
}

/**
 * Convert gateway IPv4 address string to an integer
 *
 * If no gateway IP address is given, use the IP main address but replace
 * last digit with .1
 *
 * @param gateway_ip_str      In: IPv4 gateway address
 * @param ip_int              In: Already parsed IP address
 * @return Gateway IP address on success
 *         0 on failure
*/
uint32_t calculate_gateway_ip(const char* gateway_ip_str, uint32_t ip_int)
{
   if (gateway_ip_str[0] != '\0')
   {
      return ip_str_to_int(gateway_ip_str);
   }

   return (ip_int & 0x00FFFFFF) | 0x01000000;
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

   fp = fopen(filepath, "r");
   if (fp == NULL)
   {
      return false;
   }

   ch = fgetc(fp);
   if (feof(fp))
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

void pn_main (void * arg)
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
   uint8_t        outputdata[64];
   uint8_t        outputdata_iops;
   uint16_t       outputdata_length;
   bool           outputdata_is_updated = false;
   uint8_t        alarm_payload[] =
   {
      0x00,
   };

   if (verbosity > 0)
   {
      printf("Waiting for connect request from IO-controller\n");
   }

   /* Main loop */
   for (;;)
   {
      os_event_wait(main_events, mask, &flags, OS_WAIT_FOREVER);
      if (flags & EVENT_READY_FOR_DATA)
      {
         os_event_clr(main_events, EVENT_READY_FOR_DATA); /* Re-arm */

         /* Send appl ready to profinet stack. */
         ret = pnet_application_ready(main_arep);
         if (verbosity > 0)
         {
            printf("Application signalled that it is ready for data. Return value: %d\n", ret);
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
            printf("Error when sending alarm ACK. Error: %d\n", ret);
         }
         else if (verbosity > 0)
         {
            printf("Alarm ACK sent\n");
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
            if (arguments.path_button1[0] != '\0')
            {
               button1_pressed = read_bool_from_file(arguments.path_button1);
            }
            if (arguments.path_button2[0] != '\0')
            {
               button2_pressed = read_bool_from_file(arguments.path_button2);
            }
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
            pnet_input_set_data_and_iops(APP_API, 1, 1, inputdata, sizeof(inputdata), PNET_IOXS_GOOD);

            /* Output data, for LED output */
            if (arguments.path_led[0] != '\0')
            {
               outputdata_length = sizeof(outputdata);
               pnet_output_get_data_and_iops(APP_API, 1, 1, &outputdata_is_updated, outputdata, &outputdata_length, &outputdata_iops);
               if (outputdata_is_updated == true)  //TODO check IOPS
               {
                  received_led_state = (outputdata[0] & 0x80) > 0;
                  if (received_led_state != led_state)
                  {
                     led_state = received_led_state;
                     if (verbosity > 0)
                     {
                        printf("Changing LED state: %d\n", led_state);
                     }
                     write_bool_to_file(arguments.path_led, led_state);
                  }
               }
            }
         }

         /* Create an alarm when button 2 is pressed/released */
         if (main_arep != UINT32_MAX)
         {
            if ((button2_pressed == true) && (button2_pressed_previous == false) && (alarm_allowed == true))
            {
               alarm_payload[0]++;
               printf("Sending process alarm to IO-controller. Payload: 0x%x\n", alarm_payload[0]);
               pnet_alarm_send_process_alarm(main_arep, APP_API, 1, 1, 1, sizeof(alarm_payload), alarm_payload);
            }
         }
         button2_pressed_previous = button2_pressed;

         pnet_handle_periodic();
      }
      else if (flags & EVENT_ABORT)
      {
         /* Reset main */
         main_arep = UINT32_MAX;
         alarm_allowed = true;
         os_event_clr(main_events, EVENT_ABORT); /* Re-arm */
         if (verbosity > 0)
         {
            printf("Aborting the application\n");
         }
      }
   }
   os_timer_destroy(main_timer);
   os_event_destroy(main_events);
   printf("Ending the application\n");
}

/****************************** Main ******************************************/

int main(int argc, char *argv[])
{
   /* Parse and display command line arguments */
   arguments = parse_commandline_arguments(argc, argv);
   verbosity = arguments.verbosity;  // Global variable for use in callbacks
   printf("\n** Starting Profinet demo application **\n");
   if (verbosity > 0)
   {
      printf("Verbosity level: %u\n", verbosity);
      printf("Ethernet interface: %s\n", arguments.eth_interface);
      printf("IP address: %s\n", arguments.ip_address_str);
      printf("Gateway: %s\n", arguments.gateway_ip_address_str);
      printf("LED file: %s\n", arguments.path_led);
      printf("Button1 file: %s\n", arguments.path_button1);
      printf("Button2 file: %s\n\n", arguments.path_button2);
   }

   /* Set IP and gateway, and check network interface */
   uint32_t ip_int = ip_str_to_int(arguments.ip_address_str);
   if (ip_int == IP_INVALID)
   {
      printf("Error: Invalid IP address: %s\n", arguments.ip_address_str);
      exit(EXIT_CODE_ERROR);
   }

   uint32_t gateway_ip_int = calculate_gateway_ip(arguments.gateway_ip_address_str, ip_int);
   if (gateway_ip_int == IP_INVALID)
   {
      printf("Error: Invalid gateway IP address: %s\n", arguments.gateway_ip_address_str);
      exit(EXIT_CODE_ERROR);
   }

   strcpy(pnet_default_cfg.im_0_data.order_id, "12345");
   strcpy(pnet_default_cfg.im_0_data.im_serial_number, "00001");
   pnet_default_cfg.ip_addr.a = (ip_int&0xFF);
   pnet_default_cfg.ip_addr.b = ((ip_int>>8)&0xFF);
   pnet_default_cfg.ip_addr.c = ((ip_int>>16)&0xFF);
   pnet_default_cfg.ip_addr.d = ((ip_int>>24)&0xFF);
   pnet_default_cfg.ip_gateway.a = (gateway_ip_int&0xFF);
   pnet_default_cfg.ip_gateway.b = ((gateway_ip_int>>8)&0xFF);
   pnet_default_cfg.ip_gateway.c = ((gateway_ip_int>>16)&0xFF);
   pnet_default_cfg.ip_gateway.d = ((gateway_ip_int>>24)&0xFF);

   if (!does_network_interface_exist(arguments.eth_interface))
   {
      printf("Error: The given Ethernet interface does not exist: %s\n", arguments.eth_interface);
      exit(EXIT_CODE_ERROR);
   }

   /* Paths for LED and button control files */
   if (arguments.path_led[0] != '\0')
   {
      if (!does_file_exist(arguments.path_led))
      {
         printf("Error: The given LED output file does not exist: %s\n",
            arguments.path_led);
         exit(EXIT_CODE_ERROR);
      }

      // Turn off LED initially
      if (write_bool_to_file(arguments.path_led, false) != 0)
      {
         printf("Error: Could not write to LED output file: %s\n",
            arguments.path_led);
         exit(EXIT_CODE_ERROR);
      }
   }

   if (arguments.path_button1[0] != '\0')
   {
      if (!does_file_exist(arguments.path_button1))
      {
         printf("Error: The given input file for button1 does not exist: %s\n",
            arguments.path_button1);
         exit(EXIT_CODE_ERROR);
      }
   }

   if (arguments.path_button2[0] != '\0')
   {
      if (!does_file_exist(arguments.path_button2))
      {
         printf("Error: The given input file for button2 does not exist: %s\n",
            arguments.path_button2);
         exit(EXIT_CODE_ERROR);
      }
   }

   if (pnet_init(arguments.eth_interface, TICK_INTERVAL_US, &pnet_default_cfg) != 0)
   {
      printf("Failed to initialize p-net. Do you have enough Ethernet interface permission?\n");
      exit(EXIT_CODE_ERROR);
   }

   /* Initialize timer and Profinet stack */
   main_events = os_event_create();
   main_timer  = os_timer_create(TICK_INTERVAL_US, main_timer_tick, NULL, false);

   os_thread_create ("pn_main", 15, 4096, pn_main, NULL);
   os_timer_start(main_timer);

   for(;;)
      os_usleep(5000*1000);

   return 0;
}
