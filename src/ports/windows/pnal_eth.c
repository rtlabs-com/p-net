/*********************************************************************
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#include "pnal.h"
#include <winsock.h>

static int init_winsock()
{
   static int is_initialized = 0;
   int iResult;
   WSADATA wsaData;

   if (!is_initialized)
   {
      // Initialize Winsock
      iResult = WSAStartup (MAKEWORD (2, 2), &wsaData);
      if (iResult != 0)
      {
         printf ("WSAStartup failed: %d\n", iResult);
         return 1;
      }
      is_initialized = 1;
   }
   return 0;
}

pnal_eth_handle_t * pnal_eth_init (const char * if_name, pnal_eth_callback_t * callback, void * arg)
{
   init_winsock();
   return 0;
}

int pnal_eth_send (pnal_eth_handle_t * handle, pnal_buf_t * buf)
{
   return 0;
}
