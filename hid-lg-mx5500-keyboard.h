#ifndef __HID_LG_MX5500_KEYBOARD
#define __HID_LG_MX5500_KEYBOARD

#include <linux/hid.h>

#include "hid-lg-mx5500-receiver.h"

struct lg_mx5500_keyboard;

void lg_mx5500_keyboard_handle(struct lg_mx5500 *device, const u8 *buffer,
								size_t count);

int lg_mx5500_keyboard_init(struct lg_mx5500 *device);

void lg_mx5500_keyboard_exit(struct lg_mx5500 *device);

struct lg_mx5500_keyboard *lg_mx5500_keyboard_init_on_receiver(
			struct lg_mx5500 *device,
			const u8 *buffer, size_t count);

void lg_mx5500_keyboard_exit_on_receiver(struct lg_mx5500_keyboard *keyboard);

#endif