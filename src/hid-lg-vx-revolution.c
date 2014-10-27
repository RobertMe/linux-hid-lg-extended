/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <linux/module.h>
#include <linux/hid.h>
#include <linux/hid-lg-extended.h>
#include <linux/sched.h>
#include <linux/wait.h>

#define USB_DEVICE_ID_VX_REVOLUTION 0xc521

struct lg_vx_revolution{
	struct lg_device device;
	wait_queue_head_t received;

	s8 battery_level;
};

int lg_vx_revolution_init_device(struct hid_device *hdev);

void lg_vx_revolution_exit_device(struct lg_device *device);

void lg_vx_revolution_handle(struct lg_device *device, const u8 *buffer,
								size_t count);

#define get_on_lg_device(device) container_of( 				\
			lg_find_device_on_lg_device(device, driver.device_id),\
			struct lg_vx_revolution, device)

#define get_on_device(device) container_of( 				\
			lg_find_device_on_device(device, driver.device_id),\
			struct lg_vx_revolution, device)

struct lg_vx_revolution_handler {
	u8 action;
	u8 first;
	void (*func)(struct lg_vx_revolution *mouse, const u8 *payload, size_t size);
};

static struct lg_driver driver = {
	.name = "logitech-vx-revolution",
	.device_name = "Logitech VX Revolution",
	.device_id = { HID_USB_DEVICE(USB_VENDOR_ID_LOGITECH,
			USB_DEVICE_ID_VX_REVOLUTION) },
	.device_code = LG_DRIVER_NO_CODE,

	.init = lg_vx_revolution_init_device,
	.exit = lg_vx_revolution_exit_device,
	.receive_handler = lg_vx_revolution_handle,
};

static int lg_vx_revolution_request_battery(struct lg_vx_revolution *mouse)
{
	u8 cmd[7] = { 0x10, 0x01, LG_DEVICE_ACTION_GET, 0x0d, 0x00, 0x00, 0x00 };

	mouse->battery_level = -1;
	lg_device_queue_out(mouse->device, cmd, sizeof(cmd));

	return wait_event_interruptible(mouse->received,
				 mouse->battery_level >= 0);
}

static ssize_t mouse_show_battery(struct device *device,
			struct device_attribute *attr, char *buf)
{
	struct lg_vx_revolution *mouse = get_on_device(device);

	if (lg_vx_revolution_request_battery(mouse))
		return 0;

	return scnprintf(buf, PAGE_SIZE, "%d%%\n", mouse->battery_level);
}

static DEVICE_ATTR(battery, 0444, mouse_show_battery, NULL);

static ssize_t mouse_show_name(struct device *device,
			struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%s\n", driver.device_name);
}

static DEVICE_ATTR(name, 0444, mouse_show_name, NULL);

static struct attribute *mouse_attrs[] = {
	&dev_attr_battery.attr,
	&dev_attr_name.attr,
	NULL,
};

static void mouse_handle_get_battery(
		struct lg_vx_revolution *mouse, const u8 *buf,
		size_t size)
{
	mouse->battery_level = buf[4];
}

static struct lg_vx_revolution_handler lg_vx_revolution_handlers[] = {
	{ .action = LG_DEVICE_ACTION_GET, .first = 0x0d,
		.func = mouse_handle_get_battery },
	{ }
};

void lg_vx_revolution_handle(struct lg_device *device, const u8 *buffer,
								size_t count)
{
	int i;
	int handeld = 0;
	struct lg_vx_revolution *mouse;
	struct lg_vx_revolution_handler *handler;

	mouse = get_on_lg_device(device);

	for (i = 0; lg_vx_revolution_handlers[i].action ||
		lg_vx_revolution_handlers[i].first; i++) {
		handler = &lg_vx_revolution_handlers[i];
		if (handler->action == buffer[2] &&
				handler->first == buffer[3]) {
			if (handler->func != LG_DEVICE_HANDLER_IGNORE)
				handler->func(mouse, buffer, count);
			handeld = 1;
		}
	}

	if (!handeld)
		lg_device_err((*device), "Unhandeld mouse message %02x %02x", buffer[2], buffer[3]);

	wake_up_interruptible(&mouse->received);
}

static struct lg_vx_revolution *lg_vx_revolution_create(void)
{
	struct lg_vx_revolution *mouse;

	mouse = kzalloc(sizeof(*mouse), GFP_KERNEL);
	if (!mouse)
		return NULL;

	mouse->battery_level = -1;
	init_waitqueue_head(&mouse->received);

	return mouse;
}

void lg_vx_revolution_destroy(struct lg_vx_revolution *mouse)
{
	if (!mouse)
		return;

	lg_device_destroy(&mouse->device);
	kfree(mouse);
}

int lg_vx_revolution_init_device(struct hid_device *hdev)
{
	int ret;
	struct lg_vx_revolution *mouse;

	if (hdev->type == HID_TYPE_USBMOUSE)
		return 0;

	mouse = lg_vx_revolution_create();
	if (!mouse) {
		ret = -ENOMEM;
		goto error;
	}

	ret = lg_device_init(&mouse->device, hdev, &driver);
	if (ret)
		goto error_free;

	ret = sysfs_create_files(&mouse->device.hdev->dev.kobj,
		(const struct attribute**)mouse_attrs);
	if (ret)
		goto error_free;

	return ret;
error_free:
	kfree(mouse);
error:
	return ret;
}

void lg_vx_revolution_exit_device(struct lg_device *device)
{
	struct lg_vx_revolution *mouse;

	mouse = get_on_lg_device(device);

	if (mouse == NULL)
		return;

	sysfs_remove_files(&device->hdev->dev.kobj,
		(const struct attribute**)mouse_attrs);

	lg_vx_revolution_destroy(mouse);
}

static const struct hid_device_id lg_vx_revolution_device[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_LOGITECH,
			USB_DEVICE_ID_VX_REVOLUTION) },
	{ }
};

MODULE_DEVICE_TABLE(hid, lg_vx_revolution_device);

static int __init lg_vx_revolution_init(void)
{
	lg_register_driver(&driver);

	return 0;
}

static void __exit lg_vx_revolution_exit(void)
{
	lg_unregister_driver(&driver);
}

module_init(lg_vx_revolution_init);
module_exit(lg_vx_revolution_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Robert Meijers <robert.meijers@gmail.com>");
MODULE_DESCRIPTION("Logitech VX Revolution extended driver");
MODULE_VERSION("0.1");
