/*********************************************************************
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#ifndef WINPCAP_HELPER_H
#define WINPCAP_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

int pcap_helper_init(void);
int pcap_helper_list_and_select_adapter (char * eth_interface);
int pcap_helper_populate_adapter_info (const char * eth_interface);

typedef void * PCAP_HANDLE;
int pcap_helper_open (const char * eth_interface, PCAP_HANDLE* handle);
int pcap_helper_send_telegram (PCAP_HANDLE handle, void * buffer, int length);
int pcap_helper_recieve_telegram (PCAP_HANDLE handle, void * buffer, int length);

unsigned int pcap_helper_get_ip();
unsigned int pcap_helper_get_netmask();
unsigned int pcap_helper_get_gateway();
unsigned char* pcap_helper_get_mac ();

#ifdef __cplusplus
}
#endif

#endif /* WINPCAP_HELPER_H */
