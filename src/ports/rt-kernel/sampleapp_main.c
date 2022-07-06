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
#include "app_utils.h"
#include "app_log.h"
#include "app_gsdml.h"

#include "osal_log.h"
#include "osal.h"
#include <pnet_api.h>

#include <kern/kern.h>
#include <lwip/netif.h>
#include <shell.h>

#include <string.h>

#define APP_DEFAULT_ETHERNET_INTERFACE "en1"
#define APP_DEFAULT_FILE_DIRECTORY     "/disk1"
#define APP_LOG_LEVEL                  APP_LOG_LEVEL_INFO

#define APP_BG_WORKER_THREAD_PRIORITY  2
#define APP_BG_WORKER_THREAD_STACKSIZE 4096 /* bytes */

/********************************** Globals ***********************************/

static app_data_t * sample_app = NULL;
static pnet_cfg_t pnet_cfg = {0};
app_args_t app_args = {0};

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

   pnet_show (app_get_pnet_instance (sample_app), level);
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
   (void)pnet_factory_reset (app_get_pnet_instance (sample_app));

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

/**
 * Suppress RT-Kernel banner during startup
 */
void shell_banner (void)
{
   /* Do not print message since it is run in another context
    * and is not aligned with application log
    */
}

/****************************** Main ******************************************/

int main (void)
{
   int ret;
   app_utils_netif_namelist_t netif_name_list;
   pnet_if_cfg_t netif_cfg = {0};
   uint16_t number_of_ports;

   strcpy (app_args.eth_interfaces, APP_DEFAULT_ETHERNET_INTERFACE);
   strcpy (app_args.station_name, APP_GSDML_DEFAULT_STATION_NAME);
   app_log_set_log_level (APP_LOG_LEVEL);

   APP_LOG_INFO ("\n** Starting P-Net sample application " PNET_VERSION
                 " **\n");
   APP_LOG_INFO ("\nType help to get a list of supported shell commands.\n"
                 "Type help <cmd> to get a command description.\n"
                 "For example: help pnio_show\n\n");
   APP_LOG_INFO (
      "Number of slots:      %u (incl slot for DAP module)\n",
      PNET_MAX_SLOTS);
   APP_LOG_INFO ("P-net log level:      %u (DEBUG=0, FATAL=4)\n", LOG_LEVEL);
   APP_LOG_INFO ("App log level:        %u (DEBUG=0, FATAL=4)\n", APP_LOG_LEVEL);
   APP_LOG_INFO ("Max number of ports:  %u\n", PNET_MAX_PHYSICAL_PORTS);
   APP_LOG_INFO ("Network interfaces:   %s\n", app_args.eth_interfaces);
   APP_LOG_INFO ("Default station name: %s\n", app_args.station_name);

   app_pnet_cfg_init_default (&pnet_cfg);

   /* Note: station name is defined by app_gsdml.h */

   strcpy (pnet_cfg.file_directory, APP_DEFAULT_FILE_DIRECTORY);

   ret = app_utils_pnet_cfg_init_netifs (
      app_args.eth_interfaces,
      &netif_name_list,
      &number_of_ports,
      &netif_cfg);
   if (ret != 0)
   {
      return -1;
   }

   pnet_cfg.if_cfg = netif_cfg;
   pnet_cfg.num_physical_ports = number_of_ports;

   app_utils_print_network_config (&netif_cfg, number_of_ports);

   pnet_cfg.pnal_cfg.bg_worker_thread.prio = APP_BG_WORKER_THREAD_PRIORITY;
   pnet_cfg.pnal_cfg.bg_worker_thread.stack_size =
      APP_BG_WORKER_THREAD_STACKSIZE;

   /* Initialize profinet stack */
   sample_app = app_init (&pnet_cfg, &app_args);
   if (sample_app == NULL)
   {
      printf ("Failed to initialize P-Net.\n");
      printf ("Aborting application\n");
      return -1;
   }

   /* Start main loop */
   if (app_start (sample_app, RUN_IN_MAIN_THREAD) != 0)
   {
      printf ("Failed to start\n");
      printf ("Aborting application\n");
      return -1;
   }

   app_loop_forever (sample_app);

   return 0;
}
