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

#ifndef PF_UIO_H
#define PF_UIO_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Init RTE UIO driver
 * The RTE driver provides access to RTE registers from user space.
 * The register read and write operations are used by the MERA library.
 * @return 0 on success, - 1 on error error
 */
int pf_rte_uio_init (void);

/**
 * Read RTE register
 * @param inst    In: MERA library instance
 * @param addr    In: Register address
 * @param data    In: Register value
 * @return 0 on success, - 1 on error error
 */
int pf_rte_uio_reg_read (
   struct mera_inst * inst,
   const uintptr_t addr,
   uint32_t * data);

/**
 * Write RTE register
 * @param inst    In: MERA library instance
 * @param addr    In: Register address
 * @param data    InOut: Register value
 * @return 0 on success, - 1 on error error
 */
int pf_rte_uio_reg_write (
   struct mera_inst * inst,
   const uintptr_t addr,
   const uint32_t data);

#ifdef __cplusplus
}
#endif

#endif /* PF_RTE_UIO_H */
