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

#include "osal_log.h"
#include "osal.h"
#include <pnet_api.h>

#include <gpio.h>
#include <kern/kern.h>
#include <lwip/netif.h>
#include <shell.h>

#include <string.h>

#define GPIO_LED1                      GPIO_P5_9
#define GPIO_LED2                      GPIO_P5_8
#define GPIO_BUTTON1                   GPIO_P15_13
#define GPIO_BUTTON2                   GPIO_P15_12
#define APP_DEFAULT_ETHERNET_INTERFACE "en1"
#define APP_DEFAULT_FILE_DIRECTORY     "/disk1"

/********************************** Globals ***********************************/

static app_data_t * gp_appdata = NULL;
static pnet_t * g_net = NULL;
static pnet_cfg_t pnet_default_cfg;

/************************* Utilities ******************************************/

int app_set_led (uint16_t id, bool led_state, int verbosity)
{
   if (id == APP_DATA_LED_ID)
   {
      gpio_set (GPIO_LED1, led_state ? 1 : 0); /* "LED1" on circuit board */
   }
   else if (id == APP_PROFINET_SIGNAL_LED_ID)
   {
      gpio_set (GPIO_LED2, led_state ? 1 : 0); /* "LED2" on circuit board */
   }

   return 0;
}

void app_get_button (const app_data_t * p_appdata, uint16_t id, bool * p_pressed)
{
   if (id == 0)
   {
      *p_pressed = (gpio_get (GPIO_BUTTON1) == 0);
   }
   else if (id == 1)
   {
      *p_pressed = (gpio_get (GPIO_BUTTON2) == 0);
   }
   else
   {
      *p_pressed = false;
   }
}

static void main_timer_tick (os_timer_t * timer, void * arg)
{
   os_event_set (gp_appdata->main_events, APP_EVENT_TIMER);
}

/************************* Shell commands *************************************/
/*  Press enter in the terminal to get to the built-in shell.                 */
/*  Type help to see available commands.                                      */

static int _cmd_pnio_show (int argc, char * argv[])
{
   unsigned long level = 0;
   char * end = NULL;
   extern char * os_kernel_revision;

   if (argc == 1)
   {
      level = 0x2010; /*  See documentation for pnet_show()  */
   }
   else if (argc == 2)
   {
      level = strtoul (argv[1], &end, 0);
      if (end == argv[1] || level == 0)
      {
         shell_usage (argv[0], "level must be an integer > 0");
         return -1;
      }
   }
   else
   {
      shell_usage (argv[0], "wrong number of arguments");
      return -1;
   }

   pnet_show (g_net, level);
   printf ("\n");
   printf (
      "App parameter 1 = 0x%08x\n",
      (unsigned)ntohl (gp_appdata->app_param_1));
   printf (
      "App parameter 2 = 0x%08x\n",
      (unsigned)ntohl (gp_appdata->app_param_2));
   printf ("\n");
   printf ("p-net revision: " PNET_VERSION "\n");
   printf ("rt-kernel revision: %s\n", os_kernel_revision);

   return 0;
}

const shell_cmd_t cmd_pnio_show = {
   .cmd = _cmd_pnio_show,
   .name = "pnio_show",
   .help_short = "Show pnio [level]",
   .help_long = "Show pnio [level]. Defaults to 8208. Use 65535 to show all."};
SHELL_CMD (cmd_pnio_show);

static int _cmd_pnio_factory_reset (int argc, char * argv[])
{
   printf ("Factory reset\n");
   (void)pnet_factory_reset (g_net);

   return 0;
}

const shell_cmd_t cmd_pnio_factory_reset = {
   .cmd = _cmd_pnio_factory_reset,
   .name = "pnio_factory_reset",
   .help_short = "Perform factory reset, and save to file",
   .help_long = "Perform factory reset, and save to file"};
SHELL_CMD (cmd_pnio_factory_reset);

static int _cmd_pnio_remove_files (int argc, char * argv[])
{
   printf ("Deleting data files\n");
   (void)pnet_remove_data_files (APP_DEFAULT_FILE_DIRECTORY);

   return 0;
}

const shell_cmd_t cmd_pnio_remove_files = {
   .cmd = _cmd_pnio_remove_files,
   .name = "pnio_remove_files",
   .help_short = "Remove data files",
   .help_long = "Remove data files"};
SHELL_CMD (cmd_pnio_remove_files);

/****************************** Main ******************************************/

int main (void)
{
   int ret = -1;
   app_data_t appdata;
   g_net = NULL;
   gp_appdata = &appdata;
   uint16_t number_of_ports = 0;

   /* Prepare appdata */
   memset (&appdata, 0, sizeof (appdata));
   appdata.alarm_allowed = true;
   strcpy (appdata.arguments.eth_interfaces, APP_DEFAULT_ETHERNET_INTERFACE);
   strcpy (appdata.arguments.station_name, APP_DEFAULT_STATION_NAME);
   appdata.arguments.verbosity = (LOG_LEVEL <= LOG_LEVEL_WARNING) ? 1 : 0;

   appdata.main_events = os_event_create();
   appdata.main_timer =
      os_timer_create (APP_TICK_INTERVAL_US, main_timer_tick, NULL, false);

   printf ("\n** Profinet sample application **\n");
   if (appdata.arguments.verbosity > 0)
   {
      printf (
         "Number of slots:      %u (incl slot for DAP module)\n",
         PNET_MAX_SLOTS);
      printf ("P-net log level:      %u (DEBUG=0, FATAL=4)\n", LOG_LEVEL);
      printf ("App verbosity level:  %u\n", appdata.arguments.verbosity);
      printf ("Max number of ports:  %u\n", PNET_MAX_PHYSICAL_PORTS);
      printf ("Network interfaces:   %s\n", appdata.arguments.eth_interfaces);
      printf ("Default station name: %s\n", appdata.arguments.station_name);
   }

   app_pnet_cfg_init_default (&pnet_default_cfg);
   pnet_default_cfg.cb_arg = (void *)&appdata;
   strcpy (pnet_default_cfg.station_name, appdata.arguments.station_name);
   strcpy (pnet_default_cfg.file_directory, APP_DEFAULT_FILE_DIRECTORY);
   /* Note: pnal_cfg not is used for rt-kernel  */

   ret = app_pnet_cfg_init_netifs (
      appdata.arguments.eth_interfaces,
      &appdata.if_list,
      &number_of_ports,
      &pnet_default_cfg,
      appdata.arguments.verbosity);
   if (ret != 0)
   {
      return -1;
   }
   pnet_default_cfg.num_physical_ports = number_of_ports;

   /* Initialize profinet stack */
   g_net = pnet_init (&pnet_default_cfg);
   if (g_net == NULL)
   {
      printf ("Failed to initialize p-net application.\n");
      return -1;
   }

   os_timer_start (gp_appdata->main_timer);

   app_loop_forever (g_net, &appdata);

   os_timer_destroy (appdata.main_timer);
   os_event_destroy (appdata.main_events);
   printf ("Quitting the sample application\n\n");

   return 0;
}
