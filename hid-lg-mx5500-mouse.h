#ifndef __HID_LG_MX5500_MOUSE
#define __HID_LG_MX5500_MOUSE

#include <linux/hid.h>

#include "hid-lg-mx5500-receiver.h"

struct lg_mx5500_mouse;

void lg_mx5500_mouse_handle(struct lg_mx5500 *device, const u8 *buffer,
								size_t count);

struct lg_mx5500_mouse *lg_mx5500_mouse_create_on_receiver(
			struct lg_mx5500 *device,
			const u8 *buffer, size_t count);

void lg_mx5500_mouse_destroy(struct lg_mx5500_mouse *mouse);

#endif