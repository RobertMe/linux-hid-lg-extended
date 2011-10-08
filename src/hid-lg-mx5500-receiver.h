#ifndef __HID_LG_MX5500_RECEIVER
#define __HID_LG_MX5500_RECEIVER

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <linux/hid.h>

#include "hid-lg-device.h"

#define LG_MX5500_RECEIVER_MAX_DEVICES 0x03

void lg_mx5500_receiver_exit(struct lg_device *device);

void lg_mx5500_receiver_hid_receive(struct lg_device *device, const u8 *buffer,
								size_t count);

struct lg_driver *lg_mx5500_receiver_get_driver(void);

#endif