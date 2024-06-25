/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2021 rt-labs AB, Sweden.
 * Copyright 2023 NXP
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#include "sampleapp_common.h"
#include "fsl_gpio.h"

/************************* Utilities ******************************************/

void app_set_led (uint16_t pin, bool led_state)
{
   if (pin == APP_DATA_LED_ID)
      GPIO_WritePinOutput(GPIO5, 13, led_state ? true : false);
   else if (pin == APP_PROFINET_SIGNAL_LED_ID) 
      GPIO_WritePinOutput(GPIO5, 10, led_state ? true : false);
}

bool app_get_button (uint16_t id)
{
   if (id == 0)
   {
      return (bool)GPIO_ReadPinInput(GPIO5, 12);
   }
   else if (id == 1)
   {
      // No more buttons on i.MX 8M Mini 
   }
   return false;
}

