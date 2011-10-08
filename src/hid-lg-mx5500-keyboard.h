#ifndef __HID_LG_MX5500_KEYBOARD
#define __HID_LG_MX5500_KEYBOARD

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <linux/hid.h>

#include "hid-lg-device.h"

struct lg_driver *lg_mx5500_keyboard_get_driver(void);

#endif