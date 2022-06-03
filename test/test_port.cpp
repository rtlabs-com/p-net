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
 protected:
   pnet_t dummystack = {};
};

TEST_F (PortUnitTest, PortGetPortList)
{
   pf_lldp_port_list_t port_list;

   memset (&port_list, 0xff, sizeof (port_list));

   dummystack.fspm_cfg.num_physical_ports = 1;
   pf_port_get_list_of_ports (&dummystack, &port_list);
   EXPECT_EQ (port_list.ports[0], 0x80);
   EXPECT_EQ (port_list.ports[1], 0x00);

   dummystack.fspm_cfg.num_physical_ports = 2;
   pf_port_get_list_of_ports (&dummystack, &port_list);
   EXPECT_EQ (port_list.ports[0], 0xC0);
   EXPECT_EQ (port_list.ports[1], 0x00);

   dummystack.fspm_cfg.num_physical_ports = 4;
   pf_port_get_list_of_ports (&dummystack, &port_list);
   EXPECT_EQ (port_list.ports[0], 0xF0);
   EXPECT_EQ (port_list.ports[1], 0x00);

   dummystack.fspm_cfg.num_physical_ports = 8;
   pf_port_get_list_of_ports (&dummystack, &port_list);
   EXPECT_EQ (port_list.ports[0], 0xFF);
   EXPECT_EQ (port_list.ports[1], 0x00);

   dummystack.fspm_cfg.num_physical_ports = 9;
   pf_port_get_list_of_ports (&dummystack, &port_list);
   EXPECT_EQ (port_list.ports[0], 0xFF);
   EXPECT_EQ (port_list.ports[1], 0x80);

   dummystack.fspm_cfg.num_physical_ports = 12;
   pf_port_get_list_of_ports (&dummystack, &port_list);
   EXPECT_EQ (port_list.ports[0], 0xFF);
   EXPECT_EQ (port_list.ports[1], 0xF0);

   dummystack.fspm_cfg.num_physical_ports = 16;
   pf_port_get_list_of_ports (&dummystack, &port_list);
   EXPECT_EQ (port_list.ports[0], 0xFF);
   EXPECT_EQ (port_list.ports[1], 0xFF);
}

TEST_F (PortUnitTest, PortCheckIterator)
{
   pf_port_iterator_t port_iterator;

   dummystack.fspm_cfg.num_physical_ports = 6;

   pf_port_init_iterator_over_ports (&dummystack, &port_iterator);

   EXPECT_EQ (pf_port_get_next (&port_iterator), 1);
   EXPECT_EQ (pf_port_get_next (&port_iterator), 2);
   EXPECT_EQ (pf_port_get_next (&port_iterator), 3);
   EXPECT_EQ (pf_port_get_next (&port_iterator), 4);
   EXPECT_EQ (pf_port_get_next (&port_iterator), 5);
   EXPECT_EQ (pf_port_get_next (&port_iterator), 6);

   /* Verify that we return 0 when we are done, and no auto restart */
   EXPECT_EQ (pf_port_get_next (&port_iterator), 0);
   EXPECT_EQ (pf_port_get_next (&port_iterator), 0);
   EXPECT_EQ (pf_port_get_next (&port_iterator), 0);

   /* Verify behaviour if the iterator for unknown reason has
    * value that is out of port range */
   port_iterator.next_port = 10;
   EXPECT_EQ (pf_port_get_next (&port_iterator), 0);
   EXPECT_EQ (pf_port_get_next (&port_iterator), 0);

   /* Restart the iterator */
   pf_port_init_iterator_over_ports (&dummystack, &port_iterator);

   EXPECT_EQ (pf_port_get_next (&port_iterator), 1);
   EXPECT_EQ (pf_port_get_next (&port_iterator), 2);
   EXPECT_EQ (pf_port_get_next (&port_iterator), 3);
   EXPECT_EQ (pf_port_get_next (&port_iterator), 4);
   EXPECT_EQ (pf_port_get_next (&port_iterator), 5);
   EXPECT_EQ (pf_port_get_next (&port_iterator), 6);

   /* Different number of ports */
   dummystack.fspm_cfg.num_physical_ports = 1;
   pf_port_init_iterator_over_ports (&dummystack, &port_iterator);

   EXPECT_EQ (pf_port_get_next (&port_iterator), 1);
   EXPECT_EQ (pf_port_get_next (&port_iterator), 0);
   EXPECT_EQ (pf_port_get_next (&port_iterator), 0);
}

TEST_F (PortUnitTest, PortCheckIteratorRepeatCyclic)
{
   pf_port_iterator_t port_iterator;

   dummystack.fspm_cfg.num_physical_ports = 6;

   pf_port_init_iterator_over_ports (&dummystack, &port_iterator);

   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 1);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 2);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 3);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 4);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 5);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 6);

   /* Verify auto restart */
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 1);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 2);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 3);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 4);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 5);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 6);

   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 1);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 2);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 3);

   /* Verify behaviour if the iterator for unknown reason has
    * value that is out of port range */
   port_iterator.next_port = 10;
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 1);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 2);

   /* Restart the iterator from first port */
   pf_port_init_iterator_over_ports (&dummystack, &port_iterator);

   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 1);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 2);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 3);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 4);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 5);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 6);

   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 1);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 2);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 3);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 4);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 5);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 6);

   /* Different number of ports */
   dummystack.fspm_cfg.num_physical_ports = 1;
   pf_port_init_iterator_over_ports (&dummystack, &port_iterator);

   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 1);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 1);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 1);
   EXPECT_EQ (pf_port_get_next_repeat_cyclic (&port_iterator), 1);
}

TEST_F (PortUnitTest, PortNumToSubslot)
{
   dummystack.fspm_cfg.num_physical_ports = 16;

   EXPECT_EQ (pf_port_loc_port_num_to_dap_subslot (&dummystack, 1), 0x8001);
   EXPECT_EQ (pf_port_loc_port_num_to_dap_subslot (&dummystack, 2), 0x8002);
   EXPECT_EQ (pf_port_loc_port_num_to_dap_subslot (&dummystack, 3), 0x8003);
   EXPECT_EQ (pf_port_loc_port_num_to_dap_subslot (&dummystack, 4), 0x8004);
   EXPECT_EQ (pf_port_loc_port_num_to_dap_subslot (&dummystack, 5), 0x8005);
   EXPECT_EQ (pf_port_loc_port_num_to_dap_subslot (&dummystack, 6), 0x8006);
   EXPECT_EQ (pf_port_loc_port_num_to_dap_subslot (&dummystack, 7), 0x8007);
   EXPECT_EQ (pf_port_loc_port_num_to_dap_subslot (&dummystack, 8), 0x8008);
   EXPECT_EQ (pf_port_loc_port_num_to_dap_subslot (&dummystack, 9), 0x8009);
   EXPECT_EQ (pf_port_loc_port_num_to_dap_subslot (&dummystack, 10), 0x800A);
   EXPECT_EQ (pf_port_loc_port_num_to_dap_subslot (&dummystack, 11), 0x800B);
   EXPECT_EQ (pf_port_loc_port_num_to_dap_subslot (&dummystack, 12), 0x800C);
   EXPECT_EQ (pf_port_loc_port_num_to_dap_subslot (&dummystack, 13), 0x800D);
   EXPECT_EQ (pf_port_loc_port_num_to_dap_subslot (&dummystack, 14), 0x800E);
   EXPECT_EQ (pf_port_loc_port_num_to_dap_subslot (&dummystack, 15), 0x800F);
   EXPECT_EQ (pf_port_loc_port_num_to_dap_subslot (&dummystack, 16), 0x8010);
}

TEST_F (PortUnitTest, PortSubslotToLocalPort)
{
   dummystack.fspm_cfg.num_physical_ports = 4;

   EXPECT_EQ (pf_port_dap_subslot_to_local_port (&dummystack, 0x8001), 1);
   EXPECT_EQ (pf_port_dap_subslot_to_local_port (&dummystack, 0x8002), 2);
   EXPECT_EQ (pf_port_dap_subslot_to_local_port (&dummystack, 0x8003), 3);
   EXPECT_EQ (pf_port_dap_subslot_to_local_port (&dummystack, 0x8004), 4);

   /* Out of range */
   EXPECT_EQ (pf_port_dap_subslot_to_local_port (&dummystack, 0x8000), 0);
   EXPECT_EQ (pf_port_dap_subslot_to_local_port (&dummystack, 0x8005), 0);
}

TEST_F (PortUnitTest, PortIsSubslotDap)
{
   dummystack.fspm_cfg.num_physical_ports = 4;

   EXPECT_EQ (pf_port_subslot_is_dap_port_id (&dummystack, 0x8001), true);
   EXPECT_EQ (pf_port_subslot_is_dap_port_id (&dummystack, 0x8002), true);
   EXPECT_EQ (pf_port_subslot_is_dap_port_id (&dummystack, 0x8003), true);
   EXPECT_EQ (pf_port_subslot_is_dap_port_id (&dummystack, 0x8004), true);

   /* Out of range */
   EXPECT_EQ (pf_port_subslot_is_dap_port_id (&dummystack, 0x8000), false);
   EXPECT_EQ (pf_port_subslot_is_dap_port_id (&dummystack, 0x8005), false);
}

TEST_F (PortUnitTest, PortIsValid)
{
   dummystack.fspm_cfg.num_physical_ports = 4;

   EXPECT_EQ (pf_port_is_valid (&dummystack, 1), true);
   EXPECT_EQ (pf_port_is_valid (&dummystack, 2), true);
   EXPECT_EQ (pf_port_is_valid (&dummystack, 3), true);
   EXPECT_EQ (pf_port_is_valid (&dummystack, 4), true);

   /* Out of range */
   EXPECT_EQ (pf_port_is_valid (&dummystack, 0), false);
   EXPECT_EQ (pf_port_is_valid (&dummystack, 5), false);
}

TEST_F (PortUnitTest, PortGetNumberOfPorts)
{
   dummystack.fspm_cfg.num_physical_ports = 4;

   EXPECT_EQ (pf_port_get_number_of_ports (&dummystack), 4);
}
