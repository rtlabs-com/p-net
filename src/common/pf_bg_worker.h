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

#ifndef PF_BG_WORKER_H
#define PF_BG_WORKER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Background worker jobs
 */
typedef enum pf_bg_job
{
   /** Update status for all ports  */
   PF_BGJOB_UPDATE_PORTS_STATUS = 0,

   /** Save non volatile ASE data to file */
   PF_BGJOB_SAVE_ASE_NVM_DATA,

   /** Save non volatile IM data to file */
   PF_BGJOB_SAVE_IM_NVM_DATA,

   /** Save non volatile PDPort data to file */
   PF_BGJOB_SAVE_PDPORT_NVM_DATA,
} pf_bg_job_t;

/**
 * Initialize the background worker.
 *
 * @param net              InOut: The p-net stack instance
 */
void pf_bg_worker_init (pnet_t * net);

/**
 * Start a background job.
 * This function is non-blocking and sends the job request
 * to the background worker task.
 * @param net              InOut: The p-net stack instance
 * @param job_id           In:    Job to run
 * @return  0  if operation is successfully initiated
 *          -1 if an error occurred.
 */
int pf_bg_worker_start_job (pnet_t * net, pf_bg_job_t job_id);

#ifdef __cplusplus
}
#endif

#endif /* PF_BG_WORKER_H */
