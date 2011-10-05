#ifndef __HID_LG_MX_REVOLUTION
#define __HID_LG_MX_REVOLUTION

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <linux/hid.h>

#include "hid-lg-device.h"
#include "hid-lg-mx5500-receiver.h"

enum lg_mx5500_scrollmode {
	LG_MX5500_SCROLLMODE_FREESPIN_IMMEDIATE = 0x01,
	LG_MX5500_SCROLLMODE_CLICK_CLICK_IMMEDIATE = 0x02,
	LG_MX5500_SCROLLMODE_FREESPIN_MOVE = 0x03,
	LG_MX5500_SCROLLMODE_CLICK_CLICK_MOVE = 0x04,
	LG_MX5500_SCROLLMODE_AUTOMATIC = 0x05,
	LG_MX5500_SCROLLMODE_BUTTON_SWITCH = 0x07,
	LG_MX5500_SCROLLMODE_BUTTON_TOGGLE = 0x08,
};

struct lg_driver *lg_mx_revolution_get_driver(void);

#endif