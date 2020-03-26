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

#ifdef UNIT_TEST
#define os_eth_init mock_os_eth_init
#endif

#include <string.h>
#include "pf_includes.h"


/*
 * Keep this value small as it define the number of entries in the
 * frame id map and all entries are probed until a match is found.
 *
 * Each input CR may have 2 frameIds (for RTC3)
 * Add space for DCP:     0xfefc..0xfeff.
 * Add space for alarms:  0xfc01, 0xfe01.
 */
#define PF_ETH_MAX_MAP     ((PNET_MAX_API) * (PNET_MAX_AR) * (PNET_MAX_CR) * 2 + 4 + 2)

typedef struct pf_eth_frame_id_map
{
   bool                    in_use;
   uint16_t                frame_id;
   pf_eth_frame_handler_t  frame_handler;
   void                    *p_arg;
} pf_eth_frame_id_map_t;

/*
 * The frame id map is used to quickly find the function responsible for
 * handling a frame with a specific frame id.
 * Clients may add or remove entries on the fly, but there is no locking of the table.
 * ToDo: This may be a problem in the future.
 * Note that frames may arrive at any time.
 */
static pf_eth_frame_id_map_t  id_map[PF_ETH_MAX_MAP];

int pf_eth_init(void)
{
   int ret = 0;

   memset(id_map, 0, sizeof(id_map));

   return ret;
}

int pf_eth_recv(
   os_buf_t                *p_buf,
   size_t                  len)
{
   int         ret = 0;       /* Means: "Not handled" */
   uint16_t    type_pos = 2*sizeof(pnet_ethaddr_t);
   uint16_t    type;
   uint16_t    frame_id;
   uint16_t    *p_data;
   uint16_t    ix = 0;

   /* Skip ALL VLAN tags */
   p_data = (uint16_t *)(&((uint8_t *)p_buf->payload)[type_pos]);
   type = ntohs(p_data[0]);
   while (type == OS_ETHTYPE_VLAN)
   {
      type_pos += 4;    /* Sizeof VLAN tag */

      p_data = (uint16_t *)(&((uint8_t *)p_buf->payload)[type_pos]);
      type = ntohs(p_data[0]);
   }
   frame_id = ntohs(p_data[1]);

   switch (type)
   {
   case OS_ETHTYPE_PROFINET:
      /* Find the associated frame handler */
      ix = 0;
      while ((ix < NELEMENTS(id_map)) &&
             ((id_map[ix].in_use == false) ||
              (id_map[ix].frame_id != frame_id)))
      {
         ix++;
      }
      if (ix < NELEMENTS(id_map))
      {
         ret = id_map[ix].frame_handler(frame_id, p_buf,
            type_pos + sizeof(uint16_t), id_map[ix].p_arg);
      }
      break;
   case OS_ETHTYPE_LLDP:
      /* ToDo: */
      break;
   default:
      /* Not a profinet packet. */
      ret = 0;
      break;
   }

   return ret;
}

void pf_eth_frame_id_map_add(
   uint16_t                frame_id,
   pf_eth_frame_handler_t  frame_handler,
   void                    *p_arg)
{
   uint16_t                ix = 0;

   while ((ix < NELEMENTS(id_map)) &&
          (id_map[ix].in_use == true))
   {
      ix++;
   }

   if (ix < NELEMENTS(id_map))
   {
      LOG_DEBUG(PF_ETH_LOG, "ETH(%d): Add FrameIds %#x at index %u\n", __LINE__,
         (unsigned)frame_id, (unsigned)ix);
      id_map[ix].frame_id = frame_id;
      id_map[ix].frame_handler = frame_handler;
      id_map[ix].p_arg = p_arg;
      id_map[ix].in_use = true;
   }
   else
   {
      LOG_ERROR(PF_ETH_LOG, "ETH(%d): No more room for FrameIds\n", __LINE__);
   }
}

void pf_eth_frame_id_map_remove(
   uint16_t                frame_id)
{
   uint16_t                ix = 0;

   while ((ix < NELEMENTS(id_map)) &&
          ((id_map[ix].in_use == false) ||
           (id_map[ix].frame_id != frame_id)))
   {
      ix++;
   }

   if (ix < NELEMENTS(id_map))
   {
      id_map[ix].in_use = false;
      LOG_DEBUG(PF_ETH_LOG, "ETH(%d): Free room for FrameIds %#x at index %u\n", __LINE__,
         (unsigned)frame_id, (unsigned)ix);
   }
}
