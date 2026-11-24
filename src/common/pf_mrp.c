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
 * @brief Helper functions for use by platform-dependent MRP agent.
 *
 * Some SNMP related data needs to be stored to file between invocations.
 * We read from file only at startup, but writing to file might happen any time.
 *
 */

#ifdef UNIT_TEST
#define pnal_mrp_config mock_pnal_mrp_config
#define pnal_mrp_status mock_pnal_mrp_status
#endif

#include "pf_includes.h"
#include "pf_block_writer.h"
#include "pf_block_reader.h"

#include <string.h>

#define MRP_VERSION 1

const uint8_t NULL_ADDR[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t MC_TEST_ADDR[6] = {0x01, 0x15, 0x4e, 0x00, 0x00, 0x01};
const uint8_t MC_CONTROL_ADDR[6] = {0x01, 0x15, 0x4e, 0x00, 0x00, 0x02};

typedef struct mrp_tlv
{
   uint8_t type;
   uint8_t len;
} mrp_tlv_t;

static void mrp_show (pnet_t * net);

static void pf_mrp_update_oper_state (
   pnet_t * net,
   int loc_port_num,
   pf_mrp_info_t * info);

static bool mrp_addr_filter (pnal_buf_t const * buf, uint16_t offset)
{
   return (
      (pnal_buf_compare (buf, offset, MC_CONTROL_ADDR, 6) == 0) ||
      (pnal_buf_compare (buf, offset, MC_TEST_ADDR, 6) == 0));
}

void pf_mrp_domain_uuid_str (
   const mrp_domain_uuid_t * domain,
   char uuid_string[MRP_UUID_STRING_SIZE])
{
   snprintf (
      uuid_string,
      MRP_UUID_STRING_SIZE,
      "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      domain->data[0],
      domain->data[1],
      domain->data[2],
      domain->data[3],
      domain->data[4],
      domain->data[5],
      domain->data[6],
      domain->data[7],
      domain->data[8],
      domain->data[9],
      domain->data[10],
      domain->data[11],
      domain->data[12],
      domain->data[13],
      domain->data[14],
      domain->data[15]);
}

/**
 * @internal
 * Parse MRP TLV header.
 * Each TLV is structured as follows:
 * - Type   = 8 bits
 * - Length = 8 bits
 * - data   = 0-255 bytes
 *
 * @param p_info           InOut: Parser state
 * @param p_pos            InOut: Offset in parsed buffer
 * @return parsed frame. If parsing fails, frame type is set to
 * MRP_TLV_TYPE_END and len field set to 0.
 */
mrp_tlv_t pf_mrp_get_tlv_from_packet (pf_get_info_t * p_info, uint16_t * p_pos)
{
   mrp_tlv_t tlv;

   if (p_info->result == PF_PARSE_OK)
   {
      tlv.type = pf_get_byte (p_info, p_pos);
      tlv.len = pf_get_byte (p_info, p_pos);
   }
   else
   {
      tlv.type = MRP_TLV_TYPE_END;
      tlv.len = 0;
   }
   return tlv;
}

/**
 * MRP common
 *
 * Layout of TLV:
 * 2     MRP TLV header (already processed)
 * 2     Sequence ID
 * 16    Domain UUID
 *
 *
 * @param parse_info       InOut: Parsing information.
 * @param offset           InOut: Byte offset in buffer.
 * @param tlv_len          In:    Length of TLV in bytes.
 * @param sequence_id      Out:   Sequence ID.
 * @param domain_uuid      Out:   Parsed MRP redundancy domain uuid
 */

void pf_mrp_parse_tlv_common (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   const size_t tlv_len,
   mrp_sequence_id_t * sequence_id,
   mrp_domain_uuid_t * domain_uuid)
{
   if (tlv_len != MRP_TLV_TYPE_COMMON_LEN)
   {
      p_info->result = PF_PARSE_ERROR;
      return;
   }
   *sequence_id = pf_get_uint16 (p_info, p_pos);
   pf_get_mem (p_info, p_pos, sizeof (mrp_domain_uuid_t), domain_uuid->data);
}

/**
 * MRP ring test
 *
 * Layout of TLV:
 * 2     MRP TLV header (already processed)
 * 2     MRP priority (0=max prio) See IEC 62439-2_2016 - 8.1.15
 * 6     MAC address of sending switch host (MAC address)
 * 2     Interval for next topology changed event (in ms)
 * 2     Nbr of transitions between ring open/closed state
 * 4     Timestamp
 *
 * @param parse_info       InOut: Parsing information.
 * @param offset           InOut: Byte offset in buffer.
 * @param tlv_len          In:    Length of TLV in bytes.
 * @param mrp_prio         Out:   Priority .
 * @param sa               Out:   SA MAC address
 * @param role             Out:   Port role
 * @param ring_state       Out:   Ring state
 * @param transitions      Out:   Transitions
 * @param timestamp        Out:   Timestamp
 */

void pf_mrp_parse_tlv_test (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   const size_t tlv_len,
   mrp_prio_t * mrp_prio,
   mrp_mac_addr_t * sa,
   mrp_port_role_t * role,
   mrp_ring_state_t * state,
   mrp_transitions_t * transitions,
   uint32_t * timestamp)
{
   if (tlv_len != MRP_TLV_TYPE_RING_TEST_LEN)
   {
      p_info->result = PF_PARSE_ERROR;
      return;
   }
   *mrp_prio = pf_get_uint16 (p_info, p_pos);
   pf_get_mem (p_info, p_pos, sizeof (mrp_mac_addr_t), sa->addr);
   *role = pf_get_uint16 (p_info, p_pos);
   *state = pf_get_uint16 (p_info, p_pos);
   *transitions = pf_get_uint16 (p_info, p_pos);
   *timestamp = pf_get_uint32 (p_info, p_pos);
}

/**
 * MRP topology changed event
 *
 * Layout of TLV:
 * 2     MRP TLV header (already processed)
 * 2     MRP priority (0=max prio) See IEC 62439-2_2016 - 8.1.15
 * 6     MAC address of sending switch host (MAC address)
 * 2     Interval for next topology changed event (in ms)
 *
 * @param parse_info       InOut: Parsing information.
 * @param offset           InOut: Byte offset in buffer.
 * @param tlv_len          In:    Length of TLV in bytes.
 * @param mrp_prio         Out:   Priority .
 * @param sa               Out:   SA MAC address
 * @param mrp_interval     Out:   Interval
 */

void pf_mrp_parse_tlv_topology_chg (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   const size_t tlv_len,
   mrp_prio_t * mrp_prio,
   mrp_mac_addr_t * sa,
   uint16_t * interval)
{
   if (tlv_len != MRP_TLV_TYPE_RING_TOPO_LEN)
   {
      p_info->result = PF_PARSE_ERROR;
      return;
   }
   *mrp_prio = pf_get_uint16 (p_info, p_pos);
   pf_get_mem (p_info, p_pos, sizeof (mrp_mac_addr_t), sa->addr);
   *interval = pf_get_uint16 (p_info, p_pos);
}

/**
 * MRP link up / down
 *
 * Layout of TLV:
 * 2     MRP TLV header (already processed)
 * 2     MRP priority (0=max prio) See IEC 62439-2_2016 - 8.1.15
 * 6     MAC address of sending switch host (MAC address)
 * 2     Ring port role (0=Primary 1=Secondary 2=Interconnection)
 * 2     Interval for next topology changed event (in ms)
 * 2     Ability to forward test/link change frames on BLOCKED port
 *
 *
 * @param parse_info       InOut: Parsing information.
 * @param offset           InOut: Byte offset in buffer.
 * @param tlv_len          In:    Length of TLV in bytes.
 * @param sa               Out:   SA MAC address
 * @param port_role        Out:   Port role
 * @param mrp_interval     Out:   Interval
 * @param blocked          Out:   Blocked port state
 */

void pf_mrp_parse_tlv_link_up_down (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   const size_t tlv_len,
   mrp_mac_addr_t * sa,
   mrp_port_role_t * role,
   mrp_interval_t * interval,
   mrp_blocked_t * blocked)
{
   if (tlv_len < MRP_TLV_TYPE_RING_LINK_UP_DOWN_LEN)
   {
      p_info->result = PF_PARSE_ERROR;
      return;
   }
   pf_get_mem (p_info, p_pos, sizeof (mrp_mac_addr_t), sa->addr);
   *role = pf_get_uint16 (p_info, p_pos);
   *interval = pf_get_uint16 (p_info, p_pos);
   *blocked = pf_get_uint16 (p_info, p_pos);
}

/**
 * @internal
 * Parse MRP frame buffer
 *
 * @param buf              In:    MRP frame buffer.
 * @param offset           In:    Offset of MRP data.
 * @param pf_mrp_info     Out:    Record holding parsed MRP information.
 *                                Nulled initially (also on failure).
 * @return 0 if parsing is successful, -1 on error.
 */

static int pf_mrp_parse_packet (
   pnal_buf_t * buf,
   uint16_t offset,
   pf_mrp_info_t * mrp_info)
{
   mrp_tlv_t tlv;
   pf_get_info_t parse_info;
   mrp_version_t mrp_version;

   memset (mrp_info, 0, sizeof (*mrp_info));

   parse_info.result = PF_PARSE_OK;
   parse_info.is_big_endian = true;
   parse_info.len = pnal_buf_get_size(buf);
   parse_info.p_frame = buf;
   parse_info.p_array = NULL;

   mrp_version = pf_get_uint16 (&parse_info, &offset);

   /* allow only mrp version 1 */
   if (mrp_version != MRP_VERSION)
   {
      LOG_WARNING (
         PF_MRP_LOG,
         "MRP(%d): version %d not supported\n",
         __LINE__,
         mrp_version);
      return -1;
   }

   while (tlv = pf_mrp_get_tlv_from_packet (&parse_info, &offset),
          tlv.type != MRP_TLV_TYPE_END)
   {
      switch (tlv.type)
      {
      case MRP_TLV_TYPE_COMMON:
         pf_mrp_parse_tlv_common (
            &parse_info,
            &offset,
            tlv.len,
            &mrp_info->common.sequence_id,
            &mrp_info->common.domain);
         mrp_info->flags |= FLAG_COMMON;
         break;
      case MRP_TLV_TYPE_RING_TEST:
         pf_mrp_parse_tlv_test (
            &parse_info,
            &offset,
            tlv.len,
            &mrp_info->test.prio,
            &mrp_info->test.sa,
            &mrp_info->test.port_role,
            &mrp_info->test.state,
            &mrp_info->test.transitions,
            &mrp_info->test.timestamp);
         mrp_info->flags |= FLAG_TEST;
         break;
      case MRP_TLV_TYPE_RING_TOPO:
         pf_mrp_parse_tlv_topology_chg (
            &parse_info,
            &offset,
            tlv.len,
            &mrp_info->topo_chg.prio,
            &mrp_info->topo_chg.sa,
            &mrp_info->topo_chg.interval);
         mrp_info->flags |= FLAG_TOPOLOGY;
         break;
      case MRP_TLV_TYPE_RING_LINK_DOWN:
         pf_mrp_parse_tlv_link_up_down (
            &parse_info,
            &offset,
            tlv.len,
            &mrp_info->link_down.sa,
            &mrp_info->link_down.port_role,
            &mrp_info->link_down.interval,
            &mrp_info->link_down.blocked);
         mrp_info->flags |= FLAG_LINK_DOWN;
         break;
      case MRP_TLV_TYPE_RING_LINK_UP:
         pf_mrp_parse_tlv_link_up_down (
            &parse_info,
            &offset,
            tlv.len,
            &mrp_info->link_up.sa,
            &mrp_info->link_up.port_role,
            &mrp_info->link_up.interval,
            &mrp_info->link_up.blocked);
         mrp_info->flags |= FLAG_LINK_UP;
         break;
      default:
         offset += tlv.len;
         LOG_WARNING (
            PF_MRP_LOG,
            "MRP(%d): tlv %d not parsed\n",
            __LINE__,
            tlv.type);
         break;
      }
   }

   if (parse_info.result != PF_PARSE_OK)
   {
      LOG_ERROR (
         PF_MRP_LOG,
         "MRP(%d): Received MRP frame, but failed to parse it.\n",
         __LINE__);

      return -1;
   }

   return 0;
}

int pf_mrp_recv (
   pnet_t * net,
   int loc_port_num,
   pnal_buf_t * p_frame_buf,
   uint16_t offset)
{
   pf_mrp_info_t mrp_info;
   int err = 0;

   if (!mrp_addr_filter (p_frame_buf, offset))
      return 0;

   err = pf_mrp_parse_packet (p_frame_buf, offset, &mrp_info);

   if (!err)
   {
      if (pf_port_is_valid (net, loc_port_num))
      {
         pf_mrp_update_oper_state (net, loc_port_num, &mrp_info);
      }
      else
      {
         LOG_ERROR (
            PF_MRP_LOG,
            "MRP(%d): Unexpected local port %u, frame discarded.\n",
            __LINE__,
            loc_port_num);
      }
   }

   pnal_buf_free (p_frame_buf);

   return 1; /* Means: handled */
}

void pf_mrp_update_oper_state (
   pnet_t * net,
   int loc_port_num,
   pf_mrp_info_t * info)
{
   /* not yet clear how actual network side mrp affects profinet mrp other than
    * profinet checking that configuration matches expected settings and raise
    * alarms otherwise */

   /* check for key change events */
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   if (info->flags & FLAG_COMMON)
   {
      bool diff;

      os_mutex_lock (net->mrp_mutex);
      diff =
         (memcmp (
             info->common.domain.data,
             p_port_data->mrp.domain.data,
             sizeof (mrp_domain_uuid_t)) != 0);
      os_mutex_unlock (net->mrp_mutex);

      if (diff)
      {
         char uuidstr[MRP_UUID_STRING_SIZE];

         pf_mrp_domain_uuid_str (&info->common.domain, uuidstr);
         LOG_INFO (
            PF_MRP_LOG,
            "MRP(%d): New MRP Domain UUID on port %d [%s]\n",
            __LINE__,
            loc_port_num,
            uuidstr);
      }
   }

   if (
      (info->flags & FLAG_TOPOLOGY) &&
      memcmp (info->topo_chg.sa.addr, NULL_ADDR, sizeof (mrp_mac_addr_t) != 0))
   {
      LOG_INFO (
         PF_MRP_LOG,
         "MRP(%d): Topology changed on port %d interval %d ms\n",
         __LINE__,
         loc_port_num,
         info->topo_chg.interval);
   }

   if (info->flags & FLAG_TEST)
   {
      bool diff;

      os_mutex_lock (net->mrp_mutex);
      diff = (p_port_data->mrp.state != info->test.state);
      p_port_data->mrp.state = info->test.state;
      os_mutex_unlock (net->mrp_mutex);

      if (diff)
      {
         LOG_INFO (
            PF_MRP_LOG,
            "MRP(%d): Ring state changed on port %d state %d\n",
            __LINE__,
            loc_port_num,
            info->test.state);
      }
   }

   if (
      (info->flags & FLAG_LINK_UP) &&
      memcmp (info->link_up.sa.addr, NULL_ADDR, sizeof (mrp_mac_addr_t) != 0))
   {
      LOG_INFO (
         PF_MRP_LOG,
         "MRP(%d): Link up on port %d\n",
         __LINE__,
         loc_port_num);
   }
   if (
      (info->flags & FLAG_LINK_DOWN) &&
      memcmp (info->link_down.sa.addr, NULL_ADDR, sizeof (mrp_mac_addr_t) != 0))
   {
      LOG_INFO (
         PF_MRP_LOG,
         "MRP(%d): Link down on port %d\n",
         __LINE__,
         loc_port_num);
   }
}

static void mrp_get_default_config (pf_mrp_config_t * cfg)
{
   memset (cfg, 0, sizeof (pf_mrp_config_t));

   memset (cfg->domain.data, 0xFF, sizeof (mrp_domain_uuid_t));
   cfg->version = MRP_VERSION;
   cfg->domain_name.len = sizeof (MRP_DOMAIN_NAME_DEFAULT) - 1;
   strcpy (cfg->domain_name.name, MRP_DOMAIN_NAME_DEFAULT);
   cfg->role = MEDIA_REDUNDANCY_CLIENT;
   cfg->client_params.link_down_T = MRP_CLIENT_PARAM_LINK_DOWN_T_DEFAULT;
   cfg->client_params.link_up_T = MRP_CLIENT_PARAM_LINK_UP_T_DEFAULT;
   cfg->client_params.link_nr_max = MRP_CLIENT_PARAM_LINK_NR_MAX_DEFAULT;
}

static void init_ring_ports (pnet_t * net, mrp_domain_uuid_t * domain)
{
   int port;
   pf_port_iterator_t port_iterator;
   pf_port_t * p_port_data = NULL;

   pf_port_init_iterator_over_ports (net, &port_iterator);

   port = pf_port_get_next (&port_iterator);
   while (port != 0)
   {
      p_port_data = pf_port_get_state (net, port);
      os_mutex_lock (net->mrp_mutex);
      memcpy (
         p_port_data->mrp.domain.data,
         domain->data,
         sizeof (mrp_domain_uuid_t));
      p_port_data->mrp.instance = 0;
      os_mutex_unlock (net->mrp_mutex);
      port = pf_port_get_next (&port_iterator);
   }
}

void pf_mrp_save_config (pnet_t * net)
{
   pf_mrp_intf_data_t * mrp = &net->mrp_data;
   pf_mrp_intf_data_t temporary_buffer;
   const char * directory = pf_cmina_get_file_directory (net);

   /* save to file */
   if (
      pf_file_save_if_modified (
         directory,
         PF_FILENAME_MRP_CONFIG,
         mrp,
         &temporary_buffer,
         sizeof (pf_mrp_intf_data_t)) > 0)
   {
      LOG_INFO (PF_MRP_LOG, "MRP(%d): local config saved to nvram\n", __LINE__);
   }
}

void pf_mrp_enable (pnet_t * net)
{
   pnal_mrp_cfg_t cfg;

   memcpy (
      cfg.domain_uuid,
      net->mrp_data.config.domain.data,
      sizeof (mrp_domain_uuid_t));
   cfg.domain_name = net->mrp_data.config.domain_name.name;
   cfg.link_down_T = net->mrp_data.config.client_params.link_down_T;
   cfg.link_up_T = net->mrp_data.config.client_params.link_up_T;
   cfg.link_nr_max = net->mrp_data.config.client_params.link_nr_max;

   pnal_mrp_config (true, &cfg);
}

void pf_mrp_disable (pnet_t * net)
{
   /* set default UUID */
   memset (
      net->mrp_data.config.domain.data,
      0xff,
      sizeof (mrp_domain_uuid_t));

   pnal_mrp_config (false, NULL);
}

void pf_mrp_init (pnet_t * net)
{
   net->mrp_mutex = os_mutex_create();
   CC_ASSERT (net->mrp_mutex != NULL);
}

void pf_mrp_data_init (pnet_t * net, bool default_reset)
{
   const char * directory = pf_cmina_get_file_directory (net);
   pf_mrp_intf_data_t * mrp = &net->mrp_data;
   int error;

   /* only support one instance currently */
   CC_ASSERT (PF_MRP_NBR_INSTANCES == 1);

   /* read mrp config */
   error = pf_file_load (
      directory,
      PF_FILENAME_MRP_CONFIG,
      mrp,
      sizeof (pf_mrp_intf_data_t));

   if (error || default_reset)
   {
      /* file failed to load, set default values */
      mrp_get_default_config (&mrp->config);

      /* initialize check info */
      memset ((uint8_t *)&mrp->check, 0, sizeof (pf_mrp_data_check_t));
      mrp->check_active = false;

      pf_mrp_save_config (net);
   }
   else
   {
      LOG_INFO (PF_MRP_LOG, "MRP(%d): local config loaded\n", __LINE__);
   }

   init_ring_ports (net, &mrp->config.domain);

   if (mrp->config.role == MEDIA_REDUNDANCY_CLIENT)
   {
      pf_mrp_enable (net);
   }
   else
   {
      pf_mrp_disable (net);
   }
}

void pf_mrp_remove_data_files (const char * file_directory)
{
   pf_file_clear (file_directory, PF_FILENAME_MRP_CONFIG);
}

void pf_mrp_data_clear (pnet_t * net)
{
   pf_mrp_intf_data_t * mrp = &net->mrp_data;

   /* clear runtime info */
   mrp_get_default_config (&mrp->config);

   /* initialize check info */
   memset ((uint8_t *)&mrp->check, 0, sizeof (pf_mrp_data_check_t));
   mrp->check_active = false;
}

static void mrp_show (pnet_t * net)
{
   pf_mrp_intf_data_t * mrp = &net->mrp_data;
   char uuid_str[MRP_UUID_STRING_SIZE];

   pf_mrp_domain_uuid_str (&mrp->config.domain, uuid_str);

   printf (
      " role : %s\n",
      (mrp->config.role == MEDIA_REDUNDANCY_CLIENT) ? "client" : "disabled");
   printf (" domain uuid   : %s\n", uuid_str);
   printf (" domain name   : %s\n", mrp->config.domain_name.name);
}

void pf_mrp_show (pnet_t * net)
{
   printf ("\nMRP\n");
   mrp_show (net);
   pnal_mrp_status();
}
