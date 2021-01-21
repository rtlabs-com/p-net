/*********************************************************************
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#include "sampleapp_common.h"
#include "pcap_helper.h"

#include "osal_log.h"
#include "osal.h"
#include <pnet_api.h>
#include "version.h"

#include <string.h>

#define APP_DEFAULT_ETHERNET_INTERFACE ""   /* will result in selection */

/********************************** Globals ***********************************/

static app_data_t * gp_appdata = NULL;
static pnet_t * g_net = NULL;
static pnet_cfg_t pnet_default_cfg;

/************************* Utilities ******************************************/

int app_set_led (uint16_t id, bool led_state, int verbosity)
{
   if (id == APP_DATA_LED_ID)
   {
      
   }
   else if (id == APP_PROFINET_SIGNAL_LED_ID)
   {
      
   }

   return 0;
}

void app_get_button (const app_data_t * p_appdata, uint16_t id, bool * p_pressed)
{
   *p_pressed = false;

   if (id == 0)
   {
      
   }
   else if (id == 1)
   {
      
   }

}

static void main_timer_tick (os_timer_t * timer, void * arg)
{
   os_event_set (gp_appdata->main_events, APP_EVENT_TIMER);
}

/* =============== small mini implementation of getopt =============== */
/* This is just enough to make the parse_commandline_arguments work.   */
/* It does *not* compare to the real getopt                            */

static char * optarg = NULL;
static int optind = -1;

static int getopt (int argc, char * argv[], const char * optstring)
{
   while (++optind < argc)
   {
      if (argv[optind][0] == '-')
      {
        optarg = argv[optind + 1];
        return argv[optind][1];
      }
   }
   return -1;
}

/* =================================================================== */

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
   printf ("The LEDs are controlled by the script set_profinet_leds_linux\n");
   printf ("located in the same directory as the application binary.\n");
   printf ("A version for Raspberry Pi is available, and also a version "
           "writing\n");
   printf ("to plain text files (useful for demo if no LEDs are available).\n");
   printf ("\n");
   printf ("\n");
   printf ("Optional arguments:\n");
   printf ("   --help       Show this help text and exit\n");
   printf ("   -h           Show this help text and exit\n");
   printf ("   -v           Incresase verbosity. Can be repeated.\n");
   printf ("   -f           Reset to factory settings, and store to file. "
           "Exit.\n");
   printf ("   -r           Remove stored files and exit.\n");
   printf ("   -g           Show stack details and exit. Repeat for more "
           "details.\n");
   printf (
      "   -i INTERF    Name of Ethernet interface to use. Eg. eth1. Defaults to Console User Selection\n");
   printf (
      "   -s NAME      Set station name. Defaults to %s  Only used\n",
      APP_DEFAULT_STATION_NAME);
   printf ("                if not already available in storage file.\n");
   printf ("   -b FILE      Path (absolute or relative) to read Button1. "
           "Defaults to not read Button1.\n");
   printf ("   -d FILE      Path (absolute or relative) to read Button2. "
           "Defaults to not read Button2.\n");
   printf ("   -p PATH      Absolute path to storage directory. Defaults to "
           "use current directory.\n");
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

/****************************** Main ******************************************/

int main (int argc, char * argv[])
{
   int ret = -1;
   app_data_t appdata;
   pnal_ethaddr_t macbuffer;
   pnal_ipaddr_t ip;
   pnal_ipaddr_t netmask;
   pnal_ipaddr_t gateway;

   g_net = NULL;

   /* Prepare appdata */
   memset (&appdata, 0, sizeof (appdata));
   appdata.alarm_allowed = true;
   appdata.arguments = parse_commandline_arguments (argc, argv); /* Parse and
                                                                    display
                                                                    command line
                                                                    arguments */
   appdata.main_events = os_event_create();
   appdata.main_timer =
      os_timer_create (APP_TICK_INTERVAL_US, main_timer_tick, NULL, false);
   gp_appdata = &appdata;

   printf ("\n** Profinet sample application **\n");
   if (appdata.arguments.verbosity > 0)
   {
      printf (
         "Number of slots:      %u (incl slot for DAP module)\n",
         PNET_MAX_SLOTS);
      printf ("P-net log level:      %u (DEBUG=0, FATAL=4)\n", LOG_LEVEL);
      printf ("App verbosity level:  %u\n", appdata.arguments.verbosity);
      printf ("Ethernet interface:   %s\n", appdata.arguments.eth_interface);
      printf ("Button1 file:         %s\n", appdata.arguments.path_button1);
      printf ("Button2 file:         %s\n", appdata.arguments.path_button2);
      printf ("Default station name: %s\n", appdata.arguments.station_name);
   }

   /* Select network adapter */
   pcap_helper_init();
   if (appdata.arguments.eth_interface[0] == 0)
      if (pcap_helper_list_and_select_adapter (appdata.arguments.eth_interface) < 0)
         return -1;
   if (pcap_helper_populate_adapter_info (appdata.arguments.eth_interface) < 0)
      return -1;

   /* Read IP, netmask, gateway and MAC address from operating system */
   ret = pnal_get_macaddress (appdata.arguments.eth_interface, &macbuffer);
   if (ret < 0)
   {
      printf (
         "Error: The given Ethernet interface does not exist: %s\n",
         appdata.arguments.eth_interface);
      return -1;
   }

   ip = pnal_get_ip_address (appdata.arguments.eth_interface);
   netmask = pnal_get_netmask (appdata.arguments.eth_interface);
   gateway = pnal_get_gateway (appdata.arguments.eth_interface);
   if (gateway == IP_INVALID)
   {
      printf (
         "Error: Invalid gateway IP address for Ethernet interface: %s\n",
         appdata.arguments.eth_interface);
      return -1;
   }

   if (appdata.arguments.verbosity > 0)
   {
      app_print_network_details (&macbuffer, ip, netmask, gateway);
   }

   /* Prepare stack config with IP address, gateway, station name etc */
   app_adjust_stack_configuration (&pnet_default_cfg);
   app_copy_ip_to_struct (&pnet_default_cfg.if_cfg.ip_cfg.ip_addr, ip);
   app_copy_ip_to_struct (&pnet_default_cfg.if_cfg.ip_cfg.ip_gateway, gateway);
   app_copy_ip_to_struct (&pnet_default_cfg.if_cfg.ip_cfg.ip_mask, netmask);
   strcpy (pnet_default_cfg.station_name, gp_appdata->arguments.station_name);
   strcpy (
      pnet_default_cfg.file_directory,
      appdata.arguments.path_storage_directory);

   strncpy (
      pnet_default_cfg.if_cfg.main_port.if_name,
      appdata.arguments.eth_interface,
      PNET_INTERFACE_NAME_MAX_SIZE);
   memcpy (
      pnet_default_cfg.if_cfg.main_port.eth_addr.addr,
      macbuffer.addr,
      sizeof (pnet_ethaddr_t));

   strncpy (
      pnet_default_cfg.if_cfg.ports[0].phy_port.if_name,
      appdata.arguments.eth_interface,
      PNET_INTERFACE_NAME_MAX_SIZE);
   memcpy (
      pnet_default_cfg.if_cfg.ports[0].phy_port.eth_addr.addr,
      macbuffer.addr,
      sizeof (pnet_ethaddr_t));

   pnet_default_cfg.cb_arg = (void *)gp_appdata;

   /* Initialize Profinet stack */
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
