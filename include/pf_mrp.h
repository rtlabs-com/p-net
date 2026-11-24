/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2024 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

/**
 * @file
 * @brief Helper functions for use by platform-dependent MRP service.
 *
 * The p-net stack does not contain any MRP stack. Instead, it
 * relies on external software to supply the needed MRP services. In
 * the p-net abstraction layer (pnal), the file pnal_mrp.c is
 * expected to implement the glue code needed make the external MRP
 * service communicate with the p-net stack.
 *
 */

#ifndef PF_MRP_H
#define PF_MRP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pnet_api.h"

#define PF_FILENAME_MRP_CONFIG "pnet_data_mrp.bin"

#define MRP_VERSION 1

enum mrp_tlv_header_type
{
   MRP_TLV_TYPE_END = 0x0,
   MRP_TLV_TYPE_COMMON = 0x1,
   MRP_TLV_TYPE_RING_TEST = 0x2,
   MRP_TLV_TYPE_RING_TOPO = 0x3,
   MRP_TLV_TYPE_RING_LINK_DOWN = 0x4,
   MRP_TLV_TYPE_RING_LINK_UP = 0x5,
   MRP_TLV_TYPE_MAX = 0x6
};

#define MRP_TLV_TYPE_END_LEN 0

/* see 5.2.12.12.2.1 Coding of the field MRP_RingState */
enum mrp_ring_state
{
   MRP_RING_STATE_OPEN = 0,
   MRP_RING_STATE_CLOSED = 1,
};

typedef uint16_t mrp_ring_state_t;

typedef uint16_t mrp_transitions_t;

typedef uint16_t mrp_sequence_id_t;

typedef uint16_t mrp_prio_t;

typedef uint16_t mrp_interval_t;

typedef uint16_t mrp_port_role_t;

typedef uint16_t mrp_blocked_t;

/* raw uuid */
typedef struct mrp_uuid
{
   uint8_t data[16];
} mrp_uuid_t;

typedef mrp_uuid_t mrp_domain_uuid_t;

#define MRP_UUID_STRING_SIZE (36 + 1)

typedef struct mrp_mac_addr
{
   uint8_t addr[6];
} mrp_mac_addr_t;

typedef uint16_t mrp_version_t;

/**
 * MRP Domain UUID
 */

/* MRP_TLV_TYPE_COMMON */
typedef struct pf_mrp_common
{
   uint16_t sequence_id;
   mrp_domain_uuid_t domain;
} pf_mrp_common_t;
#define MRP_TLV_TYPE_COMMON_LEN 18

/* MRP_TLV_TYPE_RING_TEST */
typedef struct pf_mrp_test
{
   mrp_prio_t prio;
   mrp_mac_addr_t sa;
   mrp_port_role_t port_role;
   mrp_ring_state_t state;
   mrp_transitions_t transitions;
   uint32_t timestamp;
} pf_mrp_test_t;
#define MRP_TLV_TYPE_RING_TEST_LEN 18

/* MRP_TLV_TYPE_RING_TOPO */
typedef struct pf_mrp_topology
{
   mrp_prio_t prio;
   mrp_mac_addr_t sa;
   mrp_interval_t interval;
} pf_mrp_topology_t;
#define MRP_TLV_TYPE_RING_TOPO_LEN 10

/*
 * Common structure for link up/down
 *
 * MRP_TLV_TYPE_RING_LINK_DOWN
 * MRP_TLV_TYPE_RING_LINK_UP
 */
typedef struct pf_mrp_link
{
   mrp_mac_addr_t sa;
   mrp_port_role_t port_role;
   mrp_interval_t interval;
   mrp_blocked_t blocked;
} pf_mrp_link_t;
#define MRP_TLV_TYPE_RING_LINK_UP_DOWN_LEN 12

typedef enum
{
   FLAG_COMMON = (1 << 0),
   FLAG_TEST = (1 << 1),
   FLAG_TOPOLOGY = (1 << 2),
   FLAG_LINK_UP = (1 << 3),
   FLAG_LINK_DOWN = (1 << 4),
} mrp_update_flag_t;

/* structure holding all parsed mrp info */
typedef struct pf_mrp_info
{
   mrp_update_flag_t flags;
   pf_mrp_common_t common;
   pf_mrp_test_t test;
   pf_mrp_topology_t topo_chg;
   pf_mrp_link_t link_up;
   pf_mrp_link_t link_down;
} pf_mrp_info_t;

/*
 *
 * Profinet MRP structs and definitions
 *
 */

/* we support one client interface currently */
#define PF_MRP_NBR_INSTANCES 1

typedef struct pf_mrp_port
{

   /* network mrp config (holds current settings as parsed from mrp traffic) */
   mrp_ring_state_t state; /* port state */

   /* profinet mrp config (as configured by profinet controller) */
   mrp_domain_uuid_t domain; /* port domain */
   uint8_t instance;
} pf_mrp_port_t;

#define MRP_DOMAIN_NAME_LEN_MAX 240
#define MRP_DOMAIN_NAME_DEFAULT "mrpdomain-default"

typedef struct mrp_domain_name
{
   uint8_t len;
   char name[MRP_DOMAIN_NAME_LEN_MAX + 1];
} mrp_domain_name_t;

enum mrp_role
{
   MEDIA_REDUNDANCY_DISABLED = 0,
   MEDIA_REDUNDANCY_CLIENT = 1,
   MEDIA_REDUNDANCY_MANAGER = 2,
   MEDIA_REDUNDANCY_AUTO = 3,
};

typedef uint16_t mrp_role_t;

/* PF_BT_MRP_CLIENT_PARAMS */
typedef struct pf_mrc_client_params
{
   uint16_t link_down_T;
   uint16_t link_up_T;
   uint16_t link_nr_max;
} pf_mrp_client_params_t;

#define MRP_CLIENT_PARAM_LINK_DOWN_T_DEFAULT 0x14
#define MRP_CLIENT_PARAM_LINK_UP_T_DEFAULT   0x14
#define MRP_CLIENT_PARAM_LINK_NR_MAX_DEFAULT 0x4

typedef struct pf_mrp_instance_data_adjust_block
{
   uint8_t instance;
   mrp_domain_uuid_t domain;
   mrp_role_t role;
   uint32_t mrp_check;
   uint8_t domain_len;
   char domain_name[MRP_DOMAIN_NAME_LEN_MAX + 1];

   /*  optional sub elements */
   pf_mrp_client_params_t client_params;
} pf_mrp_instance_data_adjust_block_t;

typedef struct pf_intf_mrp_data_adjust
{
   uint8_t nbr_instances;
   pf_mrp_instance_data_adjust_block_t instance[PF_MRP_NBR_INSTANCES];
} pf_intf_mrp_data_adjust_t;

typedef struct pf_mrp_check_flag
{
   uint32_t redundance_mgr_diag : 1;
   uint32_t lldp_domain : 1;
   uint32_t reserved_1 : 22;
   uint32_t reserved_2 : 8;
} pf_mrp_check_flag_t;

typedef struct pf_mrp_data_check_instance
{
   uint8_t instance;
   mrp_domain_uuid_t domain;
   pf_mrp_check_flag_t flags;
} pf_mrp_data_check_instance_t;

typedef struct pf_mrp_data_check
{
   uint8_t nbr_instances;
   pf_mrp_data_check_instance_t instance[PF_MRP_NBR_INSTANCES];
} pf_mrp_data_check_t;

typedef struct pf_mrp_config
{
   mrp_version_t version;
   mrp_role_t role;
   mrp_domain_uuid_t domain;
   mrp_domain_name_t domain_name;
   pf_mrp_client_params_t client_params;
   bool role_updated;
} pf_mrp_config_t;

/*
 * Main MRP interface struct
 */

typedef struct pf_mrp_data
{
   pf_mrp_config_t config;
   pf_mrp_data_check_t check;
   bool check_active;
} pf_mrp_intf_data_t;

/**
 * Save current configuration data to file
 *
 * @param net              InOut: The p-net stack instance.
 */
void pf_mrp_save_config (pnet_t * net);

/**
 * Convert domain uuid into string
 *
 * @param domain           In: MRP domain UUID
 * @param uuid_string      InOut: The UUID string
 */
void pf_mrp_domain_uuid_str (
   const mrp_domain_uuid_t * domain,
   char uuid_string[MRP_UUID_STRING_SIZE]);

/**
 * Initialize MRP.
 *
 * @param net              InOut: The p-net stack instance.
 */
void pf_mrp_init (pnet_t * net);

/**
 * Notify MRP is now enabled from controller side
 *
 * @param net              InOut: The p-net stack instance.
 */
void pf_mrp_enable (pnet_t * net);

/**
 * Notify MRP is now disable from controller side
 *
 * @param net              InOut: The p-net stack instance.
 */
void pf_mrp_disable (pnet_t * net);

/**
 * Initialize MRP related data, by reading from file
 *
 * @param net              InOut: The p-net stack instance.
 * @param default_reset    In: Reset MRP parameters to default if true.
 */
void pf_mrp_data_init (pnet_t * net, bool default_reset);

/**
 * Clear MRP related data.
 *
 *
 * @param net              InOut: The p-net stack instance.
 */
void pf_mrp_data_clear (pnet_t * net);

/**
 * Removes MRP related data files.
 *
 * @param file_directory              In: Directory path for MRP file(s)
 */
void pf_mrp_remove_data_files (const char * file_directory);

/**
 * Process received MRP message.
 *
 * Parse MRP TLV format and store selected information.
 * @param net              InOut: The p-net stack instance.
 * @param loc_port_num     In:    Local port number the frame was received on.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_port_get_list_of_ports().
 *                                Frame is dropped for invalid values.
 * @param p_frame_buf      InOut: Received LLDP message.
 *                                Will be freed.
 * @param frame_pos        In:    The offset to start of MRP data.
 * @return  0     If the frame was NOT handled by this function.
 *          1     If the frame was handled and the buffer freed.
 */
int pf_mrp_recv (
   pnet_t * net,
   int loc_port_num,
   pnal_buf_t * p_frame_buf,
   uint16_t frame_pos);

/**
 * Show MRP details
 *
 * @param net              InOut: The p-net stack instance
 */
void pf_mrp_show (pnet_t * net);

#ifdef __cplusplus
}
#endif

#endif /* PF_MRP_H */
