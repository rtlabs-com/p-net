/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2025 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#include "utils_for_testing.h"
#include "mocks.h"

#include "pf_block_writer.h"
#include "pf_includes.h"

#include <gtest/gtest.h>

class CIMTest : public PnetIntegrationTest
{
};

TEST_F (CIMTest, ElectricPowerDataRealHeader)
{
   /* Generate header */
   constexpr uint16_t num_ports = PNET_MAX_PHYSICAL_PORTS;
   pf_electric_power_device_data_t ep_device = {};
   pf_electric_power_port_data_t ep_ports[num_ports] = {};
   uint8_t p_res[64];
   uint16_t * p_res16 = (uint16_t *)p_res;
   uint32_t * p_res32 = (uint32_t *)p_res;
   uint16_t p_pos = 0;
   pf_put_info_t put_info =
   {
      .result = PF_PUT_OK,
      .p_frame = NULL,
      .p_array = p_res,
      .len = sizeof(p_res),
   };
   uint16_t slot_number = 0x30;
   uint16_t subslot_number = 0x03;
   uint16_t i;

   pf_cim_get_device_data (
      net,
      slot_number,
      subslot_number,
      &ep_device);
   pf_cim_get_port_data (
      net,
      slot_number,
      subslot_number,
      num_ports,
      ep_ports);

   pf_put_cim_electric_power_real (
      true,
      &ep_device,
      num_ports,
      ep_ports,
      slot_number,
      subslot_number,
      &put_info,
      &p_pos);

   /* Verify header according to specification: */
   /* (BlockHeader, NumberOfEntries, [ElectricPowerPortData *],
    *  ElectricPowerDeviceData') */

   /* BlockHeader */
   EXPECT_EQ (p_pos, 6 + 2 + (12 * num_ports) + 8);
   EXPECT_EQ (CC_FROM_BE16 (p_res16[0]), PF_BT_CIM_ELECTRIC_POWER_REAL);
   EXPECT_EQ (CC_FROM_BE16 (p_res16[1]), p_pos - 4);
   EXPECT_EQ (p_res[4], PNET_BLOCK_VERSION_HIGH);
   EXPECT_EQ (p_res[5], PNET_BLOCK_VERSION_LOW);

   /* NumberOfEntries */
   EXPECT_EQ (CC_FROM_BE16(p_res16[3]), num_ports);

   /** ElectricPowerPortData **/
   for (i = 0; i < num_ports; ++i)
   {
      /* CIMStationElementID */
      EXPECT_EQ (CC_FROM_BE16 (p_res16[4 + i * 6]), slot_number);
      EXPECT_EQ (CC_FROM_BE16 (p_res16[5 + i * 6]), subslot_number);

      /* Port Voltage */
      EXPECT_EQ (
         CC_FROM_BE32 (p_res32[3 + i * 3]),
         PNET_EP_PORT_TYPE_POWER_LOAD << 28 |
            (TEST_POWER_PORT_VOLTAGE_BASE + i));
      /* Port Current */
      EXPECT_EQ (
         CC_FROM_BE32 (p_res32[4 + i * 3]),
         (TEST_POWER_PORT_CURRENT_LIMIT_BASE + i) << 16 |
            (TEST_POWER_PORT_CURRENT_BASE + i));
   }

   /** ElecticPowerDeviceData **/
   /* CIMStationElementID */
      EXPECT_EQ (CC_FROM_BE16(p_res16[4 + num_ports * 6]), slot_number);
      EXPECT_EQ (CC_FROM_BE16(p_res16[5 + num_ports * 6]), subslot_number);
   /* Device Voltage */
   EXPECT_EQ (
         CC_FROM_BE32 (p_res32[3 + num_ports * 3]),
         PNET_EP_DEVICE_TYPE_PO_DATA_LINE << 28 | TEST_POWER_DEVICE_VOLTAGE);
}
