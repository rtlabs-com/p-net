/*
-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

p-net Profinet device stack
===========================

Copyright 2018 rt-labs AB

This instance of p-net is licensed for use in a specific product, as
follows:

Company: Bender GmbH & Co. KG
Product: COMTRAXX
Product description: COMTRAXX products provided by Bender GmbH & Co. KG

Use of this instance of p-net in other products is not allowed.

You are hereby given a non-exclusive, non-transferable license to use
the SOFTWARE and documentation according to the SOFTWARE LICENSE
AGREEMENT document shipped together with this instance of the
SOFTWARE. The SOFTWARE LICENSE AGREEMENT prohibits copying and
redistribution of the SOFTWARE.  It prohibits reverse-engineering
activities.  It limits warranty and liability on behalf of rt-labs.
The details of these terms are explained in subsequent chapters of the
SOFTWARE LICENSE AGREEMENT.
-----BEGIN PGP SIGNATURE-----

iQEzBAEBCgAdFiEE1e4U+gPgGJXLsxxLZlG0bopmiXIFAl/TMvQACgkQZlG0bopm
iXJE1QgAlEKblKkIrLF+hVRm6I1IfbGV6VWnBx+9IPwaHZmgQZAVTOlp4RHiNAre
Il1l1xheENWpuihbVlvz5hMCVto4fkT13IUXxOo9Va9MK8lm6JOu2lYpl1HgVIwV
R6I6dsWLeNBSBaA38DDBHIfjP4LBZC/HVYO2ISc3/OKutqasz/WIp+AjAURiKRsf
yaWGay88FITHymmqJpGaWzB+jAOvRFs8hW/GsOyX/r17AK5F5N51O+UqUTKAWNPT
f6M8QF0b07Hw+eOuB7A/0D5nvSl/rOd9LvYkiUhIUmQdHqERNkF9EJdroIjxTMXi
lmoOkPEqyGV58fh7JjMYlI60sPlGBw==
=TaN+
-----END PGP SIGNATURE-----
*/

#include "pnal.h"
#include "osal_log.h"

#include <lwip/netif.h>
#include <drivers/dev.h>

#define MAX_NUMBER_OF_IF 1
#define NET_DRIVER_NAME  "/net"

static struct netif * interface[MAX_NUMBER_OF_IF];
static int nic_index = 0;

pnal_eth_handle_t * pnal_eth_init (
   const char * if_name,
   pnal_eth_callback_t * callback,
   void * arg)
{
   pnal_eth_handle_t * handle;
   struct netif * found_if;
   drv_t * drv;

   handle = malloc (sizeof (pnal_eth_handle_t));
   UASSERT (handle != NULL, EMEM);

   handle->arg = arg;
   handle->callback = callback;
   handle->if_id = -1;

   if (nic_index < MAX_NUMBER_OF_IF)
   {
      drv = dev_find_driver (NET_DRIVER_NAME);
      if (drv != NULL)
      {
         eth_ioctl (drv, arg, IOCTL_NET_SET_RX_HOOK, callback);

         found_if = netif_find (if_name);
         if (found_if == NULL)
         {
            interface[nic_index] = netif_default;
         }
         else
         {
            interface[nic_index] = found_if;
         }
         handle->if_id = nic_index++;
      }
      else
      {
         os_log (LOG_LEVEL_ERROR, "Driver \"%s\" not found!\n", NET_DRIVER_NAME);
      }
   }

   if (handle->if_id > -1)
   {
      return handle;
   }
   else
   {
      free (handle);
      return NULL;
   }
}

int pnal_eth_send (pnal_eth_handle_t * handle, pnal_buf_t * buf)
{
   int ret = -1;

   if (buf != NULL)
   {
      /* TODO: remove tot_len from os_buff */
      buf->tot_len = buf->len;

      if (interface[handle->if_id]->linkoutput != NULL)
      {
         interface[handle->if_id]->linkoutput (interface[handle->if_id], buf);
         ret = buf->len;
      }
   }
   return ret;
}
