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

#include <winsock2.h>
#include <errno.h>
#include "pcap_helper.h"
#include <pcap.h>
#include <iphlpapi.h> //GetAdaptersInfo
#include "options.h"
#include "osal_log.h"
#include <pnet_api.h>

// ***** Cache for eth adapter info *****
static char CurrentEthName[PNET_INTERFACE_NAME_MAX_SIZE] = "";
static unsigned int NetMask = 0;
static unsigned int Address = 0;
static unsigned int Gateway = 0;
static unsigned char Mac[6] = {0};

// ***** filter data ****
static struct bpf_program fpdat;

#define FILTER_STRING                                                          \
   "ether dst 08:00:06:60:4a:3f || ether multicast || ether broadcast || "     \
   "ether proto 0x8892 || ether proto 0x88cc"

static int populate_adapter_info (const char * eth_interface);

// * ---------------------------------------------------------
// * get / set
// * ---------------------------------------------------------
unsigned int pcap_helper_get_ip (const char * eth_interface)
{
   populate_adapter_info(eth_interface);
   return Address;
}
unsigned int pcap_helper_get_netmask (const char * eth_interface)
{
   populate_adapter_info (eth_interface);
   return NetMask;
}
unsigned int pcap_helper_get_gateway (const char * eth_interface)
{
   populate_adapter_info (eth_interface);
   return Gateway;
}
unsigned char * pcap_helper_get_mac (const char * eth_interface)
{
   populate_adapter_info (eth_interface);
   return Mac;
}

// * ---------------------------------------------------------
// * Will translate between eth name (eg. eth0) and pcap name (eg. \Device\NPF_{69F12DD8-CED6-46F8-B79F-6DBA0395E86A})
// * ---------------------------------------------------------
static int get_pcap_name (const char * eth_interface, char * pcap_name)
{
   pcap_if_t * alldevs;
   pcap_if_t * d1;
   char errbuf[256];
   u_int inum1, i = 0;

   /* get list */
   if (pcap_findalldevs (&alldevs, errbuf) == -1)
   {
      LOG_ERROR (
         PF_PNAL_LOG,
         "PNAL(%d): pcap_findalldevs_ex: %s\n",
         __LINE__,
         errbuf);
      return -EFAULT;
   }

   /* scan name */
   if (sscanf (eth_interface, "eth%u", &inum1) != 1)
      return -EFAULT;

   /* Jump to the selected adapter */
   for (d1 = alldevs, i = 0; i < inum1 - 1; d1 = d1->next, i++)
      ;

   strcpy (pcap_name, d1->name);

   /* Free the device list */
   pcap_freealldevs (alldevs);

   return 0;
}

// * ---------------------------------------------------------
// * Init function for winsock
// * ---------------------------------------------------------
int pcap_helper_init (void)
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
         LOG_ERROR (
            PF_PNAL_LOG,
            "PNAL(%d): WSAStartup failed: %d\n",
            __LINE__,
            iResult);
         return -EFAULT;
      }
      is_initialized = 1;
   }
   return 0;
}

// * ---------------------------------------------------------
// * Will search out the selected network interface via win32 and copy mac etc.
// * ---------------------------------------------------------
static int populate_adapter_info (const char * eth_interface)
{
   PIP_ADAPTER_INFO pAdapterInfo;
   PIP_ADAPTER_INFO pAdapter = NULL;
   DWORD dwRetVal = 0;
   int ret = 0;
   struct sockaddr_in sa;
   char pcap_name[256];

   /* First check cache */
   if (strcmp (CurrentEthName, eth_interface) == 0)
      return 0;
   CurrentEthName[0] = 0; /* clear cache */

   /* translate from eth name to pcap name */
   if (get_pcap_name (eth_interface, pcap_name) < 0)
      return -EFAULT;

   ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);
   pAdapterInfo = (IP_ADAPTER_INFO *)malloc (sizeof (IP_ADAPTER_INFO));
   if (pAdapterInfo == NULL)
   {
      LOG_ERROR (
         PF_PNAL_LOG,
         "PNAL(%d): Allocating memory needed to call GetAdaptersinfo\n",
         __LINE__);
      return -EFAULT;
   }
   // Make an initial call to GetAdaptersInfo to get
   // the necessary size into the ulOutBufLen variable
   if (GetAdaptersInfo (pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
   {
      free (pAdapterInfo);
      pAdapterInfo = (IP_ADAPTER_INFO *)malloc (ulOutBufLen);
      if (pAdapterInfo == NULL)
      {
         LOG_ERROR (
            PF_PNAL_LOG,
            "PNAL(%d): Allocating memory needed to call GetAdaptersinfo\n",
            __LINE__);
         return -EFAULT;
      }
   }

   if ((dwRetVal = GetAdaptersInfo (pAdapterInfo, &ulOutBufLen)) == NO_ERROR)
   {
      pAdapter = pAdapterInfo;
      while (pAdapter)
      {
         if (strstr (pcap_name, pAdapter->AdapterName) != NULL)
         {
            // copy mac
            memcpy (Mac, pAdapter->Address, 6);

            // ip et.al
            inet_pton (
               AF_INET,
               pAdapter->IpAddressList.IpAddress.String,
               &(sa.sin_addr));
            Address = ntohl (sa.sin_addr.S_un.S_addr);
            inet_pton (
               AF_INET,
               pAdapter->IpAddressList.IpMask.String,
               &(sa.sin_addr));
            NetMask = ntohl (sa.sin_addr.S_un.S_addr);
            inet_pton (
               AF_INET,
               pAdapter->GatewayList.IpAddress.String,
               &(sa.sin_addr));
            Gateway = ntohl (sa.sin_addr.S_un.S_addr);

            strcpy (CurrentEthName, eth_interface);

            ret = 1;
            break;
         }

         // next
         pAdapter = pAdapter->Next;
      }
   }
   else
   {
      LOG_ERROR (
         PF_PNAL_LOG,
         "PNAL(%d): GetAdaptersInfo failed with error: %d\n",
         __LINE__,
         dwRetVal);
   }

   if (pAdapterInfo)
      free (pAdapterInfo);

   return ret;
}

// * ---------------------------------------------------------
// * select (via console) and start pcap interface
// * ---------------------------------------------------------
int pcap_helper_list_and_select_adapter (char * eth_interface)
{
   pcap_if_t * alldevs;
   pcap_if_t * d1;
   u_int inum1, i = 0;
   u_int NumOfIF = 0;

   char errbuf[256];

   printf ("\nNo adapter selected: printing the device list:\n");
   /* The user didn't provide a packet source: Retrieve the local device list */
   if (pcap_findalldevs (&alldevs, errbuf) == -1)
   {
      LOG_ERROR (
         PF_PNAL_LOG,
         "PNAL(%d): pcap_findalldevs_ex: %s\n",
         __LINE__,
         errbuf);
      return -EFAULT;
   }

   /* Print the list */
   for (d1 = alldevs; d1; d1 = d1->next)
   {
      printf ("%d. %s\t", ++i, d1->name);

      if (d1->description)
         printf (" (%s)\n", d1->description);
      else
         printf (" (No description available)\n");
   }

   NumOfIF = i;

   if (NumOfIF == 0)
   {
      printf ("\nNo interfaces found! Make sure NPcap is installed.\n");
      return -ENOENT;
   }

   if (NumOfIF >= 2)
   {
      printf ("Enter first interface number (1-%d):", NumOfIF);
      if (scanf ("%d", &inum1) < 1)
         inum1 = 0;
   }
   else
   {
      inum1 = 1; // take the one and only available interface automatically
   }

   if ((inum1 < 1) || (inum1 > NumOfIF))
   {
      printf ("\nInterface number out of range.\n");

      /* Free the device list */
      pcap_freealldevs (alldevs);
      return -ENOENT;
   }

   /* set result */
   sprintf (eth_interface, "eth%u", inum1);

   /* Free the device list */
   pcap_freealldevs (alldevs);

   return 0;
}

int pcap_helper_open (const char * eth_interface, PCAP_HANDLE* handle)
{
   char errbuf[256];
   char pcap_name[256];
   pcap_t * PcapId;

   *handle = NULL;

   /* get netmask */
   if (populate_adapter_info (eth_interface) < 0)
      return -EFAULT;

   /* translate from eth name to pcap name */
   if (get_pcap_name (eth_interface, pcap_name) < 0)
      return -EFAULT;

   /* Open the adapter */
   if (
      (PcapId = pcap_open (
          pcap_name, // name of the device
          65536,    // portion of the packet to capture.
                 // 65536 grants that the whole packet will be captured on every
                 // link layer.
          PCAP_OPENFLAG_PROMISCUOUS | // flags. We specify that we don't want to
                                      // capture loopback packets, and that the
                                      // driver should deliver us the packets as
                                      // fast as possible
             PCAP_OPENFLAG_NOCAPTURE_LOCAL | PCAP_OPENFLAG_MAX_RESPONSIVENESS,
          200,   // read timeout
          NULL,  // remote authentication
          errbuf // error buffer
          )) == NULL)
   {
      LOG_ERROR (
         PF_PNAL_LOG,
         "PNAL(%d): Opening adapter %s\n",
         __LINE__,
         eth_interface);
      return -EFAULT;
   }

   // ***** compile and activate filter ****
   if (pcap_compile (PcapId, &fpdat, FILTER_STRING, 1, NetMask) < 0)
   {
      LOG_ERROR (
         PF_PNAL_LOG,
         "PNAL(%d): pcap_compile\n",
         __LINE__);
      return -EFAULT;
   }
   if (pcap_setfilter (PcapId, &fpdat) < 0)
   {
      LOG_ERROR (PF_PNAL_LOG, "PNAL(%d): pcap_setfilter\n", __LINE__);
      return -EFAULT;
   }

   //good
   *handle = PcapId;
   return 0;
}

// * ---------------------------------------------------------
// * send data
// * ---------------------------------------------------------
int pcap_helper_send_telegram (PCAP_HANDLE handle, void * buffer, int length)
{
   int status = 0;

   status = pcap_sendpacket (
      (pcap_t*)handle, // pcap adapter
      buffer,  // pointer to ethernet packet
      length);

   if (status != 0)
   {
      LOG_ERROR (
         PF_PNAL_LOG,
         "PNAL(%d): Sending the packet, status=0x%x\n",
         __LINE__,
         status);
      return -EFAULT;
   }

   return length;
}

// * ---------------------------------------------------------
// * recieve next package. (Function will block)
// * Will return length of incomming data. (But only copy as much as buffer
// permits)
// * ---------------------------------------------------------
int pcap_helper_recieve_telegram (PCAP_HANDLE handle, void * buffer, int length)
{
   int res = 0;
   struct pcap_pkthdr * header = NULL;
   const u_char * pkt_data = NULL;

   while (1) // inner loop
   {
      res = pcap_next_ex ((pcap_t *)handle, &header, &pkt_data);
      if (res != 0)
         break;
   }

   if (res < 0)
   {
      LOG_ERROR (
         PF_PNAL_LOG,
         "PNAL(%d): Reading from NPcap: %s\n",
         __LINE__,
         pcap_geterr ((pcap_t *)handle));
      return -EFAULT;
   }

   // copy to buffer
   length = MIN ((int)header->caplen, length);
   memcpy (buffer, (void *)pkt_data, length);
   return length;
}
