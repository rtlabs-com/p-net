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
#include "app_gsdml.h"
#include "app_log.h"
#include "app_utils.h"

#include "osal.h"
#include "osal_log.h" /* For LOG_LEVEL */
#include "pnal.h"
#include "pnal_filetools.h"
#include <pnet_api.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if PNET_MAX_PHYSICAL_PORTS == 1
#define APP_DEFAULT_ETHERNET_INTERFACE "eth0"
#else
#define APP_DEFAULT_ETHERNET_INTERFACE "br0,eth0,eth1"
#endif

#define APP_MAIN_SLEEPTIME_US          5000 * 1000
#define APP_SNMP_THREAD_PRIORITY       1
#define APP_SNMP_THREAD_STACKSIZE      256 * 1024 /* bytes */
#define APP_ETH_THREAD_PRIORITY        10
#define APP_ETH_THREAD_STACKSIZE       4096 /* bytes */
#define APP_BG_WORKER_THREAD_PRIORITY  5
#define APP_BG_WORKER_THREAD_STACKSIZE 4096 /* bytes */

/* Note that this sample application uses os_timer_create() for the timer
   that controls the ticks. It is implemented in OSAL, and the Linux
   implementation uses a thread internally. To modify the timer thread priority,
   modify OSAL or use some other timer */

app_args_t app_args = {0};

/************************* Utilities ******************************************/

void show_usage()
{
   printf ("\nSample application for p-net Profinet device stack.\n");
   printf ("\n");
   printf ("Wait for connection from IO-controller.\n");
   printf ("Then read buttons (input) and send to controller.\n");
   printf ("Listen for application LED output (from controller) and set "
           "application LED state.\n");
   printf ("It will also send a counter value (useful also without buttons and "
           "LED).\n");
   printf ("Button1 value is sent in the periodic data.\n");
   printf ("Button2 cycles through triggering an alarm, setting diagnosis and "
           "creating logbook entries.\n");
   printf ("\n");
   printf ("Also the mandatory Profinet signal LED is controlled by this "
           "application.\n");
   printf ("\n");
   printf ("The LEDs are controlled by the script set_profinet_leds\n");
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
   printf ("   -v           Incresase verbosity. Can be repeated.\n");
   printf ("   -f           Reset to factory settings, and store to file. "
           "Exit.\n");
   printf ("                Remember to give the -p flag if necessary.\n");
   printf ("   -r           Remove stored files and exit.\n");
   printf ("                Remember to give the -p flag if necessary.\n");
   printf ("   -g           Show stack details and exit. Repeat for more "
           "details.\n");
   printf (
      "   -i INTERF    Name of Ethernet interface to use. Defaults to %s\n",
      APP_DEFAULT_ETHERNET_INTERFACE);
   printf ("                Comma separated list if more than one interface "
           "given.\n");
   printf (
      "   -s NAME      Set station name. Defaults to \"%s\". Only used\n",
      APP_GSDML_DEFAULT_STATION_NAME);
   printf ("                if not already available in storage file.\n");
   printf ("   -b FILE      Path (absolute or relative) to read Button1. "
           "Defaults to not read Button1.\n");
   printf ("   -d FILE      Path (absolute or relative) to read Button2. "
           "Defaults to not read Button2.\n");
   printf ("   -p PATH      Absolute path to storage directory. Defaults to "
           "use current directory.\n");
#if PNET_OPTION_DRIVER_ENABLE
   printf ("   -m MODE      Application offload mode. Only used if P-Net is\n");
   printf ("                built with hw offload enabled "
           "                (PNET_OPTION_DRIVER_ENABLE). \n");
   printf ("                Supported modes: none, cpu, full\n");
   printf ("                Defaults to none\n");
#endif
   printf ("\n");
   printf ("p-net revision: " PNET_VERSION "\n");
}

/**
 * Parse command line arguments
 *
 * @param argc      In: Number of arguments
 * @param argv      In: Arguments
 * @return Parsed arguments
 */
app_args_t parse_commandline_arguments (int argc, char * argv[])
{
   app_args_t output_arguments = {0};
   int option;

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
   strcpy (output_arguments.path_button1, "");
   strcpy (output_arguments.path_button2, "");
   strcpy (output_arguments.path_storage_directory, "");
   strcpy (output_arguments.station_name, APP_GSDML_DEFAULT_STATION_NAME);
   strcpy (output_arguments.eth_interfaces, APP_DEFAULT_ETHERNET_INTERFACE);
   output_arguments.verbosity = 0;
   output_arguments.show = 0;
   output_arguments.factory_reset = false;
   output_arguments.remove_files = false;
   output_arguments.mode = MODE_HW_OFFLOAD_NONE;

   while ((option = getopt (argc, argv, "hvgfri:s:b:d:p:m:")) != -1)
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
         if ((strlen (optarg) + 1) > sizeof (output_arguments.eth_interfaces))
         {
            printf ("Error: The argument to -i is too long.\n");
            exit (EXIT_FAILURE);
         }
         strcpy (output_arguments.eth_interfaces, optarg);
         break;
      case 's':
         strcpy (output_arguments.station_name, optarg);
         break;
      case 'b':
         if (strlen (optarg) + 1 > PNET_MAX_FILE_FULLPATH_SIZE)
         {
            printf ("Error: The argument to -b is too long.\n");
            exit (EXIT_FAILURE);
         }
         strcpy (output_arguments.path_button1, optarg);
         break;
      case 'd':
         if (strlen (optarg) + 1 > PNET_MAX_FILE_FULLPATH_SIZE)
         {
            printf ("Error: The argument to -d is too long.\n");
            exit (EXIT_FAILURE);
         }
         strcpy (output_arguments.path_button2, optarg);
         break;
      case 'p':
         if (strlen (optarg) + 1 > PNET_MAX_FILE_FULLPATH_SIZE)
         {
            printf ("Error: The argument to -p is too long.\n");
            exit (EXIT_FAILURE);
         }
         strcpy (output_arguments.path_storage_directory, optarg);
         break;
#if PNET_OPTION_DRIVER_ENABLE
      case 'm':
         if (strcmp ("none", optarg) == 0)
         {
            output_arguments.mode = MODE_HW_OFFLOAD_NONE;
         }
         else if (strcmp ("cpu", optarg) == 0)
         {
            output_arguments.mode = MODE_HW_OFFLOAD_CPU;
         }
         else if (strcmp ("full", optarg) == 0)
         {
            output_arguments.mode = MODE_HW_OFFLOAD_FULL;
         }
         else
         {
            printf ("Error: mode (-m) not supported.\n");
            exit (EXIT_FAILURE);
         }
         break;
#endif
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
                 "PNET_MAX_DIRECTORYPATH_SIZE too small?\n");
         exit (EXIT_FAILURE);
      }
   }

   return output_arguments;
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

bool app_get_button (uint16_t id)
{
   if (id == 0)
   {
      if (app_args.path_button1[0] != '\0')
      {
         return read_bool_from_file (app_args.path_button1);
      }
   }
   else if (id == 1)
   {
      if (app_args.path_button2[0] != '\0')
      {
         return read_bool_from_file (app_args.path_button2);
      }
   }
   return false;
}

void app_set_led (uint16_t id, bool led_state)
{
   /* Important:
    * The Linux sample application uses a script to set the LED state,
    * for easy adaption to different development boards.
    *
    * The script typically writes to files in the /sys directory to set LED
    * state via GPIO operations. If you do not have any physical LEDs you can
    * use a script that writes to regular files instead.
    *
    * However, file operations shall be avoided within the main task
    * in a real application. File operations may affect the timing of the
    * Profinet communication depending on file system implementation.
    */

   char id_str[7] = {0}; /** Terminated string */
   const char * argv[4];

   sprintf (id_str, "%u", id);
   id_str[sizeof (id_str) - 1] = '\0';

   argv[0] = "set_profinet_leds";
   argv[1] = (char *)&id_str;
   argv[2] = (led_state == 1) ? "1" : "0";
   argv[3] = NULL;

   if (pnal_execute_script (argv) != 0)
   {
      printf ("Failed to set LED state\n");
   }
}

/** Update configuration with file storage path.
 *  Validate this path, and Linux button file paths
 *
 * @param p_cfg      InOut: Configuration to be updated
 * @param p_args     In:    Command line arguments
 * @return 0 on success, -1 on error.
 */
static int app_pnet_cfg_init_storage (
   pnet_cfg_t * p_cfg,
   const app_args_t * p_args)
{
   strcpy (p_cfg->file_directory, p_args->path_storage_directory);

   if (p_args->verbosity > 0)
   {
      printf ("Storage directory:    %s\n\n", p_cfg->file_directory);
   }

   /* Validate paths */
   if (!pnal_does_file_exist (p_cfg->file_directory))
   {
      printf (
         "Error: The given storage directory does not exist: %s\n",
         p_cfg->file_directory);
      return -1;
   }

   if (p_args->path_button1[0] != '\0')
   {
      if (!pnal_does_file_exist (p_args->path_button1))
      {
         printf (
            "Error: The given input file for Button1 does not exist: %s\n",
            p_args->path_button1);
         return -1;
      }
   }

   if (p_args->path_button2[0] != '\0')
   {
      if (!pnal_does_file_exist (p_args->path_button2))
      {
         printf (
            "Error: The given input file for Button2 does not exist: %s\n",
            p_args->path_button2);
         return -1;
      }
   }
   return 0;
}

/****************************** Main ******************************************/

int main (int argc, char * argv[])
{
   int ret;
   int32_t app_log_level = APP_LOG_LEVEL_FATAL;
   pnet_cfg_t pnet_cfg = {0};
   app_data_t * sample_app = NULL;
   app_utils_netif_namelist_t netif_name_list;
   pnet_if_cfg_t netif_cfg = {0};
   uint16_t number_of_ports = 1;

   /* Enable line buffering for printouts, especially when logging to
      the journal (which is default when running as a systemd job) */
   setvbuf (stdout, NULL, _IOLBF, 0);

   /* Parse and display command line arguments */
   app_args = parse_commandline_arguments (argc, argv);

   app_log_level = (app_args.verbosity <= APP_LOG_LEVEL_FATAL)
                      ? APP_LOG_LEVEL_FATAL - app_args.verbosity
                      : APP_LOG_LEVEL_DEBUG;
   app_log_set_log_level (app_log_level);
   printf ("\n** Starting P-Net sample application " PNET_VERSION " **\n");

   APP_LOG_INFO (
      "Number of slots:      %u (incl slot for DAP module)\n",
      PNET_MAX_SLOTS);
   APP_LOG_INFO ("P-net log level:      %u (DEBUG=0, FATAL=4)\n", LOG_LEVEL);
   APP_LOG_INFO ("App log level:        %u (DEBUG=0, FATAL=4)\n", app_log_level);
   APP_LOG_INFO ("Max number of ports:  %u\n", PNET_MAX_PHYSICAL_PORTS);
   APP_LOG_INFO ("Network interfaces:   %s\n", app_args.eth_interfaces);
   APP_LOG_INFO ("Button1 file:         %s\n", app_args.path_button1);
   APP_LOG_INFO ("Button2 file:         %s\n", app_args.path_button2);
   APP_LOG_INFO ("Default station name: %s\n", app_args.station_name);

   /* Prepare configuration */
   app_pnet_cfg_init_default (&pnet_cfg);
   strcpy (pnet_cfg.station_name, app_args.station_name);
   ret = app_utils_pnet_cfg_init_netifs (
      app_args.eth_interfaces,
      &netif_name_list,
      &number_of_ports,
      &netif_cfg);
   if (ret != 0)
   {
      exit (EXIT_FAILURE);
   }
   pnet_cfg.if_cfg = netif_cfg;
   pnet_cfg.num_physical_ports = number_of_ports;

   app_utils_print_network_config (&netif_cfg, number_of_ports);

   /* Operating system specific settings */
   pnet_cfg.pnal_cfg.snmp_thread.prio = APP_SNMP_THREAD_PRIORITY;
   pnet_cfg.pnal_cfg.snmp_thread.stack_size = APP_SNMP_THREAD_STACKSIZE;
   pnet_cfg.pnal_cfg.eth_recv_thread.prio = APP_ETH_THREAD_PRIORITY;
   pnet_cfg.pnal_cfg.eth_recv_thread.stack_size = APP_ETH_THREAD_STACKSIZE;
   pnet_cfg.pnal_cfg.bg_worker_thread.prio = APP_BG_WORKER_THREAD_PRIORITY;
   pnet_cfg.pnal_cfg.bg_worker_thread.stack_size =
      APP_BG_WORKER_THREAD_STACKSIZE;

   ret = app_pnet_cfg_init_storage (&pnet_cfg, &app_args);
   if (ret != 0)
   {
      printf ("Failed to initialize storage.\n");
      printf ("Aborting application\n");
      exit (EXIT_FAILURE);
   }

   /* Remove files and exit */
   if (app_args.remove_files == true)
   {
      printf ("\nRemoving stored files\n");
      printf ("Exit application\n");
      (void)pnet_remove_data_files (pnet_cfg.file_directory);
      exit (EXIT_SUCCESS);
   }

   /* Initialise stack and application */
   sample_app = app_init (&pnet_cfg, &app_args);
   if (sample_app == NULL)
   {
      printf ("Failed to initialize P-Net.\n");
      printf ("Do you have enough Ethernet interface permission?\n");
      printf ("Aborting application\n");
      exit (EXIT_FAILURE);
   }

   /* Do factory reset and exit */
   if (app_args.factory_reset == true)
   {
      printf ("\nPerforming factory reset\n");
      printf ("Exit application\n");
      (void)pnet_factory_reset (app_get_pnet_instance (sample_app));
      exit (EXIT_SUCCESS);
   }

   /* Show stack info and exit */
   if (app_args.show > 0)
   {
      int level = 0xFFFF;

      printf ("\nShowing stack information.\n\n");
      if (app_args.show == 1)
      {
         level = 0x2010; /* See documentation for pnet_show() */
      }

      pnet_show (app_get_pnet_instance (sample_app), level);
      printf ("Exit application\n");
      exit (EXIT_SUCCESS);
   }

   /* Start main loop */
   if (app_start (sample_app, RUN_IN_SEPARATE_THREAD) != 0)
   {
      printf ("Failed to start\n");
      printf ("Aborting application\n");
      exit (EXIT_FAILURE);
   }

   for (;;)
   {
      os_usleep (APP_MAIN_SLEEPTIME_US);
   }

   return 0;
}
