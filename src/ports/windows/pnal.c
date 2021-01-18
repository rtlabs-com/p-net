/*********************************************************************
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#include "pnal.h"

#include "osal.h"
#include "options.h"
#include "osal_log.h"

#include <stdio.h>
#include <string.h>

/********************************* Files *************************************/

int pnal_save_file (const char * fullpath,const void * object_1,size_t size_1,const void * object_2,size_t size_2)
{
   int ret = 0; /* Assume everything goes well */
   FILE * outputfile;

   /* Open file */
   outputfile = fopen (fullpath, "wb");
   if (outputfile == NULL)
   {
      LOG_ERROR (PF_PNAL_LOG, "PNAL(%d): Could not open file %s\n", __LINE__, fullpath);
      return -1;
   }

   /* Write file contents */
   LOG_DEBUG (PF_PNAL_LOG, "PNAL(%d): Saving to file %s\n", __LINE__, fullpath);
   if (size_1 > 0)
   {
      if (fwrite (object_1, size_1, 1, outputfile) != 1)
      {
         ret = -1;
         LOG_ERROR (PF_PNAL_LOG,"PNAL(%d): Failed to write file %s\n", __LINE__, fullpath);
      }
   }
   if (size_2 > 0 && ret == 0)
   {
      if (fwrite (object_2, size_2, 1, outputfile) != 1)
      {
         ret = -1;
         LOG_ERROR (PF_PNAL_LOG,"PNAL(%d): Failed to write file %s (second buffer)\n", __LINE__, fullpath);
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

int pnal_load_file (const char * fullpath,void * object_1,size_t size_1,void * object_2,size_t size_2)
{
   int ret = 0; /* Assume everything goes well */
   FILE * inputfile;

   /* Open file */
   inputfile = fopen (fullpath, "rb");
   if (inputfile == NULL)
   {
      LOG_DEBUG (PF_PNAL_LOG,"PNAL(%d): Could not yet open file %s\n", __LINE__, fullpath);
      return -1;
   }

   /* Read file contents */
   if (size_1 > 0)
   {
      if (fread (object_1, size_1, 1, inputfile) != 1)
      {
         ret = -1;
         LOG_ERROR (PF_PNAL_LOG, "PNAL(%d): Failed to read file %s\n", __LINE__, fullpath);
      }
   }

   if (size_2 > 0 && ret == 0)
   {
      if (fread (object_2, size_2, 1, inputfile) != 1)
      {
         ret = -1;
         LOG_ERROR (PF_PNAL_LOG, "PNAL(%d): Failed to read file %s (second buffer)\n", __LINE__, fullpath);
      }
   }

   /* Close file */
   fclose (inputfile);

   return ret;
}

/*****************************************************************************/

uint32_t pnal_get_system_uptime_10ms (void)
{
   uint32_t uptime;

   /* TODO: Get sysUptime from SNMP MIB-II */
   uptime = 0;
   return uptime;
}

pnal_buf_t * pnal_buf_alloc (uint16_t length)
{
   pnal_buf_t * p = malloc (sizeof (pnal_buf_t) + length);

   if (p != NULL)
   {
      p->payload = (void *)((uint8_t *)p + sizeof (pnal_buf_t)); /* Payload follows header struct */
      p->len = length;
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
   return;
}

uint8_t pnal_buf_header (pnal_buf_t * p, int16_t header_size_increment)
{
   return 255;
}

/************************** Networking ***************************************/

int pnal_set_ip_suite (const char * interface_name,const pnal_ipaddr_t * p_ipaddr,const pnal_ipaddr_t * p_netmask,const pnal_ipaddr_t * p_gw,const char * hostname,bool permanent)
{
   return 0;
}

int pnal_eth_get_status (const char * interface_name, pnal_eth_status_t * status)
{
   return 0;
}

int pnal_get_interface_index (const char * interface_name)
{
   return 0;
}

int pnal_get_port_statistics (const char * interface_name, pnal_port_stats_t * port_stats)
{
   memset (port_stats, 0, sizeof (pnal_port_stats_t));

   return 0;
}

int pnal_get_macaddress (const char * interface_name, pnal_ethaddr_t * mac_addr)
{
   int ret = 0;
   
   return ret;
}

pnal_ipaddr_t pnal_get_ip_address (const char * interface_name)
{
   return 0;
}

pnal_ipaddr_t pnal_get_netmask (const char * interface_name)
{
   return 0;
}

pnal_ipaddr_t pnal_get_gateway (const char * interface_name)
{
   return 0;
}

int pnal_get_hostname (char * hostname)
{
   return 0;
}

int pnal_get_ip_suite (const char * interface_name, pnal_ipaddr_t * p_ipaddr, pnal_ipaddr_t * p_netmask, pnal_ipaddr_t * p_gw, char * hostname)
{
   int ret = -1;

   *p_ipaddr = pnal_get_ip_address (interface_name);
   *p_netmask = pnal_get_netmask (interface_name);
   *p_gw = pnal_get_gateway (interface_name);
   ret = pnal_get_hostname (hostname);

   return ret;
}
