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

#define _GNU_SOURCE /* For asprintf() */

#include "sampleapp_common.h"

#include "log.h"
#include "osal.h"
#include <pnet_api.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define APP_DEFAULT_ETHERNET_INTERFACE "eth0"
#define APP_THREAD_PRIORITY            15
#define APP_THREAD_STACKSIZE           4096 /* bytes */
#define APP_MAIN_SLEEPTIME_US          5000 * 1000

/************************* Utilities ******************************************/

static void main_timer_tick (os_timer_t * timer, void * arg)
{
   app_data_t * p_appdata = (app_data_t *)arg;

   os_event_set (p_appdata->main_events, EVENT_TIMER);
}

void show_usage()
{
   printf ("\nDemo application for p-net Profinet device stack.\n");
   printf ("\n");
   printf ("Wait for connection from IO-controller.\n");
   printf ("Then read buttons (input) and send to controller.\n");
   printf ("Listen for application LED output (from controller) and set "
           "application LED state.\n");
   printf ("It will also send a counter value (useful also without buttons and "
           "LED).\n");
   printf ("Button1 value is sent in the periodic data.\n");
   printf ("Button2 cycles through triggering an alarm, setting diagnosis and "
           " creating logbook entries.\n");
   printf ("\n");
   printf ("Also the mandatory Profinet signal LED is controlled by this "
           "application.\n");
   printf ("\n");
   printf ("The LEDs are controlled by the script set_profinet_leds_linux\n");
   printf ("located in the same directory as the application binary.\n");
   printf ("A version for Raspberry Pi is available, and also a version "
           "writing\n");
   printf ("to plain text files (useful for demo if no LEDs are available).\n");
   printf ("\n");
   printf ("Assumes the default gateway is found on .1 on same subnet as the "
           "IP address.\n");
   printf ("\n");
   printf ("Optional arguments:\n");
   printf ("   --help       Show this help text and exit\n");
   printf ("   -h           Show this help text and exit\n");
   printf ("   -v           Incresase verbosity\n");
   printf ("   -f           Reset to factory settings, and store to file. "
           "Exit.\n");
   printf ("   -r           Remove stored files and exit.\n");
   printf ("   -g           Show stack details and exit. Repeat for more "
           "details.\n");
   printf (
      "   -i INTERF    Name of Ethernet interface to use. Defaults to %s\n",
      APP_DEFAULT_ETHERNET_INTERFACE);
   printf (
      "   -s NAME      Set station name. Defaults to %s  Only used\n",
      APP_DEFAULT_STATION_NAME);
   printf ("                if not already available in storage file.\n");
   printf ("   -b FILE      Path (absolute or relative) to read button1. "
           "Defaults to not read button1.\n");
   printf ("   -d FILE      Path (absolute or relative) to read button2. "
           "Defaults to not read button2.\n");
   printf ("   -p PATH      Absolute path to storage directory. Defaults to "
           "use current directory.\n");
}

/**
 * Parse command line arguments
 *
 * @param argc      In: Number of arguments
 * @param argv      In: Arguments
 * @return Parsed arguments
 */
struct cmd_args parse_commandline_arguments (int argc, char * argv[])
{
   /* Special handling of long argument */
   if (argc > 1)
   {
      if (strcmp (argv[1], "--help") == 0)
      {
         show_usage();
         exit (EXIT_FAILURE);
      }
   }

   /* Default values */
   struct cmd_args output_arguments;
   strcpy (output_arguments.path_button1, "");
   strcpy (output_arguments.path_button2, "");
   strcpy (output_arguments.path_storage_directory, "");
   strcpy (output_arguments.station_name, APP_DEFAULT_STATION_NAME);
   strcpy (output_arguments.eth_interface, APP_DEFAULT_ETHERNET_INTERFACE);
   output_arguments.verbosity = 0;
   output_arguments.show = 0;
   output_arguments.factory_reset = false;
   output_arguments.remove_files = false;

   int option;
   while ((option = getopt (argc, argv, "hvgfri:s:b:d:p:")) != -1)
   {
      switch (option)
      {
      case 'v':
         output_arguments.verbosity++;
         break;
      case 'g':
         output_arguments.show++;
         break;
      case 'f':
         output_arguments.factory_reset = true;
         break;
      case 'r':
         output_arguments.remove_files = true;
         break;
      case 'i':
         strcpy (output_arguments.eth_interface, optarg);
         break;
      case 's':
         strcpy (output_arguments.station_name, optarg);
         break;
      case 'b':
         if (strlen (optarg) + 1 > PNET_MAX_FILE_FULLPATH_LEN)
         {
            printf ("Error: The argument to -b is too long.\n");
            exit (EXIT_FAILURE);
         }
         strcpy (output_arguments.path_button1, optarg);
         break;
      case 'd':
         if (strlen (optarg) + 1 > PNET_MAX_FILE_FULLPATH_LEN)
         {
            printf ("Error: The argument to -d is too long.\n");
            exit (EXIT_FAILURE);
         }
         strcpy (output_arguments.path_button2, optarg);
         break;
      case 'p':
         if (strlen (optarg) + 1 > PNET_MAX_FILE_FULLPATH_LEN)
         {
            printf ("Error: The argument to -p is too long.\n");
            exit (EXIT_FAILURE);
         }
         strcpy (output_arguments.path_storage_directory, optarg);
         break;
      case 'h':
         /* fallthrough */
      case '?':
         /* fallthrough */
      default:
         show_usage();
         exit (EXIT_FAILURE);
      }
   }

   /* Use current directory for storage, if not given */
   if (strlen (output_arguments.path_storage_directory) == 0)
   {
      if (
         getcwd (
            output_arguments.path_storage_directory,
            sizeof (output_arguments.path_storage_directory)) == NULL)
      {
         printf ("Error: Could not read current working directory. Is "
                 "PNET_MAX_DIRECTORYPATH_LENGTH too small?\n");
         exit (EXIT_FAILURE);
      }
   }

   return output_arguments;
}

/**
 * Check if a file or directory exists
 *
 * @param filepath         In:    Path to file or directory. Trailing slash
 *                                is optional for directories.
 * @return true if file exists
 */
bool does_file_exist (const char * filepath)
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
bool read_bool_from_file (const char * filepath)
{
   FILE * fp;
   char ch;
   int eof_indicator;

   fp = fopen (filepath, "r");
   if (fp == NULL)
   {
      return false;
   }

   ch = fgetc (fp);
   eof_indicator = feof (fp);
   fclose (fp);

   if (eof_indicator)
   {
      return false;
   }
   return ch == '1';
}

void app_get_button (const app_data_t * p_appdata, uint16_t id, bool * p_pressed)
{
   if (id == 0)
   {
      if (p_appdata->arguments.path_button1[0] != '\0')
      {
         *p_pressed = read_bool_from_file (p_appdata->arguments.path_button1);
      }
   }
   else if (id == 1)
   {
      if (p_appdata->arguments.path_button2[0] != '\0')
      {
         *p_pressed = read_bool_from_file (p_appdata->arguments.path_button2);
      }
   }
   else
   {
      *p_pressed = false;
   }
}

int app_set_led (uint16_t id, bool led_state)
{
   char * outputcommand;
   int textlen = -1;
   int status = -1;

   textlen = asprintf (
      &outputcommand,
      "./set_profinet_leds_linux %u %u",
      id,
      led_state);
   if (textlen < 0)
   {
      return -1;
   }
   os_log (
      LOG_LEVEL_DEBUG,
      "Command for setting LED state: %s\n",
      outputcommand);

   status = system (outputcommand);
   free (outputcommand);
   if (status != 0)
   {
      os_log (LOG_LEVEL_ERROR, "Failed to set LED state\n");
      return -1;
   }
   return 0;
}

/**
 * Run main loop in separate thread
 *
 * @param arg              In:    Thread argument   app_data_and_stack_t
 */
void pn_main_thread (void * arg)
{
   pnet_t * net;
   app_data_t * p_appdata;
   app_data_and_stack_t * appdata_and_stack;

   appdata_and_stack = (app_data_and_stack_t *)arg;
   p_appdata = appdata_and_stack->appdata;
   net = appdata_and_stack->net;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf ("Waiting for connect request from IO-controller\n\n");
   }

   app_loop_forever (net, p_appdata);

   os_timer_destroy (p_appdata->main_timer);
   os_event_destroy (p_appdata->main_events);
   printf ("Ending the application\n\n");
}

/****************************** Main ******************************************/

int main (int argc, char * argv[])
{
   pnet_t * net;
   pnet_cfg_t pnet_default_cfg;
   app_data_and_stack_t appdata_and_stack;
   app_data_t appdata;
   os_ethaddr_t macbuffer;
   os_ipaddr_t ip;
   os_ipaddr_t netmask;
   os_ipaddr_t gateway;
   int ret = 0;

   memset (&appdata, 0, sizeof (appdata));
   appdata.alarm_allowed = true;

   /* Parse and display command line arguments */
   appdata.arguments = parse_commandline_arguments (argc, argv);

   printf ("\n** Starting Profinet demo application **\n");
   if (appdata.arguments.verbosity > 0)
   {
      printf (
         "Number of slots:      %u (incl slot for DAP module)\n",
         PNET_MAX_MODULES);
      printf ("P-net log level:      %u (DEBUG=0, ERROR=3)\n", LOG_LEVEL);
      printf ("App verbosity level:  %u\n", appdata.arguments.verbosity);
      printf ("Ethernet interface:   %s\n", appdata.arguments.eth_interface);
      printf ("Button1 file:         %s\n", appdata.arguments.path_button1);
      printf ("Button2 file:         %s\n", appdata.arguments.path_button2);
      printf ("Default station name: %s\n", appdata.arguments.station_name);
   }

   /* Read IP, netmask, gateway and MAC address from operating system */
   ret = os_get_macaddress (appdata.arguments.eth_interface, &macbuffer);
   if (ret != 0)
   {
      printf (
         "Error: The given Ethernet interface does not exist: %s\n",
         appdata.arguments.eth_interface);
      exit (EXIT_FAILURE);
   }

   ip = os_get_ip_address (appdata.arguments.eth_interface);
   netmask = os_get_netmask (appdata.arguments.eth_interface);
   gateway = os_get_gateway (appdata.arguments.eth_interface);
   if (gateway == IP_INVALID)
   {
      printf (
         "Error: Invalid gateway IP address for Ethernet interface: %s\n",
         appdata.arguments.eth_interface);
      exit (EXIT_FAILURE);
   }

   if (appdata.arguments.verbosity > 0)
   {
      app_print_network_details (&macbuffer, ip, netmask, gateway);
   }

   /* Prepare stack config with IP address, gateway, station name etc */
   app_adjust_stack_configuration (&pnet_default_cfg);
   strcpy (pnet_default_cfg.im_0_data.im_order_id, "12345");
   strcpy (pnet_default_cfg.im_0_data.im_serial_number, "00001");
   app_copy_ip_to_struct (&pnet_default_cfg.ip_addr, ip);
   app_copy_ip_to_struct (&pnet_default_cfg.ip_gateway, gateway);
   app_copy_ip_to_struct (&pnet_default_cfg.ip_mask, netmask);
   strcpy (pnet_default_cfg.station_name, appdata.arguments.station_name);
   strcpy (
      pnet_default_cfg.file_directory,
      appdata.arguments.path_storage_directory);
   memcpy (
      pnet_default_cfg.eth_addr.addr,
      macbuffer.addr,
      sizeof (os_ethaddr_t));
   pnet_default_cfg.cb_arg = (void *)&appdata;

   if (appdata.arguments.verbosity > 0)
   {
      printf ("Storage directory:    %s\n\n", pnet_default_cfg.file_directory);
   }

   /* Validate paths */
   if (!does_file_exist (pnet_default_cfg.file_directory))
   {
      printf (
         "Error: The given storage directory does not exist: %s\n",
         pnet_default_cfg.file_directory);
      exit (EXIT_FAILURE);
   }
   if (appdata.arguments.path_button1[0] != '\0')
   {
      if (!does_file_exist (appdata.arguments.path_button1))
      {
         printf (
            "Error: The given input file for button1 does not exist: %s\n",
            appdata.arguments.path_button1);
         exit (EXIT_FAILURE);
      }
   }
   if (appdata.arguments.path_button2[0] != '\0')
   {
      if (!does_file_exist (appdata.arguments.path_button2))
      {
         printf (
            "Error: The given input file for button2 does not exist: %s\n",
            appdata.arguments.path_button2);
         exit (EXIT_FAILURE);
      }
   }

   /* Remove files and exit */
   if (appdata.arguments.remove_files == true)
   {
      printf ("\nRemoving stored files\n");
      (void)pnet_remove_data_files (pnet_default_cfg.file_directory);
      exit (EXIT_SUCCESS);
   }

   /* Initialize profinet stack */
   net = pnet_init (
      appdata.arguments.eth_interface,
      TICK_INTERVAL_US,
      &pnet_default_cfg);
   if (net == NULL)
   {
      printf ("Failed to initialize p-net. Do you have enough Ethernet "
              "interface permission?\n");
      exit (EXIT_FAILURE);
   }

   /* Do factory reset and exit */
   if (appdata.arguments.factory_reset == true)
   {
      printf ("\nPerforming factory reset\n");
      (void)pnet_factory_reset (net);
      exit (EXIT_SUCCESS);
   }

   /* Show stack info and exit */
   if (appdata.arguments.show > 0)
   {
      int level = 0xFFFF;

      printf ("\nShowing stack information.\n\n");
      if (appdata.arguments.show == 1)
      {
         level = 0x2010; /*  See documentation for pnet_show()  */
      }

      pnet_show (net, level);
      exit (EXIT_SUCCESS);
   }

   app_set_led (APP_DATA_LED_ID, false);
   app_plug_dap (net, &appdata);

   /* Start thread and timer */
   appdata_and_stack.appdata = &appdata;
   appdata_and_stack.net = net;
   appdata.main_events = os_event_create();
   appdata.main_timer = os_timer_create (
      TICK_INTERVAL_US,
      main_timer_tick,
      (void *)&appdata,
      false);
   os_thread_create (
      "pn_main_thread",
      APP_THREAD_PRIORITY,
      APP_THREAD_STACKSIZE,
      pn_main_thread,
      (void *)&appdata_and_stack);
   os_timer_start (appdata.main_timer);

   for (;;)
      os_usleep (APP_MAIN_SLEEPTIME_US);

   return 0;
}
