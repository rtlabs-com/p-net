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
#include <lwip/netif.h>
#include <lwip/snmp.h>
#include <lwip/sys.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int os_set_ip_suite (
   const char * interface_name,
   const os_ipaddr_t * p_ipaddr,
   const os_ipaddr_t * p_netmask,
   const os_ipaddr_t * p_gw,
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

int os_get_macaddress (const char * interface_name, os_ethaddr_t * mac_addr)
{
   memcpy (mac_addr, netif_default->hwaddr, sizeof (os_ethaddr_t));
   return 0;
}

os_ipaddr_t os_get_ip_address (const char * interface_name)
{
   return htonl (netif_default->ip_addr.addr);
}

os_ipaddr_t os_get_netmask (const char * interface_name)
{
   return htonl (netif_default->netmask.addr);
}

os_ipaddr_t os_get_gateway (const char * interface_name)
{
   /* TODO Read the actual default gateway */

   os_ipaddr_t ip;
   os_ipaddr_t gateway;

   ip = os_get_ip_address (interface_name);
   gateway = (ip & 0xFFFFFF00) | 0x00000001;

   return gateway;
}

int os_get_hostname (char * hostname)
{
   strcpy (hostname, netif_default->hostname);
   return 0;
}

int os_get_ip_suite (
   const char * interface_name,
   os_ipaddr_t * p_ipaddr,
   os_ipaddr_t * p_netmask,
   os_ipaddr_t * p_gw,
   char * hostname)
{
   int ret = -1;

   *p_ipaddr = os_get_ip_address (interface_name);
   *p_netmask = os_get_netmask (interface_name);
   *p_gw = os_get_gateway (interface_name);
   ret = os_get_hostname (hostname);

   return ret;
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

int os_save_file (
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
      return -1;
   }

   /* Write file contents */
   os_log (LOG_LEVEL_DEBUG, "Saving to file %s\n", fullpath);
   if (size_1 > 0)
   {
      if (write (outputfile, object_1, size_1) != (int)size_1)
      {
         ret = -1;
         os_log (LOG_LEVEL_ERROR, "Failed to write file %s\n", fullpath);
      }
   }
   if (size_2 > 0 && ret == 0)
   {
      if (write (outputfile, object_2, size_2) != (int)size_2)
      {
         ret = -1;
         os_log (LOG_LEVEL_ERROR, "Failed to write file %s\n", fullpath);
      }
   }

   /* Close file */
   close (outputfile);
   return ret;
}

void os_clear_file (const char * fullpath)
{
   os_log (LOG_LEVEL_DEBUG, "Clearing file %s\n", fullpath);
   (void)remove (fullpath);
}

int os_load_file (
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
      os_log (LOG_LEVEL_DEBUG, "Could not yet open file %s\n", fullpath);
      return -1;
   }

   /* Read file contents */
   if (size_1 > 0)
   {
      if (read (inputfile, object_1, size_1) != (int)size_1)
      {
         ret = -1;
         os_log (LOG_LEVEL_ERROR, "Failed to read file %s\n", fullpath);
      }
   }
   if (size_2 > 0 && ret == 0)
   {
      if (read (inputfile, object_2, size_2) != (int)size_2)
      {
         ret = -1;
         os_log (LOG_LEVEL_ERROR, "Failed to write file %s\n", fullpath);
      }
   }

   /* Close file */
   close (inputfile);

   return ret;
}

uint32_t os_get_system_uptime_10ms (void)
{
   uint32_t uptime = 0;

   MIB2_COPY_SYSUPTIME_TO (&uptime);
   return uptime;
}

os_buf_t * os_buf_alloc (uint16_t length)
{
   return pbuf_alloc (PBUF_RAW, length, PBUF_POOL);
}

void os_buf_free (os_buf_t * p)
{
   ASSERT (pbuf_free (p) == 1);
}

uint8_t os_buf_header (os_buf_t * p, int16_t header_size_increment)
{
   return pbuf_header (p, header_size_increment);
}
