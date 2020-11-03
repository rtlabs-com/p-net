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

int os_save_file (
   const char * fullpath,
   void * object_1,
   size_t size_1,
   void * object_2,
   size_t size_2)
{
   int ret = 0; /* Assume everything goes well */
   FILE * outputfile;

   /* Open file */
   outputfile = fopen (fullpath, "wb");
   if (outputfile == NULL)
   {
      os_log (LOG_LEVEL_ERROR, "Could not open file %s\n", fullpath);
      return -1;
   }

   /* Write file contents */
   os_log (LOG_LEVEL_DEBUG, "Saving to file %s\n", fullpath);
   if (size_1 > 0)
   {
      if (fwrite (object_1, size_1, 1, outputfile) != 1)
      {
         ret = -1;
         os_log (LOG_LEVEL_ERROR, "Failed to write file %s\n", fullpath);
      }
   }
   if (size_2 > 0 && ret == 0)
   {
      if (fwrite (object_2, size_2, 1, outputfile) != 1)
      {
         ret = -1;
         os_log (
            LOG_LEVEL_ERROR,
            "Failed to write file %s\n (second buffer)",
            fullpath);
      }
   }

   /* Close file */
   fclose (outputfile);

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
   int ret = 0; /* Assume everything goes well */
   FILE * inputfile;

   /* Open file */
   inputfile = fopen (fullpath, "rb");
   if (inputfile == NULL)
   {
      os_log (LOG_LEVEL_DEBUG, "Could not yet open file %s\n", fullpath);
      return -1;
   }

   /* Read file contents */
   if (size_1 > 0)
   {
      if (fread (object_1, size_1, 1, inputfile) != 1)
      {
         ret = -1;
         os_log (LOG_LEVEL_ERROR, "Failed to read file %s\n", fullpath);
      }
   }

   if (size_2 > 0 && ret == 0)
   {
      if (fread (object_2, size_2, 1, inputfile) != 1)
      {
         ret = -1;
         os_log (
            LOG_LEVEL_ERROR,
            "Failed to read file %s\n (second buffer)",
            fullpath);
      }
   }

   /* Close file */
   fclose (inputfile);

   return ret;
}

uint32_t os_get_system_uptime_10ms (void)
{
   uint32_t uptime;

   /* TODO: Implement this */
   uptime = 0;
   return uptime;
}

uint32_t os_buf_alloc_cnt = 0; /* Count outstanding buffers */

os_buf_t * os_buf_alloc (uint16_t length)
{
   os_buf_t * p = malloc (sizeof (os_buf_t) + length);

   if (p != NULL)
   {
#if 1
      p->payload = (void *)((uint8_t *)p + sizeof (os_buf_t)); /* Payload
                                                                  follows header
                                                                  struct */
      p->len = length;
#endif
      os_buf_alloc_cnt++;
   }
   else
   {
      assert ("malloc() failed\n");
   }

   return p;
}

void os_buf_free (os_buf_t * p)
{
   free (p);
   os_buf_alloc_cnt--;
   return;
}

uint8_t os_buf_header (os_buf_t * p, int16_t header_size_increment)
{
   return 255;
}

/** @internal
 * Convert IPv4 address to string
 * @param ip               In: IP address
 * @param outputstring     Out: Resulting string. Should have length
 * OS_INET_ADDRSTRLEN.
 */
static void os_ip_to_string (os_ipaddr_t ip, char * outputstring)
{
   snprintf (
      outputstring,
      OS_INET_ADDRSTRLEN,
      "%u.%u.%u.%u",
      (uint8_t) ((ip >> 24) & 0xFF),
      (uint8_t) ((ip >> 16) & 0xFF),
      (uint8_t) ((ip >> 8) & 0xFF),
      (uint8_t) (ip & 0xFF));
}

int os_set_ip_suite (
   const char * interface_name,
   os_ipaddr_t * p_ipaddr,
   os_ipaddr_t * p_netmask,
   os_ipaddr_t * p_gw,
   const char * hostname,
   bool permanent)
{
   char ip_string[OS_INET_ADDRSTRLEN];
   char netmask_string[OS_INET_ADDRSTRLEN];
   char gateway_string[OS_INET_ADDRSTRLEN];
   char * permanent_string;
   char * outputcommand;
   int textlen = -1;
   int status = -1;

   os_ip_to_string (*p_ipaddr, ip_string);
   os_ip_to_string (*p_netmask, netmask_string);
   os_ip_to_string (*p_gw, gateway_string);
   permanent_string = permanent ? "1" : "0";

   textlen = asprintf (
      &outputcommand,
      "./set_network_parameters %s %s %s %s '%s' %s",
      interface_name,
      ip_string,
      netmask_string,
      gateway_string,
      hostname,
      permanent_string);
   if (textlen < 0)
   {
      return -1;
   }

   os_log (
      LOG_LEVEL_DEBUG,
      "Command for setting network parameters: %s\n",
      outputcommand);

   status = system (outputcommand);
   free (outputcommand);
   if (status != 0)
   {
      return -1;
   }
   return 0;
}

int os_get_macaddress (const char * interface_name, os_ethaddr_t * mac_addr)
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

os_ipaddr_t os_get_ip_address (const char * interface_name)
{
   int fd;
   struct ifreq ifr;
   os_ipaddr_t ip;

   fd = socket (AF_INET, SOCK_DGRAM, 0);
   ifr.ifr_addr.sa_family = AF_INET;
   strncpy (ifr.ifr_name, interface_name, IFNAMSIZ - 1);
   ioctl (fd, SIOCGIFADDR, &ifr);
   ip = ntohl (((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr);
   close (fd);

   return ip;
}

os_ipaddr_t os_get_netmask (const char * interface_name)
{
   int fd;
   struct ifreq ifr;
   os_ipaddr_t netmask;

   fd = socket (AF_INET, SOCK_DGRAM, 0);

   ifr.ifr_addr.sa_family = AF_INET;
   strncpy (ifr.ifr_name, interface_name, IFNAMSIZ - 1);
   ioctl (fd, SIOCGIFNETMASK, &ifr);
   netmask = ntohl (((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr);
   close (fd);

   return netmask;
}

os_ipaddr_t os_get_gateway (const char * interface_name)
{
   /* TODO Read the actual default gateway (somewhat complicated) */

   os_ipaddr_t ip;
   os_ipaddr_t gateway;

   ip = os_get_ip_address (interface_name);
   gateway = (ip & 0xFFFFFF00) | 0x00000001;

   return gateway;
}

int os_get_hostname (char * hostname)
{
   int ret = -1;

   ret = gethostname (hostname, OS_HOST_NAME_MAX);
   hostname[OS_HOST_NAME_MAX - 1] = '\0';

   return ret;
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
