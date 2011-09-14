#include <linux/hid.h>
#include <asm/atomic.h>

#include "hid-lg-core.h"
#include "hid-lg-mx-revolution.h"
#include "hid-lg-mx5500-keyboard.h"
#include "hid-lg-mx5500-receiver.h"

int lg_mx5500_receiver_init_new(struct hid_device *hdev);

struct lg_mx5500_receiver_handler {
	u8 action;
	u8 first;
	void (*func)(struct lg_mx5500_receiver *receiver, const u8 *payload, size_t size);
};

static int lg_mx5500_receiver_update_max_devices(struct lg_mx5500_receiver *receiver,
							const u8 *buffer, size_t count)
{
	if (count < 6) {
		lg_device_err(receiver->device, "Too few bytes to set maximum number of devices");
		return 1;
	}
	receiver->max_devices = buffer[5];
	lg_device_dbg(receiver->device, "Maximum number of devices changed to 0x%02x",
			receiver->max_devices);

	return 0;
}

static void lg_mx5500_receiver_set_max_devices(struct lg_mx5500_receiver *receiver,
						u8 max_devices)
{
	u8 cmd[7] = { 0x10, 0xFF, LG_DEVICE_ACTION_SET, 0x00, 0x00, 0x00, 0x00 };

	cmd[5] = max_devices;
	lg_device_queue_out(receiver->device, cmd, sizeof(cmd));

	if (receiver->initialized)
		return;

	receiver->initialized = 1;
}

static void lg_mx5500_receiver_logon_device(struct lg_mx5500_receiver *receiver,
						const u8 *buffer, size_t count)
{
	switch (buffer[1]) {
	case 0x01:
		if (receiver->keyboard) {
			lg_mx5500_keyboard_exit_on_receiver(receiver->keyboard);
		}
		receiver->keyboard = lg_mx5500_keyboard_init_on_receiver(
					&receiver->device, buffer, count);
		break;
	case 0x02:
		if (receiver->keyboard) {
			lg_mx_revolution_exit_on_receiver(receiver->mouse);
		}
		receiver->mouse = lg_mx_revolution_init_on_receiver(
					&receiver->device, buffer, count);
		break;
	}
}

static void lg_mx5500_receiver_logoff_device(struct lg_mx5500_receiver *receiver,
						const u8 *buffer, size_t count)
{
	switch (buffer[1]) {
	case 0x01:
		if (receiver->keyboard) {
			lg_mx5500_keyboard_exit_on_receiver(receiver->keyboard);
			receiver->keyboard = NULL;
		}
		break;
	case 0x02:
		if (receiver->mouse) {
			lg_mx_revolution_exit_on_receiver(receiver->mouse);
			receiver->mouse = NULL;
		}
		break;
	}
}

static void lg_mx5500_receiver_devices_logon(struct lg_mx5500_receiver *receiver)
{
	u8 cmd[7] = { 0x10, 0xFF, LG_DEVICE_ACTION_SET, 0x02, 0x02, 0x00, 0x00 };

	lg_device_queue_out(receiver->device, cmd, sizeof(cmd));
}

static void lg_mx5500_receiver_handle_get_max_devices(struct lg_mx5500_receiver *receiver,
								const u8 *buffer, size_t count)
{
	if (lg_mx5500_receiver_update_max_devices(receiver, buffer, count))
		return;

	if (receiver->initialized)
		return;

	if (receiver->max_devices != 0x03)
		lg_mx5500_receiver_set_max_devices(receiver, 0x03);
	else
		lg_mx5500_receiver_devices_logon(receiver);
}

static void lg_mx5500_receiver_handle_set_max_devices(struct lg_mx5500_receiver *receiver,
								const u8 *buffer, size_t count)
{
	if (lg_mx5500_receiver_update_max_devices(receiver, buffer, count))
		return;

	if (receiver->initialized)
		return;

	lg_mx5500_receiver_devices_logon(receiver);
}

static struct lg_mx5500_receiver_handler lg_mx5500_receiver_handlers[] = {
	{ .action = LG_DEVICE_ACTION_GET, .first = 0x00,
		.func = lg_mx5500_receiver_handle_get_max_devices },
	{ .action = LG_DEVICE_ACTION_SET, .first = 0x00,
		.func = lg_mx5500_receiver_handle_set_max_devices },
	{ .action = LG_DEVICE_ACTION_SET, .first = 0x02,
		.func = LG_DEVICE_HANDLER_IGNORE },
	{ }
};

void lg_mx5500_receiver_handle(struct lg_device *device, const u8 *buffer,
								size_t count)
{
	int i;
	int handeld = 0;
	struct lg_mx5500_receiver *receiver;
	struct lg_mx5500_receiver_handler *handler;

	receiver = lg_device_get_receiver(device);

	for (i = 0; lg_mx5500_receiver_handlers[i].action ||
		lg_mx5500_receiver_handlers[i].first; i++) {
		handler = &lg_mx5500_receiver_handlers[i];
		if (handler->action == buffer[2] &&
				handler->first == buffer[3]) {
			if (handler->func != LG_DEVICE_HANDLER_IGNORE)
				handler->func(receiver, buffer, count);
			handeld = 1;
		}
	}

	if (!handeld)
		lg_device_err(receiver->device, "Unhandeld receiver message %02x %02x", buffer[2], buffer[3]);
}

void lg_mx5500_receiver_hid_receive(struct lg_device *device, const u8 *buffer,
								size_t count)
{
	struct lg_mx5500_receiver *receiver = lg_device_get_receiver(device);

	if (count < 4) {
		lg_device_err((*device), "Too few bytes to handle");
		return;
	}

	if(!receiver)
		return;

	if (buffer[1] == 0xFF) {
		lg_mx5500_receiver_handle(device, buffer, count);
	} else if (buffer[2] == 0x41) {
		lg_mx5500_receiver_logon_device(receiver, buffer, count);
	} else if (buffer[2] == 0x40) {
		lg_mx5500_receiver_logoff_device(receiver, buffer, count);
	} else if (buffer[1] == 0x01) {
		if (receiver->keyboard)
			lg_mx5500_keyboard_handle(device, buffer, count);
		else
			lg_device_err((*device), "received message for keyboard,"
				"but there isn't any present.\n"
				"message is 0x%02x 0x%02x 0x%02x",
				buffer[2], buffer[3], buffer[4]);
	} else if (buffer[1] == 0x02) {
		if (receiver->mouse)
			lg_mx_revolution_handle(device, buffer, count);
		else
			lg_device_err((*device), "received message for mouse,"
				"but there isn't any present.\n"
				"message is 0x%02x 0x%02x 0x%02x",
				buffer[2], buffer[3], buffer[4]);
	}
}

static struct lg_driver driver = {
	.device_id = { HID_USB_DEVICE(USB_VENDOR_ID_LOGITECH,
			USB_DEVICE_ID_MX5500_RECEIVER) },
	.init = lg_mx5500_receiver_init_new,
	.exit = lg_mx5500_receiver_exit,
	.receive_handler = lg_mx5500_receiver_hid_receive,
	.type = LG_MX5500_RECEIVER,
	.name = "Logitech MX5500 Receiver",
};

static struct lg_mx5500_receiver *lg_mx5500_receiver_create(void)
{
	struct lg_mx5500_receiver *receiver;

	receiver = kzalloc(sizeof(*receiver), GFP_KERNEL);
	if (!receiver)
		return NULL;

	receiver->keyboard = NULL;
	receiver->mouse = NULL;
	receiver->initialized = 0;

	return receiver;
}

static void lg_mx5500_receiver_destroy(struct lg_mx5500_receiver *receiver)
{
	if (receiver->keyboard)
		lg_mx5500_keyboard_exit_on_receiver(receiver->keyboard);

	if (receiver->mouse)
		lg_mx_revolution_exit_on_receiver(receiver->mouse);

	lg_device_destroy(&receiver->device);
	kfree(receiver);
}

int lg_mx5500_receiver_init_new(struct hid_device *hdev)
{
	int ret;
	struct lg_mx5500_receiver *receiver;
	u8 cmd[7] = { 0x10, 0xFF, LG_DEVICE_ACTION_GET, 0x00, 0x00, 0x00, 0x00 };

	receiver = lg_mx5500_receiver_create();

	if (!receiver) {
		hid_err(hdev, "Can't allocate MX5500 receiver\n");
		ret = -ENOMEM;
		goto err;
	}

	ret = lg_device_init(&receiver->device, hdev, &driver);
	if (ret)
		goto err_free;

	lg_device_queue_out(receiver->device, cmd, sizeof(cmd));

	return 0;
err_free:
	lg_mx5500_receiver_destroy(receiver);
err:
	return ret;
}

void lg_mx5500_receiver_exit(struct lg_device *device)
{
	struct lg_mx5500_receiver *receiver= lg_device_get_receiver(device);

	lg_mx5500_receiver_destroy(receiver);
}

struct lg_driver *lg_mx5500_receiver_get_driver()
{
	return &driver;
}
