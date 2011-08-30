#ifndef __HID_LG_MX_REVOLUTION
#define __HID_LG_MX_REVOLUTION

#include <linux/hid.h>

#include "hid-lg-device.h"
#include "hid-lg-mx5500-receiver.h"

struct lg_mx_revolution;

enum lg_mx5500_scrollmode {
	LG_MX5500_SCROLLMODE_FREESPIN_IMMEDIATE = 0x01,
	LG_MX5500_SCROLLMODE_CLICK_CLICK_IMMEDIATE = 0x02,
	LG_MX5500_SCROLLMODE_FREESPIN_MOVE = 0x03,
	LG_MX5500_SCROLLMODE_CLICK_CLICK_MOVE = 0x04,
	LG_MX5500_SCROLLMODE_AUTOMATIC = 0x05,
	LG_MX5500_SCROLLMODE_BUTTON_SWITCH = 0x07,
	LG_MX5500_SCROLLMODE_BUTTON_TOGGLE = 0x08,
};

void lg_mx_revolution_handle(struct lg_device *device, const u8 *buffer,
								size_t count);

int lg_mx_revolution_init(struct lg_device *device);

void lg_mx_revolution_exit(struct lg_device *device);

struct lg_mx_revolution *lg_mx_revolution_init_on_receiver(
			struct lg_device *device,
			const u8 *buffer, size_t count);

void lg_mx_revolution_exit_on_receiver(struct lg_mx_revolution *mouse);

struct lg_driver *lg_mx_revolution_get_driver(void);

#endif