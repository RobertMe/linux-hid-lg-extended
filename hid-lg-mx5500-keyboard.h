#ifndef __HID_LG_MX5500_KEYBOARD
#define __HID_LG_MX5500_KEYBOARD

#include <linux/hid.h>

#include "hid-lg-mx5500-receiver.h"

struct lg_mx5500_keyboard;

void lg_mx5500_keyboard_handle(struct lg_mx5500 *device, const u8 *buffer,
								size_t count);

struct lg_mx5500_keyboard *lg_mx5500_keyboard_create_on_receiver(
			struct lg_mx5500 *device,
			const u8 *buffer, size_t count);

void lg_mx5500_keyboard_destroy(struct lg_mx5500_keyboard *keyboard);

#endif