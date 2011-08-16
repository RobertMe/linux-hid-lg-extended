#include "hid-lg-mx5500-mouse.h"

struct lg_mx5500_mouse {
	struct lg_mx5500 *device;
	u8 devnum;
	u8 initialized;
};

struct lg_mx5500_mouse *lg_mx5500_mouse_create_on_receiver(
			struct lg_mx5500 *device,
			const u8 *buffer, size_t count)
{
	struct lg_mx5500_mouse *mouse;

	mouse = kzalloc(sizeof(*mouse), GFP_KERNEL);
	if(!mouse)
		return NULL;

	mouse->device = device;
	mouse->devnum = buffer[1];
	mouse->initialized = 0;

	return mouse;
}

void lg_mx5500_mouse_destroy(struct lg_mx5500_mouse *mouse)
{
	if(mouse == NULL)
		return;

	kfree(mouse);
}