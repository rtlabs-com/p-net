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

#include "sampleapp_common.h"
#include <gpio.h>
#include <bsp.h>

/************************* Utilities ******************************************/

void app_set_led (uint16_t id, bool led_state)
{
   if (id == APP_DATA_LED_ID)
   {
      gpio_set (GPIO_LED1, led_state ? 1 : 0); /* "LED1" on circuit board */
   }
   else if (id == APP_PROFINET_SIGNAL_LED_ID)
   {
      gpio_set (GPIO_LED2, led_state ? 1 : 0); /* "LED2" on circuit board */
   }
}

bool app_get_button (uint16_t id)
{
   if (id == 0)
   {
      return (gpio_get (GPIO_BUTTON1) == 0);
   }
   else if (id == 1)
   {
      return (gpio_get (GPIO_BUTTON2) == 0);
   }
   return false;
}
