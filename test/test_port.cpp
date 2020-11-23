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

TEST_F (PortTest, PortGetPortList)
{
   pf_lldp_port_list_t port_list;

   memset (&port_list, 0xff, sizeof (port_list));

   /* TODO: Add support for multiple ports */
   pf_port_get_list_of_ports (net, &port_list);
   EXPECT_EQ (port_list.ports[0], 0x80);
   EXPECT_EQ (port_list.ports[1], 0x00);
}

TEST_F (PortTest, PortCheckIterator)
{
   pf_port_iterator_t port_iterator;
   int port;
   int ix;

   /* Retrieve first port */
   pf_port_init_iterator_over_ports (net, &port_iterator);
   port = pf_port_get_next (&port_iterator);
   EXPECT_EQ (port, 1);

   /* More ports might be available dependent on compile time setting */
   for(ix = 2; ix <= PNET_MAX_PORT; ix++)
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
   pf_port_init_iterator_over_ports (net, &port_iterator);
   port = pf_port_get_next (&port_iterator);
   EXPECT_EQ (port, 1);
}
