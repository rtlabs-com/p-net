/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2021 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#include "pnal.h"

#include "options.h"
#include "osal.h"
#include "osal_log.h"

#include <fatfs.h>
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
   netif_set_addr (netif_default, &ip_addr, &ip_mask, &ip_gw);

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
   return 0;
}

int pnal_eth_get_status (const char * interface_name, pnal_eth_status_t * status)
{
   status->is_autonegotiation_supported = false;
   status->is_autonegotiation_enabled = false;
   status->autonegotiation_advertised_capabilities = 0;

   status->operational_mau_type = 0;
   status->running = true;

   return 0;
}

int pnal_save_file (
   const char * fullpath,
   const void * object_1,
   size_t size_1,
   const void * object_2,
   size_t size_2)
{
   FIL fil;
   FRESULT fres;
   UINT count;
   int ret = 0; /* Assume everything goes well */

   if (!SDFatFSMounted)
   {
      LOG_ERROR (
         PF_PNAL_LOG,
         "PNAL(%d): SD-Card not mounted (%s)\n",
         __LINE__,
         fullpath);
      return -1;
   }

   fres = f_open (&fil, fullpath, FA_WRITE | FA_OPEN_ALWAYS | FA_CREATE_ALWAYS);
   if (fres != FR_OK)
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
      fres = f_write (&fil, object_1, size_1, &count);
      if (fres != FR_OK || count != size_1)
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
      fres = f_write (&fil, object_2, size_2, &count);
      if (fres != FR_OK || count != size_2)
      {
         ret = -1;
         LOG_ERROR (
            PF_PNAL_LOG,
            "PNAL(%d): Failed to write file %s (second buffer)\n",
            __LINE__,
            fullpath);
      }
   }

   f_close (&fil);
   return ret;
}

void pnal_clear_file (const char * fullpath)
{
   if (!SDFatFSMounted)
   {
      LOG_ERROR (
         PF_PNAL_LOG,
         "PNAL(%d): SD-Card not mounted (%s)\n",
         __LINE__,
         fullpath);
      return;
   }
   LOG_DEBUG (PF_PNAL_LOG, "PNAL(%d): Clearing file %s\n", __LINE__, fullpath);
   f_unlink (fullpath);
}

int pnal_load_file (
   const char * fullpath,
   void * object_1,
   size_t size_1,
   void * object_2,
   size_t size_2)
{
   FIL fil;
   FRESULT fres;
   UINT count;
   int ret = 0; /* Assume everything goes well */

   if (!SDFatFSMounted)
   {
      LOG_ERROR (
         PF_PNAL_LOG,
         "PNAL(%d): SD-Card not mounted (%s)\n",
         __LINE__,
         fullpath);
      return -1;
   }

   fres = f_open (&fil, fullpath, FA_READ);
   if (fres != FR_OK)
   {
      LOG_ERROR (
         PF_PNAL_LOG,
         "PNAL(%d): Could not yet open file %s\n",
         __LINE__,
         fullpath);
      return -1;
   }

   /* Write file contents */
   if (size_1 > 0)
   {
      fres = f_read (&fil, object_1, size_1, &count);
      if (fres != FR_OK || count != size_1)
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
      fres = f_read (&fil, object_2, size_2, &count);
      if (fres != FR_OK || count != size_2)
      {
         ret = -1;
         LOG_ERROR (
            PF_PNAL_LOG,
            "PNAL(%d): Failed to read file %s (second buffer)\n",
            __LINE__,
            fullpath);
      }
   }

   f_close (&fil);
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
   CC_ASSERT (pbuf_free (p) == 1);
}

uint8_t pnal_buf_header (pnal_buf_t * p, int16_t header_size_increment)
{
   return pbuf_header (p, header_size_increment);
}
