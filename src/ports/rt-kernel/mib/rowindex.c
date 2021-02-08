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

#include "rowindex.h"

#include "options.h"
#include "pnet_api.h"
#include "osal.h"
#include "osal_log.h"
#include "pnal.h"
#include "pf_types.h"
#include "pf_snmp.h"
#include "pnal_snmp.h"

#include <string.h>

static void rowindex_construct_for_local_port (
   struct snmp_obj_id * row_index,
   int port)
{
   row_index->id[0] = port; /* lldpLocPortNum */
   row_index->len = 1;
}

static void rowindex_construct_for_local_interface (
   struct snmp_obj_id * row_index)
{
   pf_snmp_management_address_t address;
   size_t i;

   pf_snmp_get_management_address (pnal_snmp.net, &address);
   row_index->id[0] = address.subtype; /* lldpLocManAddrSubtype */
   for (i = 0; i < address.len; i++)
   {
      row_index->id[1 + i] = address.value[i]; /* lldpLocManAddr */
   }
   row_index->len = 1 + address.len;
}

static int rowindex_construct_for_remote_device (
   struct snmp_obj_id * row_index,
   int port)
{
   uint32_t timestamp;
   int error;

   error = pf_snmp_get_peer_timestamp (pnal_snmp.net, port, &timestamp);
   if (error)
   {
      return error;
   }

   row_index->id[0] = timestamp; /* lldpRemTimeMark */
   row_index->id[1] = port;      /* lldpRemLocalPortNum */
   row_index->id[2] = port;      /* lldpRemIndex */
   row_index->len = 3;

   return error;
}

static int rowindex_construct_for_remote_interface (
   struct snmp_obj_id * row_index,
   int port)
{
   uint32_t timestamp;
   pf_snmp_management_address_t address;
   int error;
   size_t i;

   error = pf_snmp_get_peer_timestamp (pnal_snmp.net, port, &timestamp);
   if (error)
   {
      return error;
   }
   error = pf_snmp_get_peer_management_address (pnal_snmp.net, port, &address);
   if (error)
   {
      return error;
   }

   row_index->id[0] = timestamp;       /* lldpRemTimeMark */
   row_index->id[1] = port;            /* lldpRemLocalPortNum */
   row_index->id[2] = port;            /* lldpRemIndex */
   row_index->id[3] = address.subtype; /* lldpRemManAddrSubtype */
   for (i = 0; i < address.len; i++)
   {
      row_index->id[4 + i] = address.value[i]; /* lldpRemManAddr */
   }
   row_index->len = 4 + address.len;

   return error;
}

int rowindex_match_with_local_port (const u32_t * row_oid, u8_t row_oid_len)
{
   pf_port_iterator_t port_iterator;
   int port;

   pf_snmp_init_port_iterator (pnal_snmp.net, &port_iterator);
   port = pf_snmp_get_next_port (&port_iterator);
   while (port != 0)
   {
      struct snmp_obj_id port_oid;

      rowindex_construct_for_local_port (&port_oid, port);
      if (snmp_oid_equal (row_oid, row_oid_len, port_oid.id, port_oid.len))
      {
         return port;
      }

      port = pf_snmp_get_next_port (&port_iterator);
   }

   return 0;
}

int rowindex_update_with_next_local_port (struct snmp_obj_id * row_oid)
{
   struct snmp_next_oid_state state;
   u32_t next_oid[SNMP_MAX_OBJ_ID_LEN];
   pf_port_iterator_t port_iterator;
   int port;

   snmp_next_oid_init (
      &state,
      row_oid->id,
      row_oid->len,
      next_oid,
      SNMP_MAX_OBJ_ID_LEN);
   pf_snmp_init_port_iterator (pnal_snmp.net, &port_iterator);
   port = pf_snmp_get_next_port (&port_iterator);
   while (port != 0)
   {
      struct snmp_obj_id port_oid;

      rowindex_construct_for_local_port (&port_oid, port);
      snmp_next_oid_check (&state, port_oid.id, port_oid.len, (void *)port);

      port = pf_snmp_get_next_port (&port_iterator);
   }

   if (state.status == SNMP_NEXT_OID_STATUS_SUCCESS)
   {
      snmp_oid_assign (row_oid, state.next_oid, state.next_oid_len);
      port = (int)state.reference;
   }
   else
   {
      port = 0;
   }

   return port;
}

int rowindex_match_with_local_interface (const u32_t * row_oid, u8_t row_oid_len)
{
   struct snmp_obj_id interface_oid;
   int interface;

   rowindex_construct_for_local_interface (&interface_oid);
   if (snmp_oid_equal (row_oid, row_oid_len, interface_oid.id, interface_oid.len))
   {
      interface = 1;
   }
   else
   {
      interface = 0;
   }

   return interface;
}

snmp_err_t rowindex_update_with_next_local_interface (
   struct snmp_obj_id * row_oid)
{
   struct snmp_next_oid_state state;
   u32_t next_oid[SNMP_MAX_OBJ_ID_LEN];
   struct snmp_obj_id interface_oid;
   int interface;

   snmp_next_oid_init (
      &state,
      row_oid->id,
      row_oid->len,
      next_oid,
      SNMP_MAX_OBJ_ID_LEN);
   rowindex_construct_for_local_interface (&interface_oid);
   snmp_next_oid_check (&state, interface_oid.id, interface_oid.len, NULL);

   if (state.status == SNMP_NEXT_OID_STATUS_SUCCESS)
   {
      snmp_oid_assign (row_oid, state.next_oid, state.next_oid_len);
      interface = 1;
   }
   else
   {
      interface = 0;
   }

   return interface;
}

int rowindex_match_with_remote_device (const u32_t * row_oid, u8_t row_oid_len)
{
   pf_port_iterator_t port_iterator;
   int port;

   pf_snmp_init_port_iterator (pnal_snmp.net, &port_iterator);
   port = pf_snmp_get_next_port (&port_iterator);
   while (port != 0)
   {
      struct snmp_obj_id port_oid;
      int error;

      error = rowindex_construct_for_remote_device (&port_oid, port);
      if (
         !error &&
         snmp_oid_equal (row_oid, row_oid_len, port_oid.id, port_oid.len))
      {
         return port;
      }

      port = pf_snmp_get_next_port (&port_iterator);
   }

   return 0;
}

int rowindex_update_with_next_remote_device (struct snmp_obj_id * row_oid)
{
   struct snmp_next_oid_state state;
   u32_t next_oid[SNMP_MAX_OBJ_ID_LEN];
   pf_port_iterator_t port_iterator;
   int port;

   snmp_next_oid_init (
      &state,
      row_oid->id,
      row_oid->len,
      next_oid,
      SNMP_MAX_OBJ_ID_LEN);
   pf_snmp_init_port_iterator (pnal_snmp.net, &port_iterator);
   port = pf_snmp_get_next_port (&port_iterator);
   while (port != 0)
   {
      int error;
      struct snmp_obj_id port_oid;

      error = rowindex_construct_for_remote_device (&port_oid, port);
      if (!error)
      {
         snmp_next_oid_check (&state, port_oid.id, port_oid.len, (void *)port);
      }

      port = pf_snmp_get_next_port (&port_iterator);
   }

   if (state.status == SNMP_NEXT_OID_STATUS_SUCCESS)
   {
      snmp_oid_assign (row_oid, state.next_oid, state.next_oid_len);
      port = (int)state.reference;
   }
   else
   {
      port = 0;
   }

   return port;
}

int rowindex_match_with_remote_interface (
   const u32_t * row_oid,
   u8_t row_oid_len)
{
   pf_port_iterator_t port_iterator;
   int port;

   pf_snmp_init_port_iterator (pnal_snmp.net, &port_iterator);
   port = pf_snmp_get_next_port (&port_iterator);
   while (port != 0)
   {
      int error;
      struct snmp_obj_id port_oid;

      error = rowindex_construct_for_remote_interface (&port_oid, port);
      if (
         !error &&
         snmp_oid_equal (row_oid, row_oid_len, port_oid.id, port_oid.len))
      {
         return port;
      }

      port = pf_snmp_get_next_port (&port_iterator);
   }

   return 0;
}

int rowindex_update_with_next_remote_interface (struct snmp_obj_id * row_oid)
{
   struct snmp_next_oid_state state;
   u32_t next_oid[SNMP_MAX_OBJ_ID_LEN];
   pf_port_iterator_t port_iterator;
   int port;

   snmp_next_oid_init (
      &state,
      row_oid->id,
      row_oid->len,
      next_oid,
      SNMP_MAX_OBJ_ID_LEN);
   pf_snmp_init_port_iterator (pnal_snmp.net, &port_iterator);
   port = pf_snmp_get_next_port (&port_iterator);
   while (port != 0)
   {
      int error;
      struct snmp_obj_id port_oid;

      error = rowindex_construct_for_remote_interface (&port_oid, port);
      if (!error)
      {
         snmp_next_oid_check (&state, port_oid.id, port_oid.len, (void *)port);
      }

      port = pf_snmp_get_next_port (&port_iterator);
   }

   if (state.status == SNMP_NEXT_OID_STATUS_SUCCESS)
   {
      snmp_oid_assign (row_oid, state.next_oid, state.next_oid_len);
      port = (int)state.reference;
   }
   else
   {
      port = 0;
   }

   return port;
}
