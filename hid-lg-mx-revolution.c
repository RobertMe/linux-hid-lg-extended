#include <linux/sched.h>
#include <linux/wait.h>

#include "hid-lg-mx-revolution.h"

struct lg_mx_revolution {
	struct lg_mx5500 *device;
	wait_queue_head_t received;
	u8 devnum;
	u8 initialized;

	short battery_level;
	u8 scrollmode_set;
	u8 scrollmode[3];
};

struct lg_mx_revolution_handler {
	u8 action;
	u8 first;
	void (*func)(struct lg_mx_revolution *mouse, const u8 *payload, size_t size);
};

static inline struct lg_mx_revolution *lg_mx_revolution_get_from_device(
			struct device *device)
{
	return lg_mx5500_get_mouse(dev_get_drvdata(device));
}

static int lg_mx_revolution_request_battery(struct lg_mx_revolution *mouse)
{
	u8 cmd[7] = { 0x10, 0x01, LG_MX5500_ACTION_GET, 0x0d, 0x00, 0x00, 0x00 };

	cmd[1] = mouse->devnum;
	mouse->battery_level = -1;
	lg_mx5500_queue_out(mouse->device, cmd, sizeof(cmd));

	return wait_event_interruptible(mouse->received,
				 mouse->battery_level >= 0);
}

static int lg_mx_revolution_request_scrollmode(struct lg_mx_revolution *mouse)
{
	u8 cmd[7] = { 0x10, 0x01, LG_MX5500_ACTION_GET, 0x56, 0x00, 0x00, 0x00 };

	if (mouse->scrollmode_set)
		return 0;

	cmd[1] = mouse->devnum;
	lg_mx5500_queue_out(mouse->device, cmd, sizeof(cmd));

	return wait_event_interruptible(mouse->received,
				 mouse->scrollmode_set);
}

static ssize_t mouse_show_battery(struct device *device,
			struct device_attribute *attr, char *buf)
{
	struct lg_mx_revolution *mouse = lg_mx_revolution_get_from_device(device);

	if (lg_mx_revolution_request_battery(mouse))
		return 0;

	return scnprintf(buf, PAGE_SIZE, "%d%%\n", mouse->battery_level);
}

static DEVICE_ATTR(battery, S_IRUGO, mouse_show_battery, NULL);

static ssize_t mouse_show_scrollmode(struct device *device,
			struct device_attribute *attr, char *buf)
{
	u8 mode;
	ssize_t length, remaining;
	char *startbuf;
	struct lg_mx_revolution *mouse = lg_mx_revolution_get_from_device(device);

	if (lg_mx_revolution_request_scrollmode(mouse))
		return 0;

	startbuf = buf;
	mode = mouse->scrollmode[0];

	// msb determines if the setting is temp or default
	// (0 is temp, 1 is deault)
	if (mode < 0x80)
		length = scnprintf(buf, PAGE_SIZE, "temp ");
	else
		length = scnprintf(buf, PAGE_SIZE, "default ");
	remaining = PAGE_SIZE - length;
	buf += length;

	mode = (mode | 0xF0) - 0xF0;
	//mode is static
	if (mode <= 4) {
		if (mode == LG_MX5500_SCROLLMODE_CLICK_CLICK_MOVE)
			length += scnprintf(buf, remaining,
					"click-to-click on move");
		else if (mode == LG_MX5500_SCROLLMODE_FREESPIN_MOVE)
			length += scnprintf(buf, remaining,
					"freespin on move");
		else if (mode == LG_MX5500_SCROLLMODE_CLICK_CLICK_IMMEDIATE)
			length += scnprintf(buf, remaining,
					"click-to-click immediate");
		else if (mode == LG_MX5500_SCROLLMODE_FREESPIN_IMMEDIATE)
			length += scnprintf(buf, remaining,
					"freespin immediate");
	}
	else if (mode == LG_MX5500_SCROLLMODE_AUTOMATIC) {
		if (mouse->scrollmode[1] == mouse->scrollmode[2])
			length += scnprintf(buf, remaining,
					"freespin above %i", mouse->scrollmode[1]);
		else
			length += scnprintf(buf, remaining,
					"freespin above %i up and %i down",
					mouse->scrollmode[1], mouse->scrollmode[2]);
	}
	else if (mode == LG_MX5500_SCROLLMODE_BUTTON_TOGGLE ||
			(mode == LG_MX5500_SCROLLMODE_BUTTON_SWITCH &&
			(mouse->scrollmode[1] >> 4) == ((mouse->scrollmode[1] | 0xF0) - 0xF0))) {
		length += scnprintf(buf, remaining,
				"toggle using button %i", (mouse->scrollmode[1] | 0xF0) - 0xF0);
	}
	else if (mode == LG_MX5500_SCROLLMODE_BUTTON_SWITCH) {
		length += scnprintf(buf, remaining,
				"switch to freespin using button %i"
				" and to click-to-click using button %i",
				mouse->scrollmode[1] >> 4,
				(mouse->scrollmode[1] | 0xF0) - 0xF0);
	}

	if (length < PAGE_SIZE) {
		startbuf[length] = '\n';
		length++;
	}

	return length;
}

static ssize_t mouse_store_scrollmode(struct device *device,
		struct device_attribute *attr, const char *buf, size_t count)
{
	short int mode, set_default, first, second, param_count;
	char cmd[7] = { 0x10, 0x01, LG_MX5500_ACTION_SET, 0x56, 0x00, 0x00, 0x00 };
	struct lg_mx_revolution *mouse = lg_mx_revolution_get_from_device(device);

	set_default = 0;
	param_count = sscanf(buf, "%hi %hi %hi %hi", &mode, &set_default,
			     &first, &second);

	if (!param_count)
		return -EINVAL;

	if (mode < 1 || mode == 6 || mode > 8)
		return -EINVAL;

	cmd[1] = mouse->devnum;
	cmd[4] = set_default ? mode | 0x80 : mode;

	if (mode == LG_MX5500_SCROLLMODE_AUTOMATIC) {
		if (param_count < 4)
			return -EINVAL;

		cmd[5] = first;
		cmd[6] = second;
	}
	else if (mode == LG_MX5500_SCROLLMODE_BUTTON_SWITCH) {
		if (param_count < 4)
			return -EINVAL;

		cmd[5] = (first << 4) + second;
	}
	else if (mode == LG_MX5500_SCROLLMODE_BUTTON_TOGGLE) {
		if (param_count < 3)
			return -EINVAL;

		cmd[6] = first;
	}

	lg_mx5500_queue_out(mouse->device, cmd, sizeof(cmd));

	return count;
}

static DEVICE_ATTR(scrollmode, S_IRUGO | S_IWUGO, mouse_show_scrollmode, mouse_store_scrollmode);

static struct attribute *mouse_attrs[] = {
	&dev_attr_battery.attr,
	&dev_attr_scrollmode.attr,
	NULL,
};

static struct attribute_group mouse_attr_group = {
	.name = "mouse",
	.attrs = mouse_attrs,
};

static void mouse_handle_get_battery(
		struct lg_mx_revolution *mouse, const u8 *buf,
		size_t size)
{
	mouse->battery_level = buf[4];
}

static void mouse_handle_scrollmode(
		struct lg_mx_revolution *mouse, const u8 *buf,
		size_t size)
{
	mouse->scrollmode[0] = buf[4];
	mouse->scrollmode[1] = buf[5];
	mouse->scrollmode[2] = buf[6];
	mouse->scrollmode_set = 1;
}

static struct lg_mx_revolution_handler lg_mx_revolution_handlers[] = {
	{ .action = LG_MX5500_ACTION_GET, .first = 0x0d,
		.func = mouse_handle_get_battery },
	{ .action = LG_MX5500_ACTION_GET, .first = 0x56,
		.func = mouse_handle_scrollmode },
	{ .action = LG_MX5500_ACTION_SET, .first = 0x56,
		.func = mouse_handle_scrollmode },
	{ }
};

void lg_mx_revolution_handle(struct lg_mx5500 *device, const u8 *buffer,
								size_t count)
{
	int i;
	int handeld = 0;
	struct lg_mx_revolution *mouse;
	struct lg_mx_revolution_handler *handler;

	mouse = lg_mx5500_get_mouse(device);

	for (i = 0; lg_mx_revolution_handlers[i].action ||
		lg_mx_revolution_handlers[i].first; i++) {
		handler = &lg_mx_revolution_handlers[i];
		if (handler->action == buffer[2] &&
				handler->first == buffer[3]) {
			if (handler->func != LG_MX5500_HANDLER_IGNORE)
				handler->func(mouse, buffer, count);
			handeld = 1;
		}
	}

	if (!handeld)
		lg_mx5500_err(device, "Unhandeld mouse message %02x %02x", buffer[2], buffer[3]);

	wake_up_interruptible(&mouse->received);
}

struct lg_mx_revolution *lg_mx_revolution_create(struct lg_mx5500 *device)
{
	struct lg_mx_revolution *mouse;

	mouse = kzalloc(sizeof(*mouse), GFP_KERNEL);
	if (!mouse)
		return NULL;

	mouse->device = device;
	mouse->devnum = 1;
	mouse->battery_level = -1;
	mouse->scrollmode_set = 0;
	mouse->initialized = 0;
	init_waitqueue_head(&mouse->received);

	return mouse;
}

void lg_mx_revolution_destroy(struct lg_mx_revolution *mouse)
{
	if (!mouse)
		return;

	kfree(mouse);
}

int lg_mx_revolution_init(struct lg_mx5500 *device)
{
	int ret;
	struct lg_mx_revolution *mouse;

	mouse = lg_mx_revolution_create(device);
	if (!mouse) {
		ret = -ENOMEM;
		goto error;
	}

	ret = sysfs_create_files(&device->hdev->dev.kobj,
		(const struct attribute**)mouse_attrs);
	if (ret)
		goto error_free;

	lg_mx5500_set_data(device, mouse);
	lg_mx5500_set_hid_receive_handler(device, lg_mx_revolution_handle);

	return ret;
error_free:
	kfree(mouse);
error:
	return ret;
}

void lg_mx_revolution_exit(struct lg_mx5500 *device)
{
	struct lg_mx_revolution *mouse;

	mouse = lg_mx5500_get_mouse(device);

	if (mouse == NULL)
		return;

	sysfs_remove_files(&device->hdev->dev.kobj,
		(const struct attribute **)mouse_attrs);

	lg_mx_revolution_destroy(mouse);
}

struct lg_mx_revolution *lg_mx_revolution_init_on_receiver(
			struct lg_mx5500 *device,
			const u8 *buffer, size_t count)
{
	struct lg_mx_revolution *mouse;

	mouse = lg_mx_revolution_create(device);
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

void lg_mx_revolution_exit_on_receiver(struct lg_mx_revolution *mouse)
{
	if (mouse == NULL)
		return;

	sysfs_remove_group(&mouse->device->hdev->dev.kobj,
				&mouse_attr_group);
	lg_mx_revolution_destroy(mouse);
}