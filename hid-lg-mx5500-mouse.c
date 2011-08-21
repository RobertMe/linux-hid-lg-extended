#include <linux/sched.h>
#include <linux/wait.h>

#include "hid-lg-mx5500-mouse.h"

struct lg_mx5500_mouse {
	struct lg_mx5500 *device;
	wait_queue_head_t received;
	u8 devnum;
	u8 initialized;

	short battery_level;
};

struct lg_mx5500_mouse_handler {
	u8 action;
	u8 first;
	void (*func)(struct lg_mx5500_mouse *mouse, const u8 *payload, size_t size);
};

static inline struct lg_mx5500_mouse *lg_mx5500_mouse_get_from_device(
			struct device *device)
{
	return lg_mx5500_get_mouse(dev_get_drvdata(device));
}

static int lg_mx5500_mouse_request_battery(struct lg_mx5500_mouse *mouse)
{
	u8 cmd[7] = { 0x10, 0x01, LG_MX5500_ACTION_GET, 0x0d, 0x00, 0x00, 0x00 };

	cmd[1] = mouse->devnum;
	mouse->battery_level = -1;
	lg_mx5500_queue_out(mouse->device, cmd, sizeof(cmd));

	return wait_event_interruptible(mouse->received,
				 mouse->battery_level >= 0);
}

static ssize_t mouse_show_battery(struct device *device,
			struct device_attribute *attr, char *buf)
{
	struct lg_mx5500_mouse *mouse = lg_mx5500_mouse_get_from_device(device);

	if (lg_mx5500_mouse_request_battery(mouse))
		return 0;

	return scnprintf(buf, PAGE_SIZE, "%d%%\n", mouse->battery_level);
}

static DEVICE_ATTR(battery, S_IRUGO, mouse_show_battery, NULL);

static struct attribute *mouse_attrs[] = {
	&dev_attr_battery.attr,
	NULL,
};

static struct attribute_group mouse_attr_group = {
	.name = "mouse",
	.attrs = mouse_attrs,
};

static void mouse_handle_get_battery(
		struct lg_mx5500_mouse *mouse, const u8 *buf,
		size_t size)
{
	mouse->battery_level = buf[4];
}

static struct lg_mx5500_mouse_handler lg_mx5500_mouse_handlers[] = {
	{ .action = LG_MX5500_ACTION_GET, .first = 0x0d,
		.func = mouse_handle_get_battery },
	{ }
};

void lg_mx5500_mouse_handle(struct lg_mx5500 *device, const u8 *buffer,
								size_t count)
{
	int i;
	int handeld = 0;
	struct lg_mx5500_mouse *mouse;
	struct lg_mx5500_mouse_handler *handler;

	mouse = lg_mx5500_get_mouse(device);

	for (i = 0; lg_mx5500_mouse_handlers[i].action ||
		lg_mx5500_mouse_handlers[i].first; i++) {
		handler = &lg_mx5500_mouse_handlers[i];
		if (handler->action == buffer[2] &&
				handler->first == buffer[3]) {
			handler->func(mouse, buffer, count);
			handeld = 1;
		}
	}

	if (!handeld)
		lg_mx5500_err(device, "Unhandeld mouse message %02x %02x", buffer[2], buffer[3]);

	wake_up_interruptible(&mouse->received);
}

struct lg_mx5500_mouse *lg_mx5500_mouse_create(struct lg_mx5500 *device)
{
	struct lg_mx5500_mouse *mouse;

	mouse = kzalloc(sizeof(*mouse), GFP_KERNEL);
	if (!mouse)
		return NULL;

	mouse->device = device;
	mouse->devnum = 1;
	mouse->battery_level = -1;
	mouse->initialized = 0;
	init_waitqueue_head(&mouse->received);

	return mouse;
}

void lg_mx5500_mouse_destroy(struct lg_mx5500_mouse *mouse)
{
	if (!mouse)
		return;

	kfree(mouse);
}

int lg_mx5500_mouse_init(struct lg_mx5500 *device)
{
	int ret;
	struct lg_mx5500_mouse *mouse;

	mouse = lg_mx5500_mouse_create(device);
	if (!mouse) {
		ret = -ENOMEM;
		goto error;
	}

	ret = sysfs_create_files(&device->hdev->dev.kobj,
		(const struct attribute**)mouse_attrs);
	if (ret)
		goto error_free;

	lg_mx5500_set_data(device, mouse);
	lg_mx5500_set_hid_receive_handler(device, lg_mx5500_mouse_handle);

	return ret;
error_free:
	kfree(mouse);
error:
	return ret;
}

void lg_mx5500_mouse_exit(struct lg_mx5500 *device)
{
	struct lg_mx5500_mouse *mouse;

	mouse = lg_mx5500_get_mouse(device);

	if (mouse == NULL)
		return;

	sysfs_remove_files(&device->hdev->dev.kobj,
		(const struct attribute **)mouse_attrs);

	lg_mx5500_mouse_destroy(mouse);
}

struct lg_mx5500_mouse *lg_mx5500_mouse_init_on_receiver(
			struct lg_mx5500 *device,
			const u8 *buffer, size_t count)
{
	struct lg_mx5500_mouse *mouse;

	mouse = lg_mx5500_mouse_create(device);
	if (!mouse)
		goto error;

	mouse->devnum = buffer[1];

	if (sysfs_create_group(&device->hdev->dev.kobj,
		&mouse_attr_group))
		goto error_free;

	return mouse;
error_free:
	kfree(mouse);
error:
	return NULL;
}

void lg_mx5500_mouse_exit_on_receiver(struct lg_mx5500_mouse *mouse)
{
	if (mouse == NULL)
		return;

	sysfs_remove_group(&mouse->device->hdev->dev.kobj,
				&mouse_attr_group);
	lg_mx5500_mouse_destroy(mouse);
}