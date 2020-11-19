/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2020 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#include "lldp-ext-pno-mib.h"

#include "options.h"
#include "pnet_api.h"
#include "osal.h"
#include "osal_log.h"
#include "pnal.h"
#include "pf_types.h"
#include "pf_snmp.h"
#include "pnal_snmp.h"
#include "rowindex.h"

#include <string.h>

/*
Generated by LwipMibCompiler
*/

#include "lwip/apps/snmp_opts.h"
#if LWIP_SNMP

#include "lldp-ext-pno-mib.h"
#include "lwip/apps/snmp.h"
#include "lwip/apps/snmp_core.h"
#include "lwip/apps/snmp_scalar.h"
#include "lwip/apps/snmp_table.h"

/* --- lldpXPnoLocalData 1.0.8802.1.1.2.1.5.3791.1.2
 * ----------------------------------------------------- */
static snmp_err_t lldpxpnoloctable_get_instance (
   const u32_t * column,
   const u32_t * row_oid,
   u8_t row_oid_len,
   struct snmp_node_instance * cell_instance);
static snmp_err_t lldpxpnoloctable_get_next_instance (
   const u32_t * column,
   struct snmp_obj_id * row_oid,
   struct snmp_node_instance * cell_instance);
static s16_t lldpxpnoloctable_get_value (
   struct snmp_node_instance * cell_instance,
   void * value);
static const struct snmp_table_col_def lldpxpnoloctable_columns[] = {
   {1,
    SNMP_ASN1_TYPE_GAUGE,
    SNMP_NODE_INSTANCE_READ_ONLY}, /* lldpXPnoLocLPDValue
                                    */
   {2,
    SNMP_ASN1_TYPE_GAUGE,
    SNMP_NODE_INSTANCE_READ_ONLY}, /* lldpXPnoLocPortTxDValue
                                    */
   {3,
    SNMP_ASN1_TYPE_GAUGE,
    SNMP_NODE_INSTANCE_READ_ONLY}, /* lldpXPnoLocPortRxDValue
                                    */
   {6,
    SNMP_ASN1_TYPE_OCTET_STRING,
    SNMP_NODE_INSTANCE_READ_ONLY}, /* lldpXPnoLocPortNoS
                                    */
};
static const struct snmp_table_node lldpxpnoloctable = SNMP_TABLE_CREATE (
   1,
   lldpxpnoloctable_columns,
   lldpxpnoloctable_get_instance,
   lldpxpnoloctable_get_next_instance,
   lldpxpnoloctable_get_value,
   NULL,
   NULL);

static const struct snmp_node * const lldpxpnolocaldata_subnodes[] = {
   &lldpxpnoloctable.node.node,
};
static const struct snmp_tree_node lldpxpnolocaldata_treenode =
   SNMP_CREATE_TREE_NODE (2, lldpxpnolocaldata_subnodes);

/* --- lldpXPnoRemoteData 1.0.8802.1.1.2.1.5.3791.1.3
 * ----------------------------------------------------- */
static snmp_err_t lldpxpnoremtable_get_instance (
   const u32_t * column,
   const u32_t * row_oid,
   u8_t row_oid_len,
   struct snmp_node_instance * cell_instance);
static snmp_err_t lldpxpnoremtable_get_next_instance (
   const u32_t * column,
   struct snmp_obj_id * row_oid,
   struct snmp_node_instance * cell_instance);
static s16_t lldpxpnoremtable_get_value (
   struct snmp_node_instance * cell_instance,
   void * value);
static const struct snmp_table_col_def lldpxpnoremtable_columns[] = {
   {1,
    SNMP_ASN1_TYPE_GAUGE,
    SNMP_NODE_INSTANCE_READ_ONLY}, /* lldpXPnoRemLPDValue
                                    */
   {2,
    SNMP_ASN1_TYPE_GAUGE,
    SNMP_NODE_INSTANCE_READ_ONLY}, /* lldpXPnoRemPortTxDValue
                                    */
   {3,
    SNMP_ASN1_TYPE_GAUGE,
    SNMP_NODE_INSTANCE_READ_ONLY}, /* lldpXPnoRemPortRxDValue
                                    */
   {6,
    SNMP_ASN1_TYPE_OCTET_STRING,
    SNMP_NODE_INSTANCE_READ_ONLY}, /* lldpXPnoRemPortNoS
                                    */
};
static const struct snmp_table_node lldpxpnoremtable = SNMP_TABLE_CREATE (
   1,
   lldpxpnoremtable_columns,
   lldpxpnoremtable_get_instance,
   lldpxpnoremtable_get_next_instance,
   lldpxpnoremtable_get_value,
   NULL,
   NULL);

static const struct snmp_node * const lldpxpnoremotedata_subnodes[] = {
   &lldpxpnoremtable.node.node,
};
static const struct snmp_tree_node lldpxpnoremotedata_treenode =
   SNMP_CREATE_TREE_NODE (3, lldpxpnoremotedata_subnodes);

/* --- lldpXPnoObjects 1.0.8802.1.1.2.1.5.3791.1
 * ----------------------------------------------------- */
static const struct snmp_node * const lldpxpnoobjects_subnodes[] = {
   &lldpxpnolocaldata_treenode.node,
   &lldpxpnoremotedata_treenode.node};
static const struct snmp_tree_node lldpxpnoobjects_treenode =
   SNMP_CREATE_TREE_NODE (1, lldpxpnoobjects_subnodes);

/* --- lldpXPnoMIB  ----------------------------------------------------- */
static const struct snmp_node * const lldpxpnomib_subnodes[] = {
   &lldpxpnoobjects_treenode.node};
static const struct snmp_tree_node lldpxpnomib_root =
   SNMP_CREATE_TREE_NODE (3791, lldpxpnomib_subnodes);
static const u32_t lldpxpnomib_base_oid[] = {1, 0, 8802, 1, 1, 2, 1, 5, 3791};
const struct snmp_mib lldpxpnomib = {
   lldpxpnomib_base_oid,
   LWIP_ARRAYSIZE (lldpxpnomib_base_oid),
   &lldpxpnomib_root.node};

/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
LWIP MIB generator - preserved section begin
Code below is preserved on regeneration. Remove these comment lines to
regenerate code.
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

/* --- lldpXPnoLocalData 1.0.8802.1.1.2.1.5.3791.1.2
 * ----------------------------------------------------- */

/**
 * Get cell in table lldpXPnoLocTable.
 *
 * Called when an SNMP Get request is received for this table.
 * If cell is found, the SNMP stack may call the corresponding get_value()
 * function below to retrieve the actual value contained in the cell.
 *
 * @param column           In:    Column index for the cell.
 * @param row_oid          In:    Row index (array) for the cell.
 * @param row_oid_len      In:    The number of elements in the row index array.
 * @param cell_instance    InOut: Cell instance (containing meta-data).
 * @return  SNMP_ERR_NOERROR if cell was found,
 *          SNMP_ERR_NOSUCHINSTANCE otherwise.
 */
static snmp_err_t lldpxpnoloctable_get_instance (
   const u32_t * column,
   const u32_t * row_oid,
   u8_t row_oid_len,
   struct snmp_node_instance * cell_instance)
{
   int port = rowindex_match_with_local_port (row_oid, row_oid_len);
   if (port == 0)
   {
      return SNMP_ERR_NOSUCHINSTANCE;
   }
   else
   {
      cell_instance->reference.s32 = port;
      return SNMP_ERR_NOERROR;
   }
}

/**
 * Get next cell in table lldpXPnoLocTable.
 *
 * Called when an SNMP GetNext request is received for this table.
 * If cell is found, the SNMP stack may call the corresponding get_value()
 * function below to retrieve the actual value contained in the cell.
 *
 * @param column           In:    Column index for the cell.
 * @param row_oid          InOut: Row index for the cell.
 * @param cell_instance    InOut: Cell instance (containing meta-data).
 * @return  SNMP_ERR_NOERROR if cell was found,
 *          SNMP_ERR_NOSUCHINSTANCE otherwise.
 */
static snmp_err_t lldpxpnoloctable_get_next_instance (
   const u32_t * column,
   struct snmp_obj_id * row_oid,
   struct snmp_node_instance * cell_instance)
{
   int port = rowindex_update_with_next_local_port (row_oid);
   if (port == 0)
   {
      return SNMP_ERR_NOSUCHINSTANCE;
   }
   else
   {
      cell_instance->reference.s32 = port;
      return SNMP_ERR_NOERROR;
   }
}

/**
 * Get value at cell in table lldpXPnoLocTable.
 *
 * Called when an SNMP Get or GetNext request is received for this table.
 * The cell was previously identified in a call to get_instance() or
 * get_next_instance().
 *
 * @param cell_instance    In:    Cell instance (containing meta-data).
 * @param value            Out:   Value to be returned in response.
 * @return  Size of returned value, in bytes.
 *          0 if error occurred.
 */
static s16_t lldpxpnoloctable_get_value (
   struct snmp_node_instance * cell_instance,
   void * value)
{
   s16_t value_len;
   int port = cell_instance->reference.s32;
   u32_t column =
      SNMP_TABLE_GET_COLUMN_FROM_OID (cell_instance->instance_oid.id);

   switch (column)
   {
   case 1:
   {
      /* lldpXPnoLocLPDValue */
      u32_t * v = (u32_t *)value;
      pf_snmp_signal_delay_t delays;

      pf_snmp_get_signal_delays (pnal_snmp.net, port, &delays);
      *v = delays.line_propagation_delay_ns;
      value_len = sizeof (u32_t);
   }
   break;
   case 2:
   {
      /* lldpXPnoLocPortTxDValue */
      u32_t * v = (u32_t *)value;
      pf_snmp_signal_delay_t delays;

      pf_snmp_get_signal_delays (pnal_snmp.net, port, &delays);
      *v = delays.port_tx_delay_ns;
      value_len = sizeof (u32_t);
   }
   break;
   case 3:
   {
      /* lldpXPnoLocPortRxDValue */
      u32_t * v = (u32_t *)value;
      pf_snmp_signal_delay_t delays;

      pf_snmp_get_signal_delays (pnal_snmp.net, port, &delays);
      *v = delays.port_rx_delay_ns;
      value_len = sizeof (u32_t);
   }
   break;
   case 6:
   {
      /* lldpXPnoLocPortNoS */
      u8_t * v = (u8_t *)value;
      pf_lldp_station_name_t station_name;

      pf_snmp_get_station_name (pnal_snmp.net, &station_name);
      value_len = station_name.len;
      if ((size_t)value_len > SNMP_MAX_VALUE_SIZE)
      {
         LOG_ERROR (
            PF_SNMP_LOG,
            "LLDP-EXT-PNO-MIB(%d): Value is too large: %" PRId16 "\n",
            __LINE__,
            value_len);
         value_len = 0;
      }
      else
      {
         memcpy (v, station_name.string, value_len);
      }
   }
   break;
   default:
   {
      LOG_ERROR (
         PF_SNMP_LOG,
         "LLDP-EXT-PNO-MIB(%d): Unknown table column: %" PRIu32 ".\n",
         __LINE__,
         column);
      value_len = 0;
   }
   break;
   }

   return value_len;
}

/* --- lldpXPnoRemoteData 1.0.8802.1.1.2.1.5.3791.1.3
 * ----------------------------------------------------- */

/**
 * Get cell in table lldpXPnoRemTable.
 *
 * Called when an SNMP Get request is received for this table.
 * If cell is found, the SNMP stack may call the corresponding get_value()
 * function below to retrieve the actual value contained in the cell.
 *
 * @param column           In:    Column index for the cell.
 * @param row_oid          In:    Row index (array) for the cell.
 * @param row_oid_len      In:    The number of elements in the row index array.
 * @param cell_instance    InOut: Cell instance (containing meta-data).
 * @return  SNMP_ERR_NOERROR if cell was found,
 *          SNMP_ERR_NOSUCHINSTANCE otherwise.
 */
static snmp_err_t lldpxpnoremtable_get_instance (
   const u32_t * column,
   const u32_t * row_oid,
   u8_t row_oid_len,
   struct snmp_node_instance * cell_instance)
{
   int port = rowindex_match_with_remote_device (row_oid, row_oid_len);
   if (port == 0)
   {
      return SNMP_ERR_NOSUCHINSTANCE;
   }
   else
   {
      cell_instance->reference.s32 = port;
      return SNMP_ERR_NOERROR;
   }
}

/**
 * Get next cell in table lldpXPnoRemTable.
 *
 * Called when an SNMP GetNext request is received for this table.
 * If cell is found, the SNMP stack may call the corresponding get_value()
 * function below to retrieve the actual value contained in the cell.
 *
 * @param column           In:    Column index for the cell.
 * @param row_oid          InOut: Row index for the cell.
 * @param cell_instance    InOut: Cell instance (containing meta-data).
 * @return  SNMP_ERR_NOERROR if cell was found,
 *          SNMP_ERR_NOSUCHINSTANCE otherwise.
 */
static snmp_err_t lldpxpnoremtable_get_next_instance (
   const u32_t * column,
   struct snmp_obj_id * row_oid,
   struct snmp_node_instance * cell_instance)
{
   int port = rowindex_update_with_next_remote_device (row_oid);
   if (port == 0)
   {
      return SNMP_ERR_NOSUCHINSTANCE;
   }
   else
   {
      cell_instance->reference.s32 = port;
      return SNMP_ERR_NOERROR;
   }
}

/**
 * Get value at cell in table lldpXPnoRemTable.
 *
 * Called when an SNMP Get or GetNext request is received for this table.
 * The cell was previously identified in a call to get_instance() or
 * get_next_instance().
 *
 * @param cell_instance    In:    Cell instance (containing meta-data).
 * @param value            Out:   Value to be returned in response.
 * @return  Size of returned value, in bytes.
 *          0 if error occurred.
 */
static s16_t lldpxpnoremtable_get_value (
   struct snmp_node_instance * cell_instance,
   void * value)
{
   s16_t value_len;
   int port = cell_instance->reference.s32;
   u32_t column =
      SNMP_TABLE_GET_COLUMN_FROM_OID (cell_instance->instance_oid.id);

   switch (column)
   {
   case 1:
   {
      /* lldpXPnoRemLPDValue */
      u32_t * v = (u32_t *)value;
      pf_snmp_signal_delay_t delays;
      int error;

      error = pf_snmp_get_peer_signal_delays (pnal_snmp.net, port, &delays);
      if (error)
      {
         value_len = 0;
      }
      else
      {
         value_len = sizeof (u32_t);
         *v = delays.line_propagation_delay_ns;
      }
   }
   break;
   case 2:
   {
      /* lldpXPnoRemPortTxDValue */
      u32_t * v = (u32_t *)value;
      pf_snmp_signal_delay_t delays;
      int error;

      error = pf_snmp_get_peer_signal_delays (pnal_snmp.net, port, &delays);
      if (error)
      {
         value_len = 0;
      }
      else
      {
         value_len = sizeof (u32_t);
         *v = delays.port_tx_delay_ns;
      }
   }
   break;
   case 3:
   {
      /* lldpXPnoRemPortRxDValue */
      u32_t * v = (u32_t *)value;
      pf_snmp_signal_delay_t delays;
      int error;

      error = pf_snmp_get_peer_signal_delays (pnal_snmp.net, port, &delays);
      if (error)
      {
         value_len = 0;
      }
      else
      {
         value_len = sizeof (u32_t);
         *v = delays.port_rx_delay_ns;
      }
   }
   break;
   case 6:
   {
      /* lldpXPnoRemPortNoS */
      u8_t * v = (u8_t *)value;
      pf_lldp_station_name_t station_name;
      int error;

      error =
         pf_snmp_get_peer_station_name (pnal_snmp.net, port, &station_name);
      value_len = station_name.len;
      if (error)
      {
         value_len = 0;
      }
      else if ((size_t)value_len > SNMP_MAX_VALUE_SIZE)
      {
         LOG_ERROR (
            PF_SNMP_LOG,
            "LLDP-EXT-PNO-MIB(%d): Value is too large: %" PRId16 "\n",
            __LINE__,
            value_len);
         value_len = 0;
      }
      else
      {
         memcpy (v, station_name.string, value_len);
      }
   }
   break;
   default:
   {
      LOG_ERROR (
         PF_SNMP_LOG,
         "LLDP-EXT-PNO-MIB(%d): Unknown table column: %" PRIu32 ".\n",
         __LINE__,
         column);
      value_len = 0;
   }
   break;
   }

   return value_len;
}

/* --- lldpXPnoObjects 1.0.8802.1.1.2.1.5.3791.1
 * ----------------------------------------------------- */
/* --- lldpXPnoMIB  ----------------------------------------------------- */
#endif /* LWIP_SNMP */
