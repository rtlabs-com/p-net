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

#include "pnal.h"

#include "options.h"
#include "osal.h"
#include "osal_log.h"
#include "pnal_filetools.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/ethtool.h>
#include <linux/if_link.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/********************************* Files *************************************/

int pnal_save_file (
   const char * fullpath,
   const void * object_1,
   size_t size_1,
   const void * object_2,
   size_t size_2)
{
   int ret = 0; /* Assume everything goes well */
   int outputfile;

   /* Open file
      Use synchronized write to make sure it ends up on disk immediately */
   outputfile = open (
      fullpath,
      O_WRONLY | O_CREAT | O_TRUNC | O_SYNC,
      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
   if (outputfile == -1)
   {
      LOG_ERROR (
         PF_PNAL_LOG,
         "PNAL(%d): Could not open file %s\n",
         __LINE__,
         fullpath);
      return -1;
   }

   /* Write file contents */
   if (size_1 > 0)
   {
      if (write (outputfile, object_1, size_1) == -1)
      {
         ret = -1;
         LOG_ERROR (
            PF_PNAL_LOG,
            "PNAL(%d): Failed to write file %s\n",
            __LINE__,
            fullpath);
      }
   }
   if (size_2 > 0 && ret == 0)
   {
      if (write (outputfile, object_2, size_2) == -1)
      {
         ret = -1;
         LOG_ERROR (
            PF_PNAL_LOG,
            "PNAL(%d): Failed to write file %s (second buffer)\n",
            __LINE__,
            fullpath);
      }
   }

   /* Close file */
   if (close (outputfile) != 0)
   {
      ret = -1;
   }

   return ret;
}

void pnal_clear_file (const char * fullpath)
{
   LOG_DEBUG (PF_PNAL_LOG, "PNAL(%d): Clearing file %s\n", __LINE__, fullpath);
   (void)remove (fullpath);
}

int pnal_load_file (
   const char * fullpath,
   void * object_1,
   size_t size_1,
   void * object_2,
   size_t size_2)
{
   int ret = 0; /* Assume everything goes well */
   int inputfile;

   /* Open file */
   inputfile = open (fullpath, O_RDONLY);
   if (inputfile == -1)
   {
      LOG_DEBUG (
         PF_PNAL_LOG,
         "PNAL(%d): Could not yet open file %s\n",
         __LINE__,
         fullpath);
      return -1;
   }

   /* Read file contents */
   if (size_1 > 0)
   {
      if (read (inputfile, object_1, size_1) <= 0)
      {
         ret = -1;
         LOG_ERROR (
            PF_PNAL_LOG,
            "PNAL(%d): Failed to read file %s\n",
            __LINE__,
            fullpath);
      }
   }

   if (size_2 > 0 && ret == 0)
   {
      if (read (inputfile, object_2, size_2) <= 0)
      {
         ret = -1;
         LOG_ERROR (
            PF_PNAL_LOG,
            "PNAL(%d): Failed to read file %s (second buffer)\n",
            __LINE__,
            fullpath);
      }
   }

   /* Close file */
   if (close (inputfile) != 0)
   {
      ret = -1;
   }

   return ret;
}

/*****************************************************************************/

uint32_t pnal_get_system_uptime_10ms (void)
{
   struct sysinfo systeminfo; /* Field .uptime contains uptime in seconds */

   if (sysinfo (&systeminfo) != 0)
   {
      return 0;
   }

   return systeminfo.uptime * 100;
}

uint32_t pnal_buf_alloc_cnt = 0; /* Count outstanding buffers */

pnal_buf_t * pnal_buf_alloc (uint16_t length)
{
   pnal_buf_t * p = malloc (sizeof (pnal_buf_t) + length);

   if (p != NULL)
   {
#if 1
      p->payload = (void *)((uint8_t *)p + sizeof (pnal_buf_t)); /* Payload
                                                                  follows header
                                                                  struct */
      p->len = length;
#endif
      pnal_buf_alloc_cnt++;
   }
   else
   {
      assert ("malloc() failed\n");
   }

   return p;
}

void pnal_buf_free (pnal_buf_t * p)
{
   free (p);
   pnal_buf_alloc_cnt--;
   return;
}

uint8_t pnal_buf_header (pnal_buf_t * p, int16_t header_size_increment)
{
   return 255;
}

/************************** Networking ***************************************/

/** @internal
 * Convert IPv4 address to string
 * @param ip               In:    IP address
 * @param outputstring     Out:   Resulting string. Should have size
 *                                PNAL_INET_ADDRSTR_SIZE.
 */
static void os_ip_to_string (pnal_ipaddr_t ip, char * outputstring)
{
   snprintf (
      outputstring,
      PNAL_INET_ADDRSTR_SIZE,
      "%u.%u.%u.%u",
      (uint8_t) ((ip >> 24) & 0xFF),
      (uint8_t) ((ip >> 16) & 0xFF),
      (uint8_t) ((ip >> 8) & 0xFF),
      (uint8_t) (ip & 0xFF));
}

int pnal_set_ip_suite (
   const char * interface_name,
   const pnal_ipaddr_t * p_ipaddr,
   const pnal_ipaddr_t * p_netmask,
   const pnal_ipaddr_t * p_gw,
   const char * hostname,
   bool permanent)
{
   char ip_string[PNAL_INET_ADDRSTR_SIZE];      /** Terminated string */
   char netmask_string[PNAL_INET_ADDRSTR_SIZE]; /** Terminated string */
   char gateway_string[PNAL_INET_ADDRSTR_SIZE]; /** Terminated string */
   const char * argv[8];

   os_ip_to_string (*p_ipaddr, ip_string);
   os_ip_to_string (*p_netmask, netmask_string);
   os_ip_to_string (*p_gw, gateway_string);

   argv[0] = "set_network_parameters";
   argv[1] = interface_name;
   argv[2] = (char *)&ip_string;
   argv[3] = (char *)&netmask_string;
   argv[4] = (char *)&gateway_string;
   argv[5] = hostname;
   argv[6] = permanent ? "1" : "0";
   argv[7] = NULL;

   return pnal_execute_script (argv);
}

/**
 * Calculate MAU type.
 *
 * @param speed            In:    Speed in Mbit/s
 * @param duplex           In:    DUPLEX_FULL, DUPLEX_HALF or DUPLEX_UNKNOWN
 * @return  The MAU type
 */
static pnal_eth_mau_t calculate_mau_type (uint32_t speed, uint8_t duplex)
{
   /* TODO Differentiate between copper, fiber and radio.
           Better handling of edge cases. */

   if (duplex == DUPLEX_FULL)
   {
      switch (speed)
      {
      case SPEED_10:
         return PNAL_ETH_MAU_COPPER_10BaseT;
      case SPEED_100:
         return PNAL_ETH_MAU_COPPER_100BaseTX_FULL_DUPLEX;
      case SPEED_1000:
         return PNAL_ETH_MAU_COPPER_1000BaseT_FULL_DUPLEX;
      default:
         break;
      }
   }
   else if (duplex == DUPLEX_HALF)
   {
      switch (speed)
      {
      case SPEED_10:
         return PNAL_ETH_MAU_COPPER_10BaseT;
      case SPEED_100:
         return PNAL_ETH_MAU_COPPER_100BaseTX_HALF_DUPLEX;
      case SPEED_1000:
         return PNAL_ETH_MAU_COPPER_1000BaseT_HALF_DUPLEX;
      default:
         break;
      }
   }

   return PNAL_ETH_MAU_RADIO;
}

/**
 * Calculate advertised capabilites
 *
 * @param advertised       In:    Linux advertised capabilites as a bitfield
 * @return Profinet advertised capabilites as a bitfield
 */
static uint16_t calculate_capabilities (uint32_t advertised)
{
   uint16_t out = 0;

   if (advertised & ADVERTISED_10baseT_Half)
   {
      out |= PNAL_ETH_AUTONEG_CAP_10BaseT_HALF_DUPLEX;
   }
   if (advertised & ADVERTISED_10baseT_Full)
   {
      out |= PNAL_ETH_AUTONEG_CAP_10BaseT_FULL_DUPLEX;
   }
   if (advertised & ADVERTISED_100baseT_Half)
   {
      out |= PNAL_ETH_AUTONEG_CAP_100BaseTX_HALF_DUPLEX;
   }
   if (advertised & ADVERTISED_100baseT_Full)
   {
      out |= PNAL_ETH_AUTONEG_CAP_100BaseTX_FULL_DUPLEX;
   }
   if (advertised & ADVERTISED_1000baseT_Half)
   {
      out |= PNAL_ETH_AUTONEG_CAP_1000BaseT_HALF_DUPLEX;
   }
   if (advertised & ADVERTISED_1000baseT_Full)
   {
      out |= PNAL_ETH_AUTONEG_CAP_1000BaseT_FULL_DUPLEX;
   }

   if (out == 0)
   {
      out |= PNAL_ETH_AUTONEG_CAP_UNKNOWN;
   }

   return out;
}

int pnal_eth_get_status (const char * interface_name, pnal_eth_status_t * status)
{
   /* TODO: Possibly use the new ETHTOOL_GLINKSETTINGS instead of ETHTOOL_GSET
      See:
   https://stackoverflow.com/questions/41822920/how-to-get-ethtool-settings */

   int ret = -1;
   int control_socket;
   struct ifreq ifr;
   struct ethtool_cmd eth_status_linux;
   uint32_t speed = 0; /* Mbit/s */

   control_socket = socket (PF_INET, SOCK_DGRAM, IPPROTO_IP);
   if (control_socket < 0)
   {
      return ret;
   }

   snprintf (ifr.ifr_name, sizeof (ifr.ifr_name), "%s", interface_name);
   ifr.ifr_data = (char *)&eth_status_linux;
   eth_status_linux.cmd = ETHTOOL_GSET;

   if (ioctl (control_socket, SIOCETHTOOL, &ifr) >= 0)
   {
      speed = ethtool_cmd_speed (&eth_status_linux);

      status->is_autonegotiation_enabled =
         (eth_status_linux.autoneg == AUTONEG_ENABLE);
      status->is_autonegotiation_supported = eth_status_linux.advertising &
                                             ADVERTISED_Autoneg;
      status->operational_mau_type =
         calculate_mau_type (speed, eth_status_linux.duplex);
      status->autonegotiation_advertised_capabilities =
         calculate_capabilities (eth_status_linux.advertising);

      ret = 0;
   }

   if (ioctl (control_socket, SIOCGIFFLAGS, &ifr) >= 0)
   {
      status->running = (ifr.ifr_flags & IFF_RUNNING) ? true : false;
   }

   close (control_socket);

   return ret;
}

int pnal_get_interface_index (const char * interface_name)
{
   return if_nametoindex (interface_name);
}

int pnal_get_port_statistics (
   const char * interface_name,
   pnal_port_stats_t * port_stats)
{
   struct ifaddrs *p_ifaddr, *p_temp;
   struct rtnl_link_stats * p_statistics_linux;
   int ret = -1;

   if (getifaddrs (&p_ifaddr) != 0)
   {
      return ret;
   }

   /* Walk the linked list and look for correct interface name */
   for (p_temp = p_ifaddr; p_temp != NULL; p_temp = p_temp->ifa_next)
   {
      if (
         (p_temp->ifa_addr != NULL) &&
         (p_temp->ifa_addr->sa_family == AF_PACKET) &&
         (strcmp (p_temp->ifa_name, interface_name) == 0) &&
         (p_temp->ifa_data != NULL))
      {
         p_statistics_linux = (struct rtnl_link_stats *)p_temp->ifa_data;

         port_stats->if_in_octets = p_statistics_linux->rx_bytes;
         port_stats->if_in_errors = p_statistics_linux->rx_errors;
         port_stats->if_in_discards = p_statistics_linux->rx_dropped;
         port_stats->if_out_octets = p_statistics_linux->tx_bytes;
         port_stats->if_out_errors = p_statistics_linux->tx_errors;
         port_stats->if_out_discards = p_statistics_linux->tx_dropped;

         ret = 0;
         break;
      }
   }
   freeifaddrs (p_ifaddr);

   return ret;
}

int pnal_get_macaddress (const char * interface_name, pnal_ethaddr_t * mac_addr)
{
   int fd;
   int ret = 0;
   struct ifreq ifr;

   fd = socket (AF_INET, SOCK_DGRAM, 0);

   ifr.ifr_addr.sa_family = AF_INET;
   strncpy (ifr.ifr_name, interface_name, IFNAMSIZ - 1);

   ret = ioctl (fd, SIOCGIFHWADDR, &ifr);
   if (ret == 0)
   {
      memcpy (mac_addr->addr, ifr.ifr_hwaddr.sa_data, 6);
   }
   close (fd);
   return ret;
}

pnal_ipaddr_t pnal_get_ip_address (const char * interface_name)
{
   int fd;
   struct ifreq ifr;
   pnal_ipaddr_t ip;

   fd = socket (AF_INET, SOCK_DGRAM, 0);
   ifr.ifr_addr.sa_family = AF_INET;
   strncpy (ifr.ifr_name, interface_name, IFNAMSIZ - 1);

   if (ioctl (fd, SIOCGIFADDR, &ifr) != 0)
   {
      close (fd);
      return PNAL_IPADDR_INVALID;
   }

   ip = ntohl (((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr);
   close (fd);

   return ip;
}

pnal_ipaddr_t pnal_get_netmask (const char * interface_name)
{
   int fd;
   struct ifreq ifr;
   pnal_ipaddr_t netmask;

   fd = socket (AF_INET, SOCK_DGRAM, 0);

   ifr.ifr_addr.sa_family = AF_INET;
   strncpy (ifr.ifr_name, interface_name, IFNAMSIZ - 1);

   if (ioctl (fd, SIOCGIFNETMASK, &ifr) != 0)
   {
      close (fd);
      return PNAL_IPADDR_INVALID;
   }

   netmask = ntohl (((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr);
   close (fd);

   return netmask;
}

pnal_ipaddr_t pnal_get_gateway (const char * interface_name)
{
   /* TODO Read the actual default gateway (somewhat complicated) */

   pnal_ipaddr_t ip;
   pnal_ipaddr_t gateway;

   ip = pnal_get_ip_address (interface_name);
   if (ip == PNAL_IPADDR_INVALID)
   {
      return PNAL_IPADDR_INVALID;
   }

   gateway = (ip & 0xFFFFFF00) | 0x00000001;

   return gateway;
}

int pnal_get_hostname (char * hostname)
{
   int ret = -1;

   ret = gethostname (hostname, PNAL_HOSTNAME_MAX_SIZE);
   hostname[PNAL_HOSTNAME_MAX_SIZE - 1] = '\0';

   return ret;
}

int pnal_get_ip_suite (
   const char * interface_name,
   pnal_ipaddr_t * p_ipaddr,
   pnal_ipaddr_t * p_netmask,
   pnal_ipaddr_t * p_gw,
   char * hostname)
{
   int ret = -1;

   *p_ipaddr = pnal_get_ip_address (interface_name);
   *p_netmask = pnal_get_netmask (interface_name);
   *p_gw = pnal_get_gateway (interface_name);
   ret = pnal_get_hostname (hostname);

   return ret;
}
