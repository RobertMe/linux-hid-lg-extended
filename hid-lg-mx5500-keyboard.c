#include "hid-lg-mx5500-keyboard.h"

struct lg_mx5500_keyboard {
	struct lg_mx5500 *device;
	u8 devnum;
	u8 initialized;
};

struct lg_mx5500_keyboard *lg_mx5500_keyboard_create_on_receiver(
			struct lg_mx5500 *device,
			const u8 *buffer, size_t count)
{
	struct lg_mx5500_keyboard *keyboard;

	keyboard = kzalloc(sizeof(*keyboard), GFP_KERNEL);
	if(!keyboard)
		return NULL;

	keyboard->device = device;
	keyboard->devnum = buffer[1];
	keyboard->initialized = 0;

	return keyboard;
}

void lg_mx5500_keyboard_destroy(struct lg_mx5500_keyboard *keyboard)
{
	if(keyboard == NULL)
		return;

	kfree(keyboard);
}