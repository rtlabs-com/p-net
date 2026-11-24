/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2025 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#ifdef UNIT_TEST
#endif

#include <string.h>
#include "pf_includes.h"
#include "pf_cim.h"

/**
 * @file
 * @brief Implements the Communication Interface Management (CIM).
 *
 * Currently only contains functions to read CIM Electric PowerReal.
 */

void pf_cim_get_device_data (
   pnet_t * net,
   uint16_t slot_number,
   uint16_t subslot_number,
   pf_electric_power_device_data_t * device_data)
{
   int res;
   device_data->device_voltage.voltage = 0;
   device_data->device_voltage.type = 0;

   if (net->fspm_cfg.ep_device_cb)
   {
      res = net->fspm_cfg.ep_device_cb (
         net,
         net->fspm_cfg.cb_arg,
         slot_number,
         subslot_number,
         &device_data->device_voltage.voltage,
         &device_data->device_voltage.type);
      if (res != 0)
      {
         /* Invalidate value even if something was written */
         device_data->device_voltage.type = 0;
      }
   }
}

void pf_cim_get_port_data (
   pnet_t * net,
   uint16_t slot_number,
   uint16_t subslot_number,
   uint16_t num_ports,
   pf_electric_power_port_data_t * port_entries)
{
   uint16_t i;
   int res;
   if (net->fspm_cfg.ep_port_cb == NULL)
   {
      return;
   }

   for (i = 0; i < num_ports; ++i)
   {
      res = net->fspm_cfg.ep_port_cb (
         net,
         net->fspm_cfg.cb_arg,
         slot_number,
         subslot_number,
         i,
         &port_entries[i].port_voltage.voltage,
         &port_entries[i].port_voltage.type,
         &port_entries[i].port_current.current,
         &port_entries[i].port_current.current_limit);

      if (res != 0)
      {
         /* Mark as invalid */
         port_entries[i].port_voltage.type = 0;
      }
   }
}

int pf_cim_write_request (
   pnet_t * net,
   const pf_iod_write_request_t * p_write_req,
   const uint8_t * p_bytes,
   uint16_t data_length,
   pnet_result_t * p_result)
{
   int ret = -1;

   switch (p_write_req->index)
   {
#if PNET_OPTION_SNMP && PNET_OPTION_SECURITY_LEVEL_1
   case PF_IDX_CIM_SNMP_ADJUST:
      LOG_DEBUG (
         PF_SNMP_LOG,
         "SNMP(%d): Write request to CIM SNMP Adjust slot = %d, subslot = %d\n",
         __LINE__,
         p_write_req->slot_number,
         p_write_req->subslot_number);

      if (
         p_write_req->slot_number == PNET_SLOT_DAP_IDENT &&
         p_write_req->subslot_number == PNET_SUBMOD_DAP_INTERFACE_1_IDENT)
      {
         ret = pf_cim_snmp_adjust (
            net,
            p_write_req,
            p_bytes,
            data_length,
            p_result);
      }
      break;
#endif /* PNET_OPTION_SNMP && PNET_OPTION_SECURITY_LEVEL_1 */
   default:
      break;
   }
   return ret;
}
