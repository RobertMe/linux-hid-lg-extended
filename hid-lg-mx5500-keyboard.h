#ifndef __HID_LG_MX5500_KEYBOARD
#define __HID_LG_MX5500_KEYBOARD

#include <linux/hid.h>

#include "hid-lg-device.h"
#include "hid-lg-mx5500-receiver.h"

struct lg_mx5500_keyboard {
	struct lg_device device;
	wait_queue_head_t received;
	u8 devnum;
	u8 initialized;
	struct attribute_group attr_group;

	short battery_level;
	short lcd_page;
	short time[3];
	short date[3];
};

struct lg_driver *lg_mx5500_keyboard_get_driver(void);

#endif