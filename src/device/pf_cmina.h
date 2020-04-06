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

#ifndef PF_CMINA_H
#define PF_CMINA_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Initialize the CMINA component.
 * @param net              InOut: The p-net stack instance
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmina_init(
   pnet_t                  *net);

/**
 * Show the CMINA status.
 * @param net              InOut: The p-net stack instance
 */
void pf_cmina_show(
   pnet_t                  *net);

/**
 * Retrieve the configured station name of the device.
 * @param net              InOut: The p-net stack instance
 * @param pp_station_name  Out:  The station name.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmina_get_station_name(
   pnet_t                  *net,
   const char              **pp_station_name);

/**
 * Retrieve the configured IP address of the device.
 * @param net              InOut: The p-net stack instance
 * @param p_ipaddr         Out:  The ip_address.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmina_get_ipaddr(
   pnet_t                  *net,
   os_ipaddr_t             *p_ipaddr);

/**
 * Handle the DCP set command.
 *
 * Triggers the \a pnet_reset_ind() callback for some incoming DCP set commands.
 *
 * @param net              InOut: The p-net stack instance
 * @param opt              In:   The option key.
 * @param sub              In:   The sub-option key.
 * @param block_qualifier  In:   The block qualifier.
 * @param value_length     In:   The length of the data to set.
 * @param p_value          In:   The data to set.
 * @param p_block_error    Out:  The block error code, if any.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmina_dcp_set_ind(
   pnet_t                  *net,
   uint8_t                 opt,
   uint8_t                 sub,
   uint16_t                block_qualifier,
   uint16_t                value_length,
   uint8_t                 *p_value,
   uint8_t                 *p_block_error);

/**
 * Commit changes to the IP-suite.
 *
 * This shall be done _after_ the answer to DCP set has been sent.
 * @param net              InOut: The p-net stack instance
 */
void pf_cmina_dcp_set_commit(
   pnet_t                  *net);

/**
 * Handle the DCP get command.
 * @param net              InOut: The p-net stack instance
 * @param opt              In:   The option key.
 * @param sub              In:   The sub-option key.
 * @param value_length     In:   The length of the pp_value buffer.
 *                         Out:  The length of the read data.
 * @param pp_value         Out:  The retrieved data.
 * @param p_block_error    Out:  The block error code, if any.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmina_dcp_get_req(
   pnet_t                  *net,
   uint8_t                 opt,
   uint8_t                 sub,
   uint16_t                *p_value_length,
   uint8_t                 **pp_value,
   uint8_t                 *p_block_error);

#ifdef __cplusplus
}
#endif

#endif /* PF_CMINA_H */
