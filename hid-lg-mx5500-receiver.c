#include <linux/hid.h>
#include <asm/atomic.h>

#include "hid-lg-core.h"
#include "hid-lg-mx5500.h"
#include "hid-lg-mx-revolution.h"
#include "hid-lg-mx5500-keyboard.h"
#include "hid-lg-mx5500-receiver.h"

struct lg_mx5500_receiver {
	u8 initialized;
	u8 max_devices;

	struct lg_device device;

	struct lg_device *connected_devices[LG_MX5500_RECEIVER_MAX_DEVICES];
};

int lg_mx5500_receiver_init_new(struct hid_device *hdev);

void lg_mx5500_receiver_exit(struct lg_device *device);

void lg_mx5500_receiver_hid_receive(struct lg_device *device, const u8 *buffer,
								size_t count);

struct lg_device *lg_mx5500_receiver_find_device(struct lg_device *device,
						 struct hid_device_id device_id);

static struct lg_driver driver = {
	.name = "Logitech MX5500 Receiver",
	.device_id = { HID_USB_DEVICE(USB_VENDOR_ID_LOGITECH,
			USB_DEVICE_ID_MX5500_RECEIVER) },
	.device_code = LG_DRIVER_NO_CODE,

	.init = lg_mx5500_receiver_init_new,
	.exit = lg_mx5500_receiver_exit,
	.receive_handler = lg_mx5500_receiver_hid_receive,
	.find_device = lg_mx5500_receiver_find_device,
};

#define get_on_lg_device(device) container_of( 				\
			lg_find_device_on_lg_device(device, driver.device_id),\
			struct lg_mx5500_receiver, device)

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
	int pos;
	int code;
	struct lg_device *new_device;
	if (count < 7)
		return;

	pos = buffer[1];
	if (pos < 1 || pos > LG_MX5500_RECEIVER_MAX_DEVICES)
		return;

	if (receiver->connected_devices[pos])
		receiver->connected_devices[pos]->driver->exit(
			receiver->connected_devices[pos]);

	code = buffer[6];
	new_device = lg_create_on_receiver(&receiver->device, code,
					   buffer, count);
	if (!new_device)
		lg_device_err(receiver->device, "Couldn't initialize new device "
			"with code 0x%02x", code);

	receiver->connected_devices[pos] = new_device;
}

static void lg_mx5500_receiver_logoff_device(struct lg_mx5500_receiver *receiver,
						const u8 *buffer, size_t count)
{
	struct lg_device *device;
	int pos;

	if (count < 2)
		return;

	pos = buffer[1];
	if (pos < 1 || pos > LG_MX5500_RECEIVER_MAX_DEVICES)
		return;

	device = receiver->connected_devices[pos];
	if (!device)
		return;

	device->driver->exit(device);

	receiver->connected_devices[pos] = NULL;
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

	receiver = get_on_lg_device(device);

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

static void lg_mx5500_receiver_handle_on_device(struct lg_mx5500_receiver *receiver,
					 const u8 *buffer, size_t count)
{
	struct lg_device *handling_device;

	handling_device = receiver->connected_devices[buffer[1]];

	if (!handling_device)
		return;

	handling_device->driver->receive_handler(handling_device,
						 buffer, count);
}

void lg_mx5500_receiver_hid_receive(struct lg_device *device, const u8 *buffer,
								size_t count)
{
	struct lg_mx5500_receiver *receiver = get_on_lg_device(device);

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
	} else if (buffer[1] >= 1 && buffer[1] <= LG_MX5500_RECEIVER_MAX_DEVICES) {
		lg_mx5500_receiver_handle_on_device(receiver, buffer, count);
	}
}

struct lg_device *lg_mx5500_receiver_find_device(struct lg_device *device,
						 struct hid_device_id device_id)
{
	struct lg_mx5500_receiver *receiver = get_on_lg_device(device);
	struct lg_driver *compare_driver;
	int i;

	for (i = 0; i < LG_MX5500_RECEIVER_MAX_DEVICES; i++) {
		if (!receiver->connected_devices[i])
			continue;

		compare_driver = receiver->connected_devices[i]->driver;
		if (compare_driver->device_id.bus == device_id.bus &&
			compare_driver->device_id.vendor == device_id.vendor &&
			compare_driver->device_id.product == device_id.product)
			return receiver->connected_devices[i];
	}

	return NULL;
}

static struct lg_mx5500_receiver *lg_mx5500_receiver_create(void)
{
	struct lg_mx5500_receiver *receiver;
	int i;

	receiver = kzalloc(sizeof(*receiver), GFP_KERNEL);
	if (!receiver)
		return NULL;

	for (i = 0; i < LG_MX5500_RECEIVER_MAX_DEVICES; i++) {
		receiver->connected_devices[i] = NULL;
	}

	receiver->initialized = 0;

	return receiver;
}

static void lg_mx5500_receiver_destroy(struct lg_mx5500_receiver *receiver)
{
	int i;
	for (i = 0; i < LG_MX5500_RECEIVER_MAX_DEVICES; i++) {
		if (receiver->connected_devices[i]) {
			receiver->connected_devices[i]->driver->exit(
				receiver->connected_devices[i]);
		}
	}

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
	struct lg_mx5500_receiver *receiver= get_on_lg_device(device);

	lg_mx5500_receiver_destroy(receiver);
}

struct lg_driver *lg_mx5500_receiver_get_driver()
{
	return &driver;
}
