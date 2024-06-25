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

/*
 * Note: this file originally auto-generated by mib2c
 * using mib2c.iterate.conf
 */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#undef LOG_DEBUG
#undef LOG_WARNING
#undef LOG_INFO
#undef LOG_ERROR
#undef LOG_FATAL

#include "lldpRemTable.h"

/** Initializes the lldpRemTable module */
void init_lldpRemTable (pnet_t * pnet)
{
   /* here we initialize all the tables we're planning on supporting */
   initialize_table_lldpRemTable (pnet);
}

static void lldpRemTable_loop_free (
   void * loopctx,
   netsnmp_iterator_info * iinfo)
{
   SNMP_FREE (loopctx);
}

/** Initialize the lldpRemTable table by defining its contents and how it's
 * structured */
void initialize_table_lldpRemTable (pnet_t * pnet)
{
   const oid lldpRemTable_oid[] = {1, 0, 8802, 1, 1, 2, 1, 4, 1};
   const size_t lldpRemTable_oid_len = OID_LENGTH (lldpRemTable_oid);
   netsnmp_handler_registration * reg;
   netsnmp_iterator_info * iinfo;
   netsnmp_table_registration_info * table_info;

   reg = netsnmp_create_handler_registration (
      "lldpRemTable",
      lldpRemTable_handler,
      lldpRemTable_oid,
      lldpRemTable_oid_len,
      HANDLER_CAN_RONLY);

   reg->my_reg_void = pnet;

   table_info = SNMP_MALLOC_TYPEDEF (netsnmp_table_registration_info);
   netsnmp_table_helper_add_indexes (
      table_info,
      ASN_TIMETICKS, /* index: lldpRemTimeMark */
      ASN_INTEGER,   /* index: lldpRemLocalPortNum */
      ASN_INTEGER,   /* index: lldpRemIndex */
      0);
   table_info->min_column = COLUMN_LLDPREMCHASSISIDSUBTYPE;
   table_info->max_column = COLUMN_LLDPREMSYSCAPENABLED;

   iinfo = SNMP_MALLOC_TYPEDEF (netsnmp_iterator_info);
   iinfo->get_first_data_point = lldpRemTable_get_first_data_point;
   iinfo->get_next_data_point = lldpRemTable_get_next_data_point;
   iinfo->table_reginfo = table_info;

   iinfo->free_loop_context_at_end = lldpRemTable_loop_free;
   iinfo->myvoid = pnet;

   netsnmp_register_table_iterator (reg, iinfo);
}

netsnmp_variable_list * lldpRemTable_get_first_data_point (
   void ** my_loop_context,
   void ** my_data_context,
   netsnmp_variable_list * put_index_data,
   netsnmp_iterator_info * mydata)
{
   pnet_t * pnet = (pnet_t *)mydata->myvoid;
   pf_port_iterator_t * iterator;

   iterator = SNMP_MALLOC_TYPEDEF (pf_port_iterator_t);
   pf_snmp_init_port_iterator (pnet, iterator);
   *my_loop_context = iterator;

   return lldpRemTable_get_next_data_point (
      my_loop_context,
      my_data_context,
      put_index_data,
      mydata);
}

netsnmp_variable_list * lldpRemTable_get_next_data_point (
   void ** my_loop_context,
   void ** my_data_context,
   netsnmp_variable_list * put_index_data,
   netsnmp_iterator_info * mydata)
{
   netsnmp_variable_list * idx = put_index_data;
   pnet_t * pnet = (pnet_t *)mydata->myvoid;
   pf_port_iterator_t * iterator;
   int port;
   uint32_t timestamp;
   int error;

   iterator = (pf_port_iterator_t *)*my_loop_context;

   do
   {
      port = pf_snmp_get_next_port (iterator);
      if (port == 0)
      {
         return NULL;
      }

      error = pf_snmp_get_peer_timestamp (pnet, port, &timestamp);
   } while (error);

   snmp_set_var_typed_integer (idx, ASN_TIMETICKS, timestamp);
   idx = idx->next_variable;

   snmp_set_var_typed_integer (idx, ASN_INTEGER, port);
   idx = idx->next_variable;

   snmp_set_var_typed_integer (idx, ASN_INTEGER, port);

   *my_data_context = (void *)(uintptr_t)port;
   return put_index_data;
}

/** handles requests for the lldpRemTable table */
int lldpRemTable_handler (
   netsnmp_mib_handler * handler,
   netsnmp_handler_registration * reginfo,
   netsnmp_agent_request_info * reqinfo,
   netsnmp_request_info * requests)
{

   netsnmp_request_info * request;
   netsnmp_table_request_info * table_info;
   pnet_t * pnet = reginfo->my_reg_void;
   void * my_data_context;
   pf_lldp_chassis_id_t chassis_id;
   pf_lldp_port_id_t port_id;
   pf_lldp_port_description_t port_desc;
   int port;
   int error;

   switch (reqinfo->mode)
   {
      /*
       * Read-support (also covers GetNext requests)
       */
   case MODE_GET:
      for (request = requests; request; request = request->next)
      {
         my_data_context = netsnmp_extract_iterator_context (request);
         table_info = netsnmp_extract_table_info (request);

         LOG_DEBUG (
            PF_SNMP_LOG,
            "lldpRemTable(%d): GET. Column number: %u\n",
            __LINE__,
            table_info->colnum);

         switch (table_info->colnum)
         {
         case COLUMN_LLDPREMCHASSISIDSUBTYPE:
            if (my_data_context == NULL)
            {
               netsnmp_set_request_error (reqinfo, request, SNMP_NOSUCHINSTANCE);
               continue;
            }

            port = (int)(uintptr_t)my_data_context;
            error = pf_snmp_get_peer_chassis_id (pnet, port, &chassis_id);
            if (error)
            {
               netsnmp_set_request_error (reqinfo, request, SNMP_NOSUCHINSTANCE);
               continue;
            }

            snmp_set_var_typed_integer (
               request->requestvb,
               ASN_INTEGER,
               chassis_id.subtype);
            break;
         case COLUMN_LLDPREMCHASSISID:
            if (my_data_context == NULL)
            {
               netsnmp_set_request_error (reqinfo, request, SNMP_NOSUCHINSTANCE);
               continue;
            }

            port = (int)(uintptr_t)my_data_context;
            error = pf_snmp_get_peer_chassis_id (pnet, port, &chassis_id);
            if (error)
            {
               netsnmp_set_request_error (reqinfo, request, SNMP_NOSUCHINSTANCE);
               continue;
            }

            snmp_set_var_typed_value (
               request->requestvb,
               ASN_OCTET_STR,
               chassis_id.string,
               chassis_id.len);
            break;
         case COLUMN_LLDPREMPORTIDSUBTYPE:
            if (my_data_context == NULL)
            {
               netsnmp_set_request_error (reqinfo, request, SNMP_NOSUCHINSTANCE);
               continue;
            }

            port = (int)(uintptr_t)my_data_context;
            error = pf_snmp_get_peer_port_id (pnet, port, &port_id);
            if (error)
            {
               netsnmp_set_request_error (reqinfo, request, SNMP_NOSUCHINSTANCE);
               continue;
            }

            snmp_set_var_typed_integer (
               request->requestvb,
               ASN_INTEGER,
               port_id.subtype);
            break;
         case COLUMN_LLDPREMPORTID:
            if (my_data_context == NULL)
            {
               netsnmp_set_request_error (reqinfo, request, SNMP_NOSUCHINSTANCE);
               continue;
            }

            port = (int)(uintptr_t)my_data_context;
            error = pf_snmp_get_peer_port_id (pnet, port, &port_id);
            if (error)
            {
               netsnmp_set_request_error (reqinfo, request, SNMP_NOSUCHINSTANCE);
               continue;
            }

            snmp_set_var_typed_value (
               request->requestvb,
               ASN_OCTET_STR,
               port_id.string,
               port_id.len);
            break;
         case COLUMN_LLDPREMPORTDESC:
            if (my_data_context == NULL)
            {
               netsnmp_set_request_error (reqinfo, request, SNMP_NOSUCHINSTANCE);
               continue;
            }

            port = (int)(uintptr_t)my_data_context;
            error = pf_snmp_get_peer_port_description (pnet, port, &port_desc);
            if (error)
            {
               netsnmp_set_request_error (reqinfo, request, SNMP_NOSUCHINSTANCE);
               continue;
            }

            snmp_set_var_typed_value (
               request->requestvb,
               ASN_OCTET_STR,
               port_desc.string,
               port_desc.len);
            break;
         default:
            netsnmp_set_request_error (reqinfo, request, SNMP_NOSUCHOBJECT);
            break;
         }
      }
      break;
   default:
      LOG_DEBUG (
         PF_SNMP_LOG,
         "lldpRemTable(%d): Unknown mode: %u\n",
         __LINE__,
         reqinfo->mode);
      break;
   }
   return SNMP_ERR_NOERROR;
}