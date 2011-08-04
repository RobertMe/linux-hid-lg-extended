#include "hid-lg-mx5500.h"
#include "hid-lg-mx5500-receiver.h"

struct lg_mx5500_receiver {
	u8 initialized;
	u8 mode;

	struct lg_mx5500 *device;

	struct lg_mx5500_keyboard *keyboard;
	struct lg_mx5500_mouse *mouse;
};

struct lg_mx5500_receiver_handler {
	u8 action;
	u8 first;
	void (*func)(struct lg_mx5500_receiver *receiver, const u8 *payload, size_t size);
};

static int lg_mx5500_receiver_update_mode(struct lg_mx5500_receiver *receiver,
							const u8 *buffer, size_t count)
{
	if(count < 6) {
		lg_mx5500_err(receiver->device, "To few bytes to read mode");
		return 1;
	}
	receiver->mode = buffer[5];
	lg_mx5500_dbg(receiver->device, "Mode changed to 0x%02x", receiver->mode);

	return 0;
}

static void lg_mx5500_receiver_set_mode(struct lg_mx5500_receiver *receiver, u8 mode)
{
	u8 cmd[7] = { 0x10, 0xFF, LG_MX5500_ACTION_SET, 0x00, 0x00, 0x00, 0x00 };

	cmd[5] = mode;
	lg_mx5500_queue_out(receiver->device, cmd, sizeof(cmd));
}

static void lg_mx5500_receiver_handle_get_mode(struct lg_mx5500_receiver *receiver,
								const u8 *buffer, size_t count)
{
	if(lg_mx5500_receiver_update_mode(receiver, buffer, count))
		return;

	if(receiver->initialized)
		return;

	if(receiver->mode != 0x03)
		lg_mx5500_receiver_set_mode(receiver, 0x03);
}

static void lg_mx5500_receiver_handle_set_mode(struct lg_mx5500_receiver *receiver,
								const u8 *buffer, size_t count)
{
	if(lg_mx5500_receiver_update_mode(receiver, buffer, count))
		return;

	if(receiver->initialized)
		return;

	receiver->initialized = 1;
}

static struct lg_mx5500_receiver_handler lg_mx5500_receiver_handlers[] = {
	{ .action = LG_MX5500_ACTION_GET, .first = 0x00,
		.func = lg_mx5500_receiver_handle_get_mode },
	{ .action = LG_MX5500_ACTION_SET, .first = 0x00,
		.func = lg_mx5500_receiver_handle_set_mode },
	{ }
};

void lg_mx5500_receiver_handle(struct lg_mx5500 *device, const u8 *buffer,
								size_t count)
{
	int i;
	struct lg_mx5500_receiver *receiver;
	struct lg_mx5500_receiver_handler *handler;

	receiver = lg_mx5500_get_data(device);

	for(i = 0; lg_mx5500_receiver_handlers[i].action ||
		lg_mx5500_receiver_handlers[i].first; i++) {
		handler = &lg_mx5500_receiver_handlers[i];
		if(handler->action == buffer[2] &&
				handler->first == buffer[3]) {
			handler->func(receiver, buffer, count);
		}
	}
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
