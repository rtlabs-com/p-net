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
#include "pcap_helper.h"

/**
 * @internal
 * Run a thread that listens to incoming raw Ethernet sockets.
 * Delegate the actual work to thread_arg->callback
 *
 * This is a function to be passed into os_thread_create()
 * Do not change the argument types.
 *
 * @param thread_arg     InOut: Will be converted to pnal_eth_handle_t
 */
static void os_eth_task (void * thread_arg)
{
   pnal_eth_handle_t * eth_handle = thread_arg;
   int readlen;
   int handled = 0;

   pnal_buf_t * p = pnal_buf_alloc (PNAL_BUF_MAX_SIZE);
   assert (p != NULL);

   while (1)
   {
      /* read */
      if ((readlen = pcap_helper_recieve_telegram (eth_handle->pcap_handle, p->payload, PNAL_BUF_MAX_SIZE)) < 0) continue;
      p->len = readlen;

      if (eth_handle->callback != NULL)
      {
         handled = eth_handle->callback (eth_handle->arg, p);
      }
      else
      {
         handled = 0; /* Message not handled */
      }

      if (handled == 1)
      {
         p = pnal_buf_alloc (PNAL_BUF_MAX_SIZE);
         assert (p != NULL);
      }
   }
}

pnal_eth_handle_t * pnal_eth_init (const char * if_name, pnal_eth_callback_t * callback, void * arg)
{
   pnal_eth_handle_t * handle;
   PCAP_HANDLE pcap;

   /* open pcap */
   if (pcap_helper_open (if_name, &pcap) < 0)
      return NULL;

   handle = malloc (sizeof (pnal_eth_handle_t));
   if (handle == NULL)
      return NULL;

   handle->arg = arg;
   handle->callback = callback;
   handle->pcap_handle = pcap;

   /* create eth thread */
   handle->thread =
      os_thread_create ("os_eth_task", 10, 4096, os_eth_task, handle);

   return handle;
}

int pnal_eth_send (pnal_eth_handle_t * handle, pnal_buf_t * buf)
{
   return pcap_helper_send_telegram (handle->pcap_handle, buf->payload, buf->len);
}
