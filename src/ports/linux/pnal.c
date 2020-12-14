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

#include "osal.h"
#include "options.h"
#include "osal_log.h"
#include "pnal_filetools.h"

#include <arpa/inet.h>
#include <net/if.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int pnal_save_file (
   const char * fullpath,
   const void * object_1,
   size_t size_1,
   const void * object_2,
   size_t size_2)
{
   int ret = 0; /* Assume everything goes well */
   FILE * outputfile;

   /* Open file */
   outputfile = fopen (fullpath, "wb");
   if (outputfile == NULL)
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
      if (fwrite (object_1, size_1, 1, outputfile) != 1)
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
      if (fwrite (object_2, size_2, 1, outputfile) != 1)
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
   fclose (outputfile);

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
   FILE * inputfile;

   /* Open file */
   inputfile = fopen (fullpath, "rb");
   if (inputfile == NULL)
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
      if (fread (object_1, size_1, 1, inputfile) != 1)
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
      if (fread (object_2, size_2, 1, inputfile) != 1)
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
   fclose (inputfile);

   return ret;
}

uint32_t pnal_get_system_uptime_10ms (void)
{
   uint32_t uptime;

   /* TODO: Get sysUptime from SNMP MIB-II */
   uptime = 0;
   return uptime;
}

int pnal_get_interface_index (pnal_eth_handle_t * handle)
{
   int index;

   /* TODO: Get ifIndex from ifTable in SNMP MIB-II */
   index = 0;
   return index;
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

/** @internal
 * Convert IPv4 address to string
 * @param ip               In: IP address
 * @param outputstring     Out: Resulting string. Should have length
 * PNAL_INET_ADDRSTRLEN.
 */
static void os_ip_to_string (pnal_ipaddr_t ip, char * outputstring)
{
   snprintf (
      outputstring,
      PNAL_INET_ADDRSTRLEN,
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
   char ip_string[PNAL_INET_ADDRSTRLEN];
   char netmask_string[PNAL_INET_ADDRSTRLEN];
   char gateway_string[PNAL_INET_ADDRSTRLEN];
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
   ioctl (fd, SIOCGIFADDR, &ifr);
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
   ioctl (fd, SIOCGIFNETMASK, &ifr);
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
   gateway = (ip & 0xFFFFFF00) | 0x00000001;

   return gateway;
}

int pnal_get_hostname (char * hostname)
{
   int ret = -1;

   ret = gethostname (hostname, PNAL_HOST_NAME_MAX);
   hostname[PNAL_HOST_NAME_MAX - 1] = '\0';

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
