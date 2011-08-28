#ifndef __HID_LG_MX5500_RECEIVER
#define __HID_LG_MX5500_RECEIVER

#include <linux/hid.h>

#include "hid-lg-device.h"

struct lg_mx5500_receiver {
	u8 initialized;
	u8 max_devices;

	struct lg_device *device;

	struct lg_mx5500_keyboard *keyboard;
	struct lg_mx_revolution *mouse;
};

int lg_mx5500_receiver_init(struct lg_device *device);

void lg_mx5500_receiver_exit(struct lg_device *device);

#endif