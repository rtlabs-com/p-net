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

#ifndef PF_SRAM_UIO_H
#define PF_SRAM_UIO_H

#ifdef __cplusplus
extern "C" {
#endif

#define PF_SRAM_INVALID_ADDRESS 0

/**
 * Init SRAM UIO driver.
 * The SRAM driver provides access to LAN9662 SRAM used by the RTE.
 * The driver implements a frame concept for managing the SRAM
 * as frames mapping to a inbound or outbound RTE configurations.
 * @return 0 on success, - 1 on error error
 */
int pf_sram_init (void);

/**
 * Allocate SRAM frame
 * @return Frame start address, PF_SRAM_INVALID_ADDRESS on error
 */
uint16_t pf_sram_frame_alloc (void);

/**
 * Free SRAM frame
 * @param Frame start address
 */
void pf_sram_frame_free (uint16_t frame_start_address);

/**
 * Write SRAM
 * @param address    In: SRAM address to write
 * @param data       In: Data
 * @param length     In: Length of data
 * @return 0 on success, -1 on error
 */
int pf_sram_write (uint16_t address, const uint8_t * data, uint16_t length);

/**
 * Read SRAM
 * @param address    In:  SRAM address to read
 * @param data       Out: Data
 * @param length     In:  Length of data
 * @return 0 on success, -1 on error
 */
int pf_sram_read (uint16_t address, uint8_t * data, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif /* PF_PORT_H */
