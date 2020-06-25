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

/**
 * @file
 * @brief Send LLDP frame on startup.
 */

#ifndef PF_LLDP_H
#define PF_LLDP_H

#ifdef __cplusplus
extern "C"
{
#endif

#define LLDP_TYPE_END                     	0
#define LLDP_TYPE_CHASSIS_ID              	1
#define LLDP_TYPE_PORT_ID                 	2
#define LLDP_TYPE_TTL                     	3
#define LLDP_TYPE_MANAGEMENT              	8
#define LLDP_TYPE_ORG_SPEC                	127

#define LLDP_SUBTYPE_CHASSIS_ID_MAC      	4
#define LLDP_SUBTYPE_CHASSIS_ID_NAME      	7
#define LLDP_SUBTYPE_PORT_ID_LOCAL        	7

#define LLDP_IEEE_SUBTYPE_MAC_PHY         	1

#define LLDP_PROFIBUS_CODE	{0x00,0x0e,0xcf}
#define LLDP_PROFIBUS_SUBTYPE_DELAY_VALUES	0x01
#define LLDP_PROFIBUS_SUBTYPE_PORT_STATUS	0x02
#define LLDP_PROFIBUS_SUBTYPE_CHASSIS_MAC	0x05

#define LLDP_TYPE_MASK		0xFE00
#define LLDP_TYPE_SHIFT		0x9
#define LLDP_LENGTH_MASK	0x1FF
#define LLDP_MAX_TLV		512

#define MAKE_UINT16(a, b)	((uint16_t)(((a) & 0xff) | ((uint16_t)(((b) << 8) & 0xff00))))
#define GET_UINT16( ptr )	(uint16_t) (MAKE_UINT16((*(uint8_t*)(ptr)),(*(uint8_t*)(ptr+1))))

typedef struct lldp_frame
{
	uint8_t type;
	uint8_t len;
	uint8_t value[LLDP_MAX_TLV];
}LLDP_FRAME;

/**
 * Initialize the LLDP component.
 *
 * This function initializes the LLDP component and
 * sends the initial LLDP message.
 * @param net               InOut: The p-net stack instance
 */
void pf_lldp_init(
   pnet_t                  *net);

/**
 * Build and send an LLDP message.
 * @param net              InOut: The p-net stack instance
 */
void pf_lldp_send(
   pnet_t                  *net);

/**
 * Recieve an LLDP message.
 * @param net              InOut: The p-net stack instance
 */
void pf_lldp_recv(
   pnet_t                  *net,
   os_buf_t                *p_frame_buf,
   uint16_t	   				frame_pos);

#ifdef __cplusplus
}
#endif

#endif /* PF_LLDP_H */
