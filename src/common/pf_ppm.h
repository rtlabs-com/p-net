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

#ifndef PF_PPM_H
#define PF_PPM_H

#ifdef __cplusplus
extern "C"
{
#endif


/**
 * Initialize the PPM component.
 * @param net              InOut: The p-net stack instance
 */
void pf_ppm_init(
   pnet_t                  *net);

/**
 * Instantiate and start a PPM instance.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param crep             In:   The IOCR index.
 * @return  0  if the PPM instance was activated.
 *          -1 if an error occurred.
 */
int pf_ppm_activate_req(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                crep);

/**
 * Close and de-commission a PPM instance.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param crep             In:   The IOCR index.
 * @return  0  if the PPM instance was closed.
 *          -1 if an error occurred.
 */
int pf_ppm_close_req(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                crep);

/**
 * Set the data and IOPS for a sub-module.
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:   The API id.
 * @param slot_nbr         In:   The slot number.
 * @param subslot_nbr      In:   The sub-slot number.
 * @param p_data           In:   The application data.
 * @param data_len         In:   The length of the application data.
 * @param iops             In:   The IOPS of the application data.
 * @param iops_len         In:   The length of the IOPS.
 * @return  0  if the input data and IOPS was set.
 *          -1 if an error occurred.
 */
int pf_ppm_set_data_and_iops(
   pnet_t                  *net,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   uint8_t                 *p_data,
   uint16_t                data_len,
   uint8_t                 *p_iops,
   uint8_t                 iops_len);

/**
 * Set IOCS for a sub-module.
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:   The API id.
 * @param slot_nbr         In:   The slot number.
 * @param subslot_nbr      In:   The sub-slot number.
 * @param p_iocs           In:   The IOCS of the application data.
 * @param iocs_len         In:   The length of the IOCS data.
 * @return  0  if the IOCS was set.
 *          -1 if an error occurred.
 */
int pf_ppm_set_iocs(
   pnet_t                  *net,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   uint8_t                 *p_iocs,
   uint8_t                 iocs_len);

/**
 * Retrieve the data and IOPS for a sub-module.
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:   The API id.
 * @param slot_nbr         In:   The slot number.
 * @param subslot_nbr      In:   The sub-slot number.
 * @param p_data           In:   The application data.
 * @param p_data_len       In:   The length of the p_data buffer.
 *                         Out:  Actual length of the sub-module data.
 * @param p_iops           In:   The IOPS of the application data.
 * @param p_iops_len       In:   Size of buffer at p_iops.
 *                         Out:  Actual length of IOPS data.
 * @return  0  if the input data and IOPS could e retrieved.
 *          -1 if an error occurred.
 */
int pf_ppm_get_data_and_iops(
   pnet_t                  *net,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   uint8_t                 *p_data,
   uint16_t                *p_data_len,
   uint8_t                 *p_iops,
   uint8_t                 *p_iops_len);

/**
 * Retrieve IOCS for a sub-module.
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:   The API id.
 * @param slot_nbr         In:   The slot number.
 * @param subslot_nbr      In:   The sub-slot number.
 * @param p_iocs           Out:  The IOCS of the application data.
 * @param p_iocs_len       In:   Size of buffer at p_iocs.
 *                         Out:  Actual length of IOCS data.
 * @return  0  if the IOCS could be retrieved.
 *          -1 if an error occurred.
 */
int pf_ppm_get_iocs(
   pnet_t                  *net,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   uint8_t                 *p_iocs,
   uint8_t                 *p_iocs_len);

/**
 * Implements the "Local Set State" primitive.
 * @param p_ar             In:   The AR instance.
 * @param crep             In:   The IOCR instance.
 * @param primary          In:   true if the state is "primary".
 * @return  0  if the state was set.
 *          -1 if an error occurred.
 */
int pf_ppm_set_data_status_state(
   pf_ar_t                 *p_ar,
   uint32_t                crep,
   bool                    primary);

/**
 * Implements the "Local Set Redundancy State" primitive.
 * @param p_ar             In:   The AR instance.
 * @param crep             In:   The IOCR instance.
 * @param redundant        In:   true if the state is "redundant".
 * @return  0  if the redundancy state was set.
 *          -1 if an error occurred.
 */
int pf_ppm_set_data_status_redundancy(
   pf_ar_t                 *p_ar,
   uint32_t                crep,
   bool                    redundant);

/**
 * Implements the "Local set Provider State" primitive.
 * @param p_ar             In:   The AR instance.
 * @param crep             In:   The IOCR instance.
 * @param run              In:   true if the application is "running".
 * @return  0  if the provider status was set.
 *          -1 if an error occurred.
 */
int pf_ppm_set_data_status_provider(
   pf_ar_t                 *p_ar,
   uint32_t                crep,
   bool                    run);

/**
 * Get the data status of the PPM connection.
 * @param p_ppm            In:   The PPM instance.
 * @param p_data_status    Out:  The PPM data status.
 * @return
 */
int pf_ppm_get_data_status(
   pf_ppm_t                *p_ppm,
   uint8_t                 *p_data_status);

/**
 * Set/Reset the station problem indicator which is inclued in all data messages.
 *
 * |param p_ar                In:   The AR instance.
 * @param problem_indicator   In:   The problem indicator.
 */
void pf_ppm_set_problem_indicator(
   pf_ar_t                 *p_ar,
   bool                    problem_indicator);

/**
 * Show information about a PPM instance.
 * @param p_ppm            In:   The PPM instance.
 */
void pf_ppm_show(
   pf_ppm_t                *p_ppm);

#ifdef __cplusplus
}
#endif

#endif /* PF_PPM_H */
