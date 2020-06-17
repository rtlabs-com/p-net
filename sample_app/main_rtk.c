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

#include "sampleapp_common.h"

#include "log.h"
#include "osal.h"
#include <pnet_api.h>

#include <gpio.h>
#include <kern.h>
#include <lwip/netif.h>
#include <shell.h>

#include <string.h>

#define GPIO_LED1                      GPIO_P5_9
#define GPIO_LED2                      GPIO_P5_8
#define GPIO_BUTTON1                   GPIO_P15_13
#define GPIO_BUTTON2                   GPIO_P15_12
#define APP_DEFAULT_ETHERNET_INTERFACE "en1"


/********************************** Globals ***********************************/

static app_data_t                  *gp_appdata = NULL;
static pnet_t                      *g_net = NULL;
static pnet_cfg_t                  pnet_default_cfg;


/************************* Utilities ******************************************/

 int app_set_led(
   uint16_t                id,
   bool                    led_state)
{
   if (id == APP_DATA_LED_ID)
   {
      gpio_set(GPIO_LED1, led_state ? 1 : 0);  /* "LED1" on circuit board */
   }
   else if (id == APP_PROFINET_SIGNAL_LED_ID)
   {
      gpio_set(GPIO_LED2, led_state ? 1 : 0);  /* "LED2" on circuit board */
   }

   return 0;
}


static void app_get_button(uint16_t id, bool *p_pressed)
{
   if (id == 0)
   {
      *p_pressed = (gpio_get(GPIO_BUTTON1) == 0);
   }
   else if (id == 1)
   {
      *p_pressed = (gpio_get(GPIO_BUTTON2) == 0);
   }
   else
   {
      *p_pressed = false;
   }
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

   if (g_net != NULL)
   {
      printf("Already initialized p-net application.\n");
      return 0;
   }

   if (argc == 2)
   {
      strcpy(gp_appdata->arguments.station_name, argv[1]);
   }
   else{
      strcpy(gp_appdata->arguments.station_name, APP_DEFAULT_STATION_NAME);
   }

   app_adjust_stack_configuration(&pnet_default_cfg);
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
      app_plug_dap(g_net);

      if (gp_appdata->arguments.verbosity > 0)
      {
         printf("\nStation name:        %s\n", gp_appdata->arguments.station_name);
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
      .help_short = "Start pnio sample app",
      .help_long = "pnio_run [station name]\n\n"
                   "Start the pnio sample app. Station name defaults to "
                   APP_DEFAULT_STATION_NAME " if not given.\n"
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

   memset(&cmdline_arguments, 0, sizeof(cmdline_arguments));
   strcpy(cmdline_arguments.eth_interface, APP_DEFAULT_ETHERNET_INTERFACE);
   cmdline_arguments.verbosity = (LOG_LEVEL <= LOG_LEVEL_WARNING) ? 1 : 0;

   gp_appdata = &appdata;

   memset(&appdata, 0, sizeof(appdata));
   appdata.alarm_allowed = true;
   appdata.main_arep = UINT32_MAX;
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
   }

   /* Main loop */
   for (;;)
   {
      os_event_wait(appdata.main_events, mask, &flags, OS_WAIT_FOREVER);
      if (flags & EVENT_READY_FOR_DATA)
      {
         os_event_clr(appdata.main_events, EVENT_READY_FOR_DATA); /* Re-arm */
         if (appdata.arguments.verbosity > 0)
         {
            printf("Application will signal that it is ready for data.\n");
         }
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

            app_get_button(0, &button1_pressed);
            app_get_button(1, &button2_pressed);
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
                  app_set_led(APP_DATA_LED_ID, received_led_state);
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
