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

#include "pnal.h"

#include "osal.h"
#include "osal_log.h"
#include "options.h"

#include <drivers/net.h>
#include <gpio.h>
#include <lwip/apps/snmp.h>
#include <lwip/netif.h>
#include <lwip/snmp.h>
#include <lwip/sys.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int pnal_set_ip_suite (
   const char * interface_name,
   const pnal_ipaddr_t * p_ipaddr,
   const pnal_ipaddr_t * p_netmask,
   const pnal_ipaddr_t * p_gw,
   const char * hostname,
   bool permanent)
{
   ip_addr_t ip_addr;
   ip_addr_t ip_mask;
   ip_addr_t ip_gw;

   ip_addr.addr = htonl (*p_ipaddr);
   ip_mask.addr = htonl (*p_netmask);
   ip_gw.addr = htonl (*p_gw);
   net_configure (NULL, &ip_addr, &ip_mask, &ip_gw, NULL, hostname);

   return 0;
}

int pnal_get_macaddress (const char * interface_name, pnal_ethaddr_t * mac_addr)
{
   memcpy (mac_addr, netif_default->hwaddr, sizeof (pnal_ethaddr_t));
   return 0;
}

pnal_ipaddr_t pnal_get_ip_address (const char * interface_name)
{
   return htonl (netif_default->ip_addr.addr);
}

pnal_ipaddr_t pnal_get_netmask (const char * interface_name)
{
   return htonl (netif_default->netmask.addr);
}

pnal_ipaddr_t pnal_get_gateway (const char * interface_name)
{
   /* TODO Read the actual default gateway */

   pnal_ipaddr_t ip;
   pnal_ipaddr_t gateway;

   ip = pnal_get_ip_address (interface_name);
   gateway = (ip & 0xFFFFFF00) | 0x00000001;

   return gateway;
}

int pnal_get_hostname (char * hostname)
{
   strcpy (hostname, netif_default->hostname);
   return 0;
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

int pnal_get_port_statistics (
   const char * interface_name,
   pnal_port_stats_t * port_stats)
{
   port_stats->if_in_octets = netif_default->mib2_counters.ifinoctets;
   port_stats->if_in_errors = netif_default->mib2_counters.ifinerrors;
   port_stats->if_in_discards = netif_default->mib2_counters.ifindiscards;
   port_stats->if_out_octets = netif_default->mib2_counters.ifoutoctets;
   port_stats->if_out_errors = netif_default->mib2_counters.ifouterrors;
   port_stats->if_out_discards = netif_default->mib2_counters.ifoutdiscards;

   return 0;
}

int pnal_get_interface_index (const char * interface_name)
{
   u8_t index;
   struct netif * netif = netif_find (interface_name);

   if (netif == NULL)
   {
      return 0;
   }

   index = netif_to_num (netif);

   return index;
}

int pnal_eth_get_status (const char * interface_name, pnal_eth_status_t * status)
{
   /* TODO: Read current status */
   status->is_autonegotiation_supported = true;
   status->is_autonegotiation_enabled = true;
   status->autonegotiation_advertised_capabilities =
      PNAL_ETH_AUTONEG_CAP_100BaseTX_HALF_DUPLEX |
      PNAL_ETH_AUTONEG_CAP_100BaseTX_FULL_DUPLEX;
   status->operational_mau_type = PNAL_ETH_MAU_COPPER_100BaseTX_FULL_DUPLEX;

   return 0;
}

int os_snprintf (char * str, size_t size, const char * fmt, ...)
{
   int ret;
   va_list list;

   va_start (list, fmt);
   ret = snprintf (str, size, fmt, list);
   va_end (list);
   return ret;
}

int pnal_save_file (
   const char * fullpath,
   const void * object_1,
   size_t size_1,
   const void * object_2,
   size_t size_2)
{
   int outputfile;
   int ret = 0; /* Assume everything goes well */

   /* Open file */
   outputfile = open (fullpath, O_WRONLY | O_CREAT);
   if (outputfile < 0)
   {
      LOG_ERROR (
         PF_PNAL_LOG,
         "PNAL(%d): Could not open file %s\n",
         __LINE__,
         fullpath);
      return -1;
   }

   /* Write file contents */
   LOG_DEBUG (PF_PNAL_LOG, "PNAL(%d): Saving to file %s\n", __LINE__, fullpath);
   if (size_1 > 0)
   {
      if (write (outputfile, object_1, size_1) != (int)size_1)
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
      if (write (outputfile, object_2, size_2) != (int)size_2)
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
   close (outputfile);
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
   int inputfile;
   int ret = 0; /* Assume everything goes well */

   /* Open file */
   inputfile = open (fullpath, O_RDONLY);
   if (inputfile < 0)
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
      if (read (inputfile, object_1, size_1) != (int)size_1)
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
      if (read (inputfile, object_2, size_2) != (int)size_2)
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
   close (inputfile);

   return ret;
}

uint32_t pnal_get_system_uptime_10ms (void)
{
   uint32_t uptime = 0;

   MIB2_COPY_SYSUPTIME_TO (&uptime);
   return uptime;
}

pnal_buf_t * pnal_buf_alloc (uint16_t length)
{
   return pbuf_alloc (PBUF_RAW, length, PBUF_POOL);
}

void pnal_buf_free (pnal_buf_t * p)
{
   ASSERT (pbuf_free (p) == 1);
}

uint8_t pnal_buf_header (pnal_buf_t * p, int16_t header_size_increment)
{
   return pbuf_header (p, header_size_increment);
}
