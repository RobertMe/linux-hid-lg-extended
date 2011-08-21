#ifndef __HID_LG_MX5500_MOUSE
#define __HID_LG_MX5500_MOUSE

#include <linux/hid.h>

#include "hid-lg-mx5500-receiver.h"

struct lg_mx5500_mouse;

void lg_mx5500_mouse_handle(struct lg_mx5500 *device, const u8 *buffer,
								size_t count);

int lg_mx5500_mouse_init(struct lg_mx5500 *device);

void lg_mx5500_mouse_exit(struct lg_mx5500 *device);

struct lg_mx5500_mouse *lg_mx5500_mouse_init_on_receiver(
			struct lg_mx5500 *device,
			const u8 *buffer, size_t count);

void lg_mx5500_mouse_exit_on_receiver(struct lg_mx5500_mouse *mouse);

#endif