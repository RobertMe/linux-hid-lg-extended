#include <linux/hid.h>
#include <asm/atomic.h>

#include "hid-lg-mx5500.h"
#include "hid-lg-mx5500-keyboard.h"
#include "hid-lg-mx5500-mouse.h"
#include "hid-lg-mx5500-receiver.h"

struct lg_mx5500_receiver_handler {
	u8 action;
	u8 first;
	void (*func)(struct lg_mx5500_receiver *receiver, const u8 *payload, size_t size);
};

static int lg_mx5500_receiver_update_max_devices(struct lg_mx5500_receiver *receiver,
							const u8 *buffer, size_t count)
{
	if(count < 6) {
		lg_mx5500_err(receiver->device, "Too few bytes to set maximum number of devices");
		return 1;
	}
	receiver->max_devices = buffer[5];
	lg_mx5500_dbg(receiver->device, "Maximum number of devices changed to 0x%02x",
			receiver->max_devices);

	return 0;
}

static void lg_mx5500_receiver_set_max_devices(struct lg_mx5500_receiver *receiver,
						u8 max_devices)
{
	u8 cmd[7] = { 0x10, 0xFF, LG_MX5500_ACTION_SET, 0x00, 0x00, 0x00, 0x00 };

	cmd[5] = max_devices;
	lg_mx5500_queue_out(receiver->device, cmd, sizeof(cmd));

	if(receiver->initialized)
		return;

	receiver->initialized = 1;
}

static void lg_mx5500_receiver_logon_device(struct lg_mx5500_receiver *receiver,
						const u8 *buffer, size_t count)
{
	switch(buffer[1]) {
	case 0x01:
		receiver->keyboard = lg_mx5500_keyboard_create_on_receiver(
					receiver->device, buffer, count);
		break;
	case 0x02:
		receiver->mouse = lg_mx5500_mouse_create_on_receiver(
					receiver->device, buffer, count);
		break;
	}
}

static void lg_mx5500_receiver_logoff_device(struct lg_mx5500_receiver *receiver,
						const u8 *buffer, size_t count)
{
	switch(buffer[1]) {
	case 0x01:
		if(receiver->keyboard) {
			lg_mx5500_keyboard_destroy(receiver->keyboard);
			receiver->keyboard = NULL;
		}
		break;
	case 0x02:
		if(receiver->mouse) {
			lg_mx5500_mouse_destroy(receiver->mouse);
			receiver->mouse = NULL;
		}
		break;
	}
}

static void lg_mx5500_receiver_devices_logon(struct lg_mx5500_receiver *receiver)
{
	u8 cmd[7] = { 0x10, 0xFF, LG_MX5500_ACTION_SET, 0x02, 0x02, 0x00, 0x00 };

	lg_mx5500_queue_out(receiver->device, cmd, sizeof(cmd));
}

static void lg_mx5500_receiver_handle_get_max_devices(struct lg_mx5500_receiver *receiver,
								const u8 *buffer, size_t count)
{
	if(lg_mx5500_receiver_update_max_devices(receiver, buffer, count))
		return;

	if(receiver->initialized)
		return;

	if(receiver->max_devices != 0x03)
		lg_mx5500_receiver_set_max_devices(receiver, 0x03);
	else
		lg_mx5500_receiver_devices_logon(receiver);
}

static void lg_mx5500_receiver_handle_set_max_devices(struct lg_mx5500_receiver *receiver,
								const u8 *buffer, size_t count)
{
	if(lg_mx5500_receiver_update_max_devices(receiver, buffer, count))
		return;

	if(receiver->initialized)
		return;

	lg_mx5500_receiver_devices_logon(receiver);
}

static struct lg_mx5500_receiver_handler lg_mx5500_receiver_handlers[] = {
	{ .action = LG_MX5500_ACTION_GET, .first = 0x00,
		.func = lg_mx5500_receiver_handle_get_max_devices },
	{ .action = LG_MX5500_ACTION_SET, .first = 0x00,
		.func = lg_mx5500_receiver_handle_set_max_devices },
	{ }
};

void lg_mx5500_receiver_handle(struct lg_mx5500 *device, const u8 *buffer,
								size_t count)
{
	int i;
	int handeld = 0;
	struct lg_mx5500_receiver *receiver;
	struct lg_mx5500_receiver_handler *handler;

	receiver = lg_mx5500_get_data(device);

	for(i = 0; lg_mx5500_receiver_handlers[i].action ||
		lg_mx5500_receiver_handlers[i].first; i++) {
		handler = &lg_mx5500_receiver_handlers[i];
		if(handler->action == buffer[2] &&
				handler->first == buffer[3]) {
			handler->func(receiver, buffer, count);
			handeld = 1;
		}
	}

	if(!handeld)
		lg_mx5500_err(device, "Unhandeld receiver message %02x %02x", buffer[2], buffer[3]);
}

static struct lg_mx5500_receiver *lg_mx5500_receiver_create(struct lg_mx5500 *device)
{
	struct lg_mx5500_receiver *receiver;

	receiver = kzalloc(sizeof(*receiver), GFP_KERNEL);
	if(!receiver)
		return NULL;

	receiver->device = device;
	receiver->keyboard = NULL;
	receiver->mouse = NULL;
	receiver->initialized = 0;

	return receiver;
}

static void lg_mx5500_receiver_destroy(struct lg_mx5500_receiver *receiver)
{
	if(receiver->keyboard)
		lg_mx5500_keyboard_destroy(receiver->keyboard);

	if(receiver->mouse)
		lg_mx5500_mouse_destroy(receiver->mouse);

	kfree(receiver);
}

static void lg_mx5500_receiver_hid_receive(struct lg_mx5500 *device, const u8 *buffer,
								size_t count)
{
	if(count < 4) {
		lg_mx5500_err(device, "Too few bytes to handle");
		return;
	}

	if(buffer[1] == 0xFF) {
		lg_mx5500_receiver_handle(device, buffer, count);
	}
	else if(buffer[2] == 0x41) {
		lg_mx5500_receiver_logon_device(lg_mx5500_get_data(device), buffer, count);
	}
	else if(buffer[2] == 0x40) {
		lg_mx5500_receiver_logoff_device(lg_mx5500_get_data(device), buffer, count);
	}
}

int lg_mx5500_receiver_init(struct lg_mx5500 *device)
{
	struct lg_mx5500_receiver *receiver;
	u8 cmd[7] = { 0x10, 0xFF, LG_MX5500_ACTION_GET, 0x00, 0x00, 0x00, 0x00 };

	receiver = lg_mx5500_receiver_create(device);

	if(!receiver) {
		lg_mx5500_err(device, "Can't alloc device\n");
		return -ENOMEM;
	}

	lg_mx5500_set_data(device, receiver);
	lg_mx5500_set_hid_receive_handler(device, lg_mx5500_receiver_hid_receive);

	lg_mx5500_queue(device, &device->out_queue, cmd, sizeof(cmd));

	return 0;
}

void lg_mx5500_receiver_exit(struct lg_mx5500 *device)
{
	struct lg_mx5500_receiver *receiver= lg_mx5500_get_data(device);

	lg_mx5500_receiver_destroy(receiver);
}
