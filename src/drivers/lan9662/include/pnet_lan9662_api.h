/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2021 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#ifndef PF_LAN9662_API_H
#define PF_LAN9662_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include <pnet_api.h>

typedef enum
{
   PNET_MERA_DATA_TYPE_SRAM,
   PNET_MERA_DATA_TYPE_QSPI
} pnet_mera_rte_data_type_t;

/**
 * Submodule RTE data configuration.
 * Used for both inputs and outputs configuration.
 * The default value is used if the IOXS is bad.
 */
typedef struct pnet_mera_rte_data_cfg
{
   pnet_mera_rte_data_type_t type;
   uint16_t address;
   const uint8_t * default_data;
} pnet_mera_rte_data_cfg_t;

/**
 * Configure RTE input for a single subslot.
 * This operation is used to configure a QSPI data source.
 * Called during the AR establishment.
 *
 * @param net              InOut: The p-net stack instance
 * @param api              In:    The API.
 * @param slot             In:    The slot.
 * @param subslot          In:    The sub-slot.
 * @param rte_data_cfg     In:    RTE configuration
 * @return  0  if the RTE is successfully configured.
 *          -1 if an error occurred. For example if HW resources are not
 *             available.
 */
PNET_EXPORT int pnet_mera_input_set_rte_config (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   const pnet_mera_rte_data_cfg_t * rte_data_cfg);

/**
 * Configure RTE output for a single subslot.
 * This operation is used to configure a QSPI data sink.
 * Called during the AR establishment.
 *
 * @param net              InOut: The p-net stack instance
 * @param api              In:    The API.
 * @param slot             In:    The slot.
 * @param subslot          In:    The sub-slot.
 * @param rte_data_cfg     In:    RTE configuration
 * @return  0  if the RTE is successfully configured.
 *          -1 if an error occurred. For example if HW resources are not
 *             available.
 */
PNET_EXPORT int pnet_mera_output_set_rte_config (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   const pnet_mera_rte_data_cfg_t * rte_data_cfg);

#ifdef __cplusplus
}
#endif

#endif /* PF_LAN9662_API_H */
