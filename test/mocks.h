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

#ifndef MOCKS_H
#define MOCKS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "options.h"
#include "pnet_api.h"
#include "pf_includes.h"
#include "osal.h"

typedef struct mock_os_data_obj
{
   uint8_t eth_send_copy[PF_FRAME_BUFFER_SIZE];
   uint16_t eth_send_len;
   uint16_t eth_send_count;

   uint16_t udp_sendto_len;
   uint16_t udp_sendto_count;

   uint8_t udp_recvfrom_buffer[PF_FRAME_BUFFER_SIZE];
   uint16_t udp_recvfrom_length;
   uint16_t udp_recvfrom_count;

   uint16_t set_ip_suite_count;

   uint32_t current_time_us;

   char file_fullpath[100]; /* Full file path at latest save operation */
   uint16_t file_size;
   uint8_t file_content[2000];

} mock_os_data_t;

typedef struct mock_lldp_data
{
   pf_lldp_management_address_t management_address;
   pf_lldp_management_address_t peer_management_address;
   pf_lldp_link_status_t link_status;
   pf_lldp_link_status_t peer_link_status;
   int error;

} mock_lldp_data_t;

extern mock_os_data_t mock_os_data;
extern mock_lldp_data_t mock_lldp_data;

uint32_t mock_os_get_current_time_us (void);

void mock_init (void);
void mock_clear (void);
void mock_set_pnal_udp_recvfrom_buffer (uint8_t * p_src, uint16_t len);

pnal_eth_handle_t * mock_pnal_eth_init (
   const char * if_name,
   pnal_eth_callback_t * callback,
   void * arg);
int mock_pnal_eth_send (pnal_eth_handle_t * handle, pnal_buf_t * buf);
void mock_os_cpy_mac_addr (uint8_t * mac_addr);
int mock_pnal_udp_open (pnal_ipaddr_t addr, pnal_ipport_t port);
int mock_pnal_udp_sendto (
   uint32_t id,
   pnal_ipaddr_t dst_addr,
   pnal_ipport_t dst_port,
   const uint8_t * data,
   int size);
int mock_pnal_udp_recvfrom (
   uint32_t id,
   pnal_ipaddr_t * dst_addr,
   pnal_ipport_t * dst_port,
   uint8_t * data,
   int size);
void mock_pnal_udp_close (uint32_t id);
int mock_pnal_set_ip_suite (
   const char * interface_name,
   const pnal_ipaddr_t * p_ipaddr,
   const pnal_ipaddr_t * p_netmask,
   const pnal_ipaddr_t * p_gw,
   const char * hostname,
   bool permanent);

int mock_pnal_save_file (
   const char * fullpath,
   const void * object_1,
   size_t size_1,
   const void * object_2,
   size_t size_2);

void mock_pnal_clear_file (const char * fullpath);

int mock_pnal_load_file (
   const char * fullpath,
   void * object_1,
   size_t size_1,
   void * object_2,
   size_t size_2);

int mock_pf_alarm_send_diagnosis (
   pf_ar_t * p_ar,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   pf_diag_item_t * p_item);

void mock_pf_generate_uuid (
   uint32_t timestamp,
   uint32_t session_number,
   pnet_ethaddr_t mac_address,
   pf_uuid_t * p_uuid);

void mock_pf_lldp_get_management_address (
   pnet_t * net,
   pf_lldp_management_address_t * p_man_address);

int mock_pf_lldp_get_peer_management_address (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_management_address_t * p_man_address);

void mock_pf_lldp_get_link_status (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_link_status_t * p_link_status);

int mock_pf_lldp_get_peer_link_status (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_link_status_t * p_link_status);

int mock_pnal_snmp_init (pnet_t * pnet);

#ifdef __cplusplus
}
#endif

#endif /* MOCKS_H */
