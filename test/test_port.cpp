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

#include "utils_for_testing.h"
#include "mocks.h"

#include "pf_includes.h"

#include <gtest/gtest.h>

class PortTest : public PnetIntegrationTest
{
};

class PortUnitTest : public PnetUnitTest
{
};

TEST_F (PortUnitTest, PortGetPortList)
{
   pnet_t * dummystack = NULL;
   pf_lldp_port_list_t port_list;

   memset (&port_list, 0xff, sizeof (port_list));

   pf_port_get_list_of_ports (dummystack, 1, &port_list);
   EXPECT_EQ (port_list.ports[0], 0x80);
   EXPECT_EQ (port_list.ports[1], 0x00);

   pf_port_get_list_of_ports (dummystack, 2, &port_list);
   EXPECT_EQ (port_list.ports[0], 0xC0);
   EXPECT_EQ (port_list.ports[1], 0x00);

   pf_port_get_list_of_ports (dummystack, 4, &port_list);
   EXPECT_EQ (port_list.ports[0], 0xF0);
   EXPECT_EQ (port_list.ports[1], 0x00);

   pf_port_get_list_of_ports (dummystack, 8, &port_list);
   EXPECT_EQ (port_list.ports[0], 0xFF);
   EXPECT_EQ (port_list.ports[1], 0x00);

   pf_port_get_list_of_ports (dummystack, 9, &port_list);
   EXPECT_EQ (port_list.ports[0], 0xFF);
   EXPECT_EQ (port_list.ports[1], 0x80);

   pf_port_get_list_of_ports (dummystack, 12, &port_list);
   EXPECT_EQ (port_list.ports[0], 0xFF);
   EXPECT_EQ (port_list.ports[1], 0xF0);

   pf_port_get_list_of_ports (dummystack, 16, &port_list);
   EXPECT_EQ (port_list.ports[0], 0xFF);
   EXPECT_EQ (port_list.ports[1], 0xFF);
}

TEST_F (PortUnitTest, PortCheckIterator)
{
   pnet_t * dummystack = NULL;
   pf_port_iterator_t port_iterator;
   int port;
   int ix;

   /* Retrieve first port */
   pf_port_init_iterator_over_ports (dummystack, &port_iterator);
   port = pf_port_get_next (&port_iterator);
   EXPECT_EQ (port, 1);

   /* More ports might be available dependent on compile time setting */
   for (ix = 2; ix <= PNET_NUMBER_OF_PHYSICAL_PORTS; ix++)
   {
      port = pf_port_get_next (&port_iterator);
      EXPECT_EQ (port, ix);
   }

   /* Verify that we return 0 when we are done */
   port = pf_port_get_next (&port_iterator);
   EXPECT_EQ (port, 0);

   /* Do not restart automatically */
   port = pf_port_get_next (&port_iterator);
   EXPECT_EQ (port, 0);

   /* Restart the iterator */
   pf_port_init_iterator_over_ports (dummystack, &port_iterator);
   port = pf_port_get_next (&port_iterator);
   EXPECT_EQ (port, 1);
}

TEST_F (PortUnitTest, PortCheckIteratorRepeatCyclic)
{
   pnet_t * dummystack = NULL;
   pf_port_iterator_t port_iterator;
   int port;
   int ix;

   /* Retrieve first port */
   pf_port_init_iterator_over_ports (dummystack, &port_iterator);
   port = pf_port_get_next_repeat_cyclic (&port_iterator);
   EXPECT_EQ (port, 1);

   /* More ports might be available dependent on compile time setting */

   for (ix = 2; ix <= PNET_NUMBER_OF_PHYSICAL_PORTS; ix++)
   {
      port = pf_port_get_next_repeat_cyclic (&port_iterator);
      EXPECT_EQ (port, ix);
   }

   /* Verify that we return 1 again  */
   port = pf_port_get_next_repeat_cyclic (&port_iterator);
   EXPECT_EQ (port, 1);

   /* Continue to loop over available ports */
   for (ix = 2; ix <= PNET_NUMBER_OF_PHYSICAL_PORTS; ix++)
   {
      port = pf_port_get_next_repeat_cyclic (&port_iterator);
      EXPECT_EQ (port, ix);
   }

   /* Verify that we return 1 again  */
   port = pf_port_get_next_repeat_cyclic (&port_iterator);
   EXPECT_EQ (port, 1);

   /* Restart the iterator */
   pf_port_init_iterator_over_ports (dummystack, &port_iterator);
   port = pf_port_get_next_repeat_cyclic (&port_iterator);
   EXPECT_EQ (port, 1);
}

TEST_F (PortUnitTest, loc_port_num_to_dap_subslot)
{
   pnet_t * dummystack = NULL;
   int port;
   uint16_t sub_slot;
   pf_port_iterator_t port_iterator;

   pf_port_init_iterator_over_ports (dummystack, &port_iterator);
   port = pf_port_get_next (&port_iterator);

   while (port != 0)
   {
      sub_slot = pf_port_loc_port_num_to_dap_subslot (port);
      EXPECT_EQ (sub_slot, 0x8000 + port);
      port = pf_port_get_next (&port_iterator);
   }
}

TEST_F (PortUnitTest, dap_subslot_to_local_port)
{
   pnet_t * dummystack = NULL;
   int port;
   int local_port_num;
   pf_port_iterator_t port_iterator;

   pf_port_init_iterator_over_ports (dummystack, &port_iterator);
   port = pf_port_get_next (&port_iterator);

   while (port != 0)
   {
      local_port_num = pf_port_dap_subslot_to_local_port (0x8000 + port);
      EXPECT_EQ (local_port_num, port);
      port = pf_port_get_next (&port_iterator);
   }

   /* Invalid / not port sub slot  (low)*/
   local_port_num = pf_port_dap_subslot_to_local_port (0x8000);
   EXPECT_EQ (local_port_num, 0);

   /* Invalid / not port sub slot  (high)*/
   local_port_num = pf_port_dap_subslot_to_local_port (
      0x8000 + PNET_NUMBER_OF_PHYSICAL_PORTS + 1);
   EXPECT_EQ (local_port_num, 0);
}
