
#include <string.h>
#include <pnet_api.h>

#include "main_osal.h"
#include "main_log.h"

/* The following constant must match the GSDML file. */
#define APP_PARAM_IDX_1          123
#define APP_PARAM_IDX_2          124


#define EVENT_READY_FOR_DATA     BIT(0)
#define EVENT_TIMER              BIT(1)
#define EVENT_ALARM              BIT(2)
#define EVENT_ABORT              BIT(15)

#define TICK_INTERVAL_US         1000 /* 1 ms */

/* Declare call-back functions */
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

/* From the GSDML file */

/*
 * Module ident number for the DAP module.
 * The DAP module must be plugged by the application after the call to pnet_init.
 */
#define PNET_DAP_IDENT           0x00000001     /* Slot 0 */

/*
 * Sub-module ident numbers for the DAP sub-slots.
 * The DAP submodules must be plugged by the application after the call to pnet_init.
 */
#define PNET_DAP_DAP_IDENT       0x00000001     /* Subslot 1 */
#define PNET_DAP_INTERFACE_IDENT 0x00008000     /* Subslot 0x8000 */
#define PNET_DAP_PORT_0_IDENT    0x00008001     /* Subslot 0x8001 */

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
    PNET_DAP_IDENT,
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
 {0, 0, 1, PNET_DAP_IDENT, PNET_DAP_DAP_IDENT, PNET_DIR_NO_IO, 0, 0},
 {0, 0, 32768, PNET_DAP_IDENT, PNET_DAP_INTERFACE_IDENT, PNET_DIR_NO_IO, 0, 0},
 {0, 0, 32769, PNET_DAP_IDENT, PNET_DAP_PORT_0_IDENT, PNET_DIR_NO_IO, 0, 0},
 {0, 1, 1, PNET_MOD_8_8_IDENT, PNET_SUBMOD_IDENT, PNET_DIR_IO, 1, 1},
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
         0xfe, 0xed, 0xbe, 0xef
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
      .ip_addr = { 192, 168, 1, 171 },
      .ip_mask = { 255, 255, 255, 0 },
      .ip_gateway = { 192, 168, 1, 1 },
};

static os_timer_t                *main_timer = NULL;
static os_event_t                *main_events = NULL;

static uint8_t                   data_ctr = 0;

static uint32_t                  main_arep = UINT32_MAX;
static bool                      alarm_allowed = true;

static uint32_t                  app_param_1 = 0x0;
static uint32_t                  app_param_2 = 0x0;

static bool                      init_done = false;

static int app_connect_ind(
   uint32_t                arep,
   pnet_result_t           *p_result)
{
   MAIN_LOG_INFO(PNET_MAIN_LOG, "APP: connect call-back %d %d %d %d\n",
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
   MAIN_LOG_INFO(PNET_MAIN_LOG, "APP: release call-back %d %d %d %d\n",
         p_result->pnio_status.error_code,p_result->pnio_status.error_decode,
         p_result->pnio_status.error_code_1,p_result->pnio_status.error_code_2);

   return 0;
}

static int app_dcontrol_ind(
   uint32_t                arep,
   pnet_control_command_t  control_command,
   pnet_result_t           *p_result)
{
   MAIN_LOG_INFO(PNET_MAIN_LOG, "APP: dcontrol call-back (cmd:%d) %d %d %d %d\n",
         control_command,
         p_result->pnio_status.error_code,p_result->pnio_status.error_decode,
         p_result->pnio_status.error_code_1,p_result->pnio_status.error_code_2);

   return 0;
}

static int app_ccontrol_cnf(
   uint32_t                arep,
   pnet_result_t           *p_result)
{
   MAIN_LOG_INFO(PNET_MAIN_LOG, "APP: ccontrol confirmation call-back %d %d %d %d\n",
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
   MAIN_LOG_INFO(PNET_MAIN_LOG, "APP: write call-back: idx %u\n", (unsigned)idx);
   if (idx == APP_PARAM_IDX_1)
   {
      if (write_length == sizeof(app_param_1))
      {
         memcpy(&app_param_1, p_write_data, sizeof(app_param_1));
      }
      else
      {
          MAIN_LOG_INFO(PNET_MAIN_LOG, "APP: write call-back: idx %u  length %u - expected length %u\n",
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
          MAIN_LOG_INFO(PNET_MAIN_LOG, "APP: write call-back: idx %u  length %u - expected length %u\n",
                (unsigned)idx, (unsigned)write_length, (unsigned)sizeof(app_param_2));
      }
   }
   else
   {
      MAIN_LOG_INFO(PNET_MAIN_LOG, "APP: write call-back: unknown idx %u\n", (unsigned)idx);
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
   MAIN_LOG_INFO(PNET_MAIN_LOG, "APP: read call-back: idx %u  length %u\n",
         (unsigned)idx, (unsigned)*p_read_length);

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
   uint8_t                 data[1] = {0};
   uint16_t                err_cls = 0;
   uint16_t                err_code = 0;

   MAIN_LOG_DEBUG(PNET_MAIN_LOG, "APP: state call-back %d\n", state);

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
      /* Save the arep for later use */
      main_arep = arep;
      os_event_set(main_events, EVENT_READY_FOR_DATA);
      (void)pnet_input_set_data_and_iops(0, 0, 1, NULL, 0, PNET_IOXS_GOOD);
      (void)pnet_input_set_data_and_iops(0, 0, 0x8000, NULL, 0, PNET_IOXS_GOOD);
      (void)pnet_input_set_data_and_iops(0, 0, 0x8001, NULL, 0, PNET_IOXS_GOOD);
      (void)pnet_input_set_data_and_iops(0, 1, 1, data, sizeof(data), PNET_IOXS_GOOD);
      (void)pnet_output_set_iocs(0, 0, 1, PNET_IOXS_GOOD);
      (void)pnet_output_set_iocs(0, 0, 0x8000, PNET_IOXS_GOOD);
      (void)pnet_output_set_iocs(0, 0, 0x8001, PNET_IOXS_GOOD);
      (void)pnet_output_set_iocs(0, 1, 1, PNET_IOXS_GOOD);
      (void)pnet_set_provider_state(true);
   }
   else if (state == PNET_EVENT_DATA)
   {
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
   while ((ix < NELEMENTS(cfg_module_ident_numbers)) &&
          (cfg_module_ident_numbers[ix] != module_ident))
   {
      ix++;
   }

   if (ix < NELEMENTS(cfg_module_ident_numbers))
   {
      if (pnet_pull_module(api, slot) != 0)
      {
         MAIN_LOG_DEBUG(PNET_MAIN_LOG, "APP: Pull module failed\n");
      }
      /* For now support any module in any slot */
      ret = pnet_plug_module(api, slot, module_ident);
   }

   MAIN_LOG_DEBUG(PNET_MAIN_LOG, "APP: ret %d expected module call-back 0x%08x\n", ret, (unsigned)module_ident);

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
   while ((ix < NELEMENTS(submodule_cfg)) &&
          ((submodule_cfg[ix].module_ident_nbr != module_ident) ||
           (submodule_cfg[ix].submodule_ident_nbr != submodule_ident)))
   {
      ix++;
   }
   if (ix < NELEMENTS(submodule_cfg))
   {
      if (pnet_pull_submodule(api, slot, subslot) != 0)
      {
         MAIN_LOG_DEBUG(PNET_MAIN_LOG, "APP: Pull sub-module failed\n");
      }
      ret = pnet_plug_submodule(api, slot, subslot, module_ident, submodule_ident,
         submodule_cfg[ix].data_dir,
         submodule_cfg[ix].insize, submodule_cfg[ix].outsize);
   }

   MAIN_LOG_DEBUG(PNET_MAIN_LOG, "APP: ret %d expected submodule call-back module ident 0x%08x, submodule ident 0x%08x\n", ret, (unsigned)module_ident, (unsigned)submodule_ident);

   return ret;
}

static int app_new_data_status_ind(
   uint32_t                arep,
   uint32_t                crep,
   uint8_t                 changes)
{
   MAIN_LOG_DEBUG(PNET_MAIN_LOG, "APP: New data status 0x%02x\n", changes);

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
   MAIN_LOG_DEBUG(PNET_MAIN_LOG, "APP: Alarm indicated\n");

   os_event_set(main_events, EVENT_ALARM);

   return 0;
}

static int app_alarm_cnf(
   uint32_t                arep,
   pnet_pnio_status_t      *p_pnio_status)
{
   MAIN_LOG_DEBUG(PNET_MAIN_LOG, "APP: Alarm confirmed: %u, %u, %u, %u\n",
      p_pnio_status->error_code, p_pnio_status->error_decode,
      p_pnio_status->error_code_1, p_pnio_status->error_code_2);

   alarm_allowed = true;

   return 0;
}

static int app_alarm_ack_cnf(
   uint32_t                arep,
   int                     res)
{
   MAIN_LOG_DEBUG(PNET_MAIN_LOG, "APP: Alarm ACK confirmed: %d\n", res);

   return 0;
}

static void main_timer_tick(
   os_timer_t              *timer,
   void                    *arg)
{
   os_event_set(main_events, EVENT_TIMER);
}

int main(void)
{
   int         ret = -1;
   uint32_t    mask = EVENT_READY_FOR_DATA | EVENT_TIMER | EVENT_ALARM | EVENT_ABORT;
   uint32_t    flags = 0;
   uint32_t    tick_ctr = 0;
   uint8_t     data[] =
   {
    0x00,       /* Slot 1, subslot 1 Data */
   };

   uint16_t                ix;

   MAIN_LOG_INFO(PNET_MAIN_LOG, "APP: Hello world\n");
   main_events = os_event_create();
   main_timer  = os_timer_create(TICK_INTERVAL_US, main_timer_tick, NULL, false);

   strcpy(pnet_default_cfg.im_0_data.order_id, "12345");
   strcpy(pnet_default_cfg.im_0_data.im_serial_number, "00001");


   if (pnet_init("eth0", TICK_INTERVAL_US, &pnet_default_cfg) == 0)
   {
      MAIN_LOG_DEBUG(PNET_MAIN_LOG, "APP: Call-backs connected\n");
      for (ix = 0; ix < NELEMENTS(submodule_cfg); ix++)
      {
         /* Plug all submodules - modules are plugged automatically */
         if (pnet_plug_submodule(submodule_cfg[ix].api, submodule_cfg[ix].slot_nbr, submodule_cfg[ix].subslot_nbr,
            submodule_cfg[ix].module_ident_nbr, submodule_cfg[ix].submodule_ident_nbr,
            submodule_cfg[ix].data_dir, submodule_cfg[ix].insize, submodule_cfg[ix].outsize) != 0)
         {
            MAIN_LOG_ERROR(PNET_MAIN_LOG, "APP: Could not plug %u, %u, %u\n",
               (unsigned)submodule_cfg[ix].api, (unsigned)submodule_cfg[ix].slot_nbr, (unsigned)submodule_cfg[ix].subslot_nbr);
         }
      }



      if (init_done == true)
      {
         os_timer_stop(main_timer);
      }
      os_timer_start(main_timer);
      init_done = true;
   }
   else
   {
      MAIN_LOG_DEBUG(PNET_MAIN_LOG, "APP: Could not connect application call-backs\n");
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
         MAIN_LOG_DEBUG(PNET_MAIN_LOG, "APP: Return from apprdy: %d\n", ret);
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
            MAIN_LOG_ERROR(PNET_MAIN_LOG, "APP: Alarm ACK error %d\n", ret);
         }
         else
         {
            MAIN_LOG_INFO(PNET_MAIN_LOG, "APP: Alarm ACK sent\n");
         }
      }
      else if (flags & EVENT_TIMER)
      {
         os_event_clr(main_events, EVENT_TIMER); /* Re-arm */
         tick_ctr++;
#if 1
         /* Set output data every 10ms */
         if ((main_arep != UINT32_MAX) && (tick_ctr > 10))
         {
            tick_ctr = 0;
            data[0] = data_ctr++;
            ret = pnet_input_set_data_and_iops(0, 1, 1, data, sizeof(data), PNET_IOXS_GOOD);
            //MAIN_LOG_DEBUG(PNET_MAIN_LOG, "APP: Return from send  : %d\n", ret);
         }
#endif
#if 0
         /* Set new output when button 1 is pressed/released */
         pressed = false;
         os_get_button(0, &pressed);

         if (main_arep != UINT32_MAX)
         {
            if ((pressed == true) && (old_1_pressed == false))
            {
               MAIN_LOG_DEBUG(PNET_MAIN_LOG, "APP: New input\n");
               data[0] = data_ctr++;
               ret = pnet_input_set_data_and_iops(0, 1, 1, data, sizeof(data), PNET_IOXS_GOOD);
            }
         }
         old_1_pressed = pressed;
#endif
#if 0
         /* Create an alarm when button 2 is pressed/released */
         pressed = false;
         os_get_button(1, &pressed);

         if (main_arep != UINT32_MAX)
         {
            if ((pressed == true) && (old_2_pressed == false) && (alarm_allowed == true))
            {
               MAIN_LOG_DEBUG(PNET_MAIN_LOG, "APP: Send ALARM\n");
               alarm[0]++;
               alarm_allowed = true;
               ret = pnet_alarm_send_process_alarm(main_arep, 0, 1, 1, sizeof(alarm), 1, alarm);
            }
         }
         old_2_pressed = pressed;
#endif
         pnet_handle_periodic();
      }
      else if (flags & EVENT_ABORT)
      {
         /* Reset main */
         main_arep = UINT32_MAX;
         alarm_allowed = true;
         os_event_clr(main_events, EVENT_ABORT); /* Re-arm */
         MAIN_LOG_DEBUG(PNET_MAIN_LOG, "APP: abort\n");
      }
   }
   os_timer_destroy(main_timer);
   os_event_destroy(main_events);

   MAIN_LOG_INFO(PNET_MAIN_LOG, "APP: End of the world\n");
   return 0;
}
