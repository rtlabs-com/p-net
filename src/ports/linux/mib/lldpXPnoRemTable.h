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
#ifndef LLDPXPNOREMTABLE_H
#define LLDPXPNOREMTABLE_H

#include "pf_includes.h"

/* function declarations */
void init_lldpXPnoRemTable (pnet_t * pnet);
void initialize_table_lldpXPnoRemTable (pnet_t * pnet);
Netsnmp_Node_Handler lldpXPnoRemTable_handler;
Netsnmp_First_Data_Point lldpXPnoRemTable_get_first_data_point;
Netsnmp_Next_Data_Point lldpXPnoRemTable_get_next_data_point;

/* column number definitions for table lldpXPnoRemTable */
#define COLUMN_LLDPXPNOREMLPDVALUE                        1
#define COLUMN_LLDPXPNOREMPORTTXDVALUE                    2
#define COLUMN_LLDPXPNOREMPORTRXDVALUE                    3
#define COLUMN_LLDPXPNOREMPORTSTATUSRT2                   4
#define COLUMN_LLDPXPNOREMPORTSTATUSRT3                   5
#define COLUMN_LLDPXPNOREMPORTNOS                         6
#define COLUMN_LLDPXPNOREMPORTMRPUUID                     7
#define COLUMN_LLDPXPNOREMPORTMRRTSTATUS                  8
#define COLUMN_LLDPXPNOREMPORTPTCPMASTER                  9
#define COLUMN_LLDPXPNOREMPORTPTCPSUBDOMAINUUID           10
#define COLUMN_LLDPXPNOREMPORTPTCPIRDATAUUID              11
#define COLUMN_LLDPXPNOREMPORTMODERT3                     12
#define COLUMN_LLDPXPNOREMPORTPERIODLENGTH                13
#define COLUMN_LLDPXPNOREMPORTPERIODVALIDITY              14
#define COLUMN_LLDPXPNOREMPORTREDOFFSET                   15
#define COLUMN_LLDPXPNOREMPORTREDVALIDITY                 16
#define COLUMN_LLDPXPNOREMPORTORANGEOFFSET                17
#define COLUMN_LLDPXPNOREMPORTORANGEVALIDITY              18
#define COLUMN_LLDPXPNOREMPORTGREENOFFSET                 19
#define COLUMN_LLDPXPNOREMPORTGREENVALIDITY               20
#define COLUMN_LLDPXPNOREMPORTSTATUSRT3PREAMBLESHORTENING 21
#define COLUMN_LLDPXPNOREMPORTSTATUSRT3FRAGMENTATION      22
#define COLUMN_LLDPXPNOREMPORTOPERMAUTYPEEXTENSION        23
#define COLUMN_LLDPXPNOREMPORTMRPICROLE                   24
#define COLUMN_LLDPXPNOREMPORTMRPICDOMAINID               25
#endif /* LLDPXPNOREMTABLE_H */
