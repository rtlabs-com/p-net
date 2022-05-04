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

   /* Per port Ethernet link status.
    * Note that port numbers start at 1. To simplify test cases, we add a
    * dummy array element at index 0.
    */
   pnal_eth_status_t eth_status[PNET_MAX_PHYSICAL_PORTS + 1];
   pnal_port_stats_t port_statistics[PNET_MAX_PHYSICAL_PORTS + 1];

   uint16_t udp_sendto_len;
   uint16_t udp_sendto_count;

   uint8_t udp_recvfrom_buffer[PF_FRAME_BUFFER_SIZE];
   uint16_t udp_recvfrom_length;
   uint16_t udp_recvfrom_count;

   uint16_t set_ip_suite_count;

   uint32_t current_time_us;
   uint32_t system_uptime_10ms;

   int interface_index;
   pnal_eth_handle_t * eth_if_handle;

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

typedef struct mock_file_data
{
   char filename[PNET_MAX_FILENAME_SIZE];
   uint8_t object[300];
   size_t size;
   bool is_save_failing; /* Used for injecting error */
   bool is_load_failing; /* Used for injecting error */

} mock_file_data_t;

typedef struct mock_fspm_data
{
   char im_location[PNET_LOCATION_MAX_SIZE];
} mock_fspm_data_t;

extern mock_os_data_t mock_os_data;
extern mock_lldp_data_t mock_lldp_data;
extern mock_file_data_t mock_file_data;
extern mock_fspm_data_t mock_fspm_data;

uint32_t mock_os_get_current_time_us (void);
uint32_t mock_pnal_get_system_uptime_10ms (void);

void mock_init (void);
void mock_clear (void);
void mock_set_pnal_udp_recvfrom_buffer (uint8_t * p_src, uint16_t len);

pnal_eth_handle_t * mock_pnal_eth_init (
   const char * if_name,
   const pnal_cfg_t * pnal_cfg,
   pnal_eth_callback_t * callback,
   void * arg);
int mock_pnal_eth_send (pnal_eth_handle_t * handle, pnal_buf_t * buf);
int mock_pnal_get_macaddress (
   const char * interface_name,
   pnal_ethaddr_t * p_mac);
int mock_pnal_eth_get_status (
   const char * interface_name,
   pnal_eth_status_t * status);
int mock_pnal_get_port_statistics (
   const char * interface_name,
   pnal_port_stats_t * port_stats);
int mock_pnal_get_hostname (char * hostname);
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
int mock_pnal_get_interface_index (const char * interface_name);

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

int mock_pnal_snmp_init (pnet_t * pnet, const pnal_cfg_t * pnal_cfg);

int mock_pf_file_save_if_modified (
   const char * directory,
   const char * filename,
   const void * p_object,
   void * p_tempobject,
   size_t size);

int mock_pf_file_save (
   const char * directory,
   const char * filename,
   const void * p_object,
   size_t size);

void mock_pf_file_clear (const char * directory, const char * filename);

int mock_pf_file_load (
   const char * directory,
   const char * filename,
   void * p_object,
   size_t size);

void mock_pf_fspm_get_im_location (pnet_t * net, char * location);

void mock_pf_fspm_save_im_location (pnet_t * net, const char * location);

void mock_pf_bg_worker_init (pnet_t * net);

int mock_pf_bg_worker_start_job (pnet_t * net, pf_bg_job_t job_id);

#ifdef __cplusplus
}
#endif

#endif /* MOCKS_H */
