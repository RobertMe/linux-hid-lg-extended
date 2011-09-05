#include <linux/sched.h>
#include <linux/wait.h>

#include "hid-lg-mx5500-keyboard.h"

struct lg_mx5500_keyboard {
	struct lg_device *device;
	wait_queue_head_t received;
	u8 devnum;
	u8 initialized;

	short battery_level;
	short lcd_page;
	short time[3];
	short date[3];
};

struct lg_mx5500_keyboard_handler {
	u8 action;
	u8 first;
	void (*func)(struct lg_mx5500_keyboard *keyboard, const u8 *payload, size_t size);
};

static inline struct lg_mx5500_keyboard *lg_mx5500_keyboard_get_from_device(
			struct device *device)
{
	return lg_device_get_keyboard(dev_get_drvdata(device));
}

static int lg_mx5500_keyboard_request_battery(struct lg_mx5500_keyboard *keyboard)
{
	u8 cmd[7] = { 0x10, 0x01, LG_DEVICE_ACTION_GET, 0x0d, 0x00, 0x00, 0x00 };

	cmd[1] = keyboard->devnum;
	keyboard->battery_level = -1;
	lg_device_queue_out(keyboard->device, cmd, sizeof(cmd));

	return wait_event_interruptible(keyboard->received,
				 keyboard->battery_level >= 0);
}

static int lg_mx5500_keyboard_request_time(struct lg_mx5500_keyboard *keyboard)
{
	u8 cmd[7] = { 0x10, 0x01, LG_DEVICE_ACTION_GET, 0x31, 0x00, 0x00, 0x00 };

	cmd[1] = keyboard->devnum;
	keyboard->time[0] = -1;
	lg_device_queue_out(keyboard->device, cmd, sizeof(cmd));

	return wait_event_interruptible(keyboard->received,
				 keyboard->time[0] >= 0);
}

static int lg_mx5500_keyboard_request_date(struct lg_mx5500_keyboard *keyboard)
{
	u8 cmd[7] = { 0x10, 0x01, LG_DEVICE_ACTION_GET, 0x32, 0x00, 0x00, 0x00 };

	cmd[1] = keyboard->devnum;
	keyboard->date[0] = -1;
	keyboard->date[1] = -1;
	lg_device_queue_out(keyboard->device, cmd, sizeof(cmd));
	cmd[3] = 0x33;
	lg_device_queue_out(keyboard->device, cmd, sizeof(cmd));

	return wait_event_interruptible(keyboard->received,
				 keyboard->date[0] >= 0 && keyboard->date[1] >= 0);
}

static ssize_t keyboard_show_battery(struct device *device,
			struct device_attribute *attr, char *buf)
{
	struct lg_mx5500_keyboard *keyboard = lg_mx5500_keyboard_get_from_device(device);

	if (lg_mx5500_keyboard_request_battery(keyboard))
		return 0;

	return scnprintf(buf, PAGE_SIZE, "%d%%\n", keyboard->battery_level);
}

static DEVICE_ATTR(battery, S_IRUGO, keyboard_show_battery, NULL);

static ssize_t keyboard_show_lcd_page(struct device *device,
			struct device_attribute *attr, char *buf)
{
	struct lg_mx5500_keyboard *keyboard;

	keyboard = lg_mx5500_keyboard_get_from_device(device);

	return scnprintf(buf, PAGE_SIZE, "%d\n", keyboard->lcd_page);
}

static DEVICE_ATTR(lcd_page, S_IRUGO, keyboard_show_lcd_page, NULL);

static ssize_t keyboard_show_time(struct device *device,
		struct device_attribute *attr, char *buf)
{
	struct lg_mx5500_keyboard *keyboard = lg_mx5500_keyboard_get_from_device(device);

	if (lg_mx5500_keyboard_request_time(keyboard))
		return 0;

	return scnprintf(buf, PAGE_SIZE, "%02hi:%02hi:%02hi\n", keyboard->time[0],
		keyboard->time[1], keyboard->time[2]);
}

static ssize_t keyboard_store_time(struct device *device,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct lg_mx5500_keyboard *keyboard;
	u8 cmd[7] = { 0x10, 0x01, LG_DEVICE_ACTION_SET, 0x31, 0x00, 0x00, 0x00 };
	int err;
	short second, minute, hour;

	keyboard = lg_mx5500_keyboard_get_from_device(device);
	err = sscanf(buf, "%02hi:%02hi:%02hi", &hour, &minute, &second);
	if (err < 0)
		return err;
	else if (err != 3)
		return -EINVAL;

	cmd[1] = keyboard->devnum;
	cmd[4] = second;
	cmd[5] = minute;
	cmd[6] = hour;

	lg_device_queue_out(keyboard->device, cmd, sizeof(cmd));

	return count;
}

static DEVICE_ATTR(time, S_IRUGO | S_IWUGO, keyboard_show_time, keyboard_store_time);

static ssize_t keyboard_show_date(struct device *device,
		struct device_attribute *attr, char *buf)
{
	struct lg_mx5500_keyboard *keyboard = lg_mx5500_keyboard_get_from_device(device);

	if (lg_mx5500_keyboard_request_date(keyboard))
		return 0;

	return scnprintf(buf, PAGE_SIZE, "20%02hi %hi %hi\n", keyboard->date[0],
		keyboard->date[1] + 1, keyboard->date[2]);
}

static ssize_t keyboard_store_date(struct device *device,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct lg_mx5500_keyboard *keyboard;
	u8 cmd_day[7] = { 0x10, 0x01, LG_DEVICE_ACTION_SET, 0x32, 0x06, 0x00, 0x00 };
	u8 cmd_year[7] = { 0x10, 0x01, LG_DEVICE_ACTION_SET, 0x33, 0x00, 0x00, 0x00 };
	int err;
	short day, month, year;

	keyboard = lg_mx5500_keyboard_get_from_device(device);
	err = sscanf(buf, "%02hi %02hi %02hi", &year, &month, &day);
	if (err < 0)
		return err;
	else if (err != 3)
		return -EINVAL;

	if (year > 2000)
		year -= 2000;
	month--;

	cmd_year[1] = keyboard->devnum;
	cmd_year[4] = year;

	cmd_day[1] = keyboard->devnum;
	cmd_day[5] = day;
	cmd_day[6] = month;

	lg_device_queue_out(keyboard->device, cmd_year, sizeof(cmd_year));
	lg_device_queue_out(keyboard->device, cmd_day, sizeof(cmd_day));

	return count;
}

static DEVICE_ATTR(date, S_IRUGO | S_IWUGO, keyboard_show_date, keyboard_store_date);

static struct attribute *keyboard_attrs[] = {
	&dev_attr_lcd_page.attr,
	&dev_attr_battery.attr,
	&dev_attr_time.attr,
	&dev_attr_date.attr,
	NULL,
};

static struct attribute_group keyboard_attr_group = {
	.name = "keyboard",
	.attrs = keyboard_attrs,
};

static void keyboard_handle_get_battery(
		struct lg_mx5500_keyboard *keyboard, const u8 *buf,
		size_t size)
{
	keyboard->battery_level = buf[4];
}

static void keyboard_handle_get_time(
		struct lg_mx5500_keyboard *keyboard, const u8 *buf,
		size_t size)
{
	keyboard->time[0] = buf[7];
	keyboard->time[1] = buf[6];
	keyboard->time[2] = buf[5];
}

static void keyboard_handle_get_date_day(
		struct lg_mx5500_keyboard *keyboard, const u8 *buf,
		size_t size)
{
	keyboard->date[1] = buf[6];
	keyboard->date[2] = buf[5];
}

static void keyboard_handle_get_date_year(
		struct lg_mx5500_keyboard *keyboard, const u8 *buf,
		size_t size)
{
	keyboard->date[0] = buf[4];
}

static void keyboard_handle_lcd_page_changed_event(
		struct lg_mx5500_keyboard *keyboard, const u8 *buf,
		size_t size)
{
	keyboard->lcd_page = buf[4];
}

static struct lg_mx5500_keyboard_handler lg_mx5500_keyboard_handlers[] = {
	{ .action = 0x0b, .first = 0x00,
		.func = keyboard_handle_lcd_page_changed_event },
	{ .action = LG_DEVICE_ACTION_GET, .first = 0x0d,
		.func = keyboard_handle_get_battery },
	{ .action = LG_DEVICE_ACTION_GET, .first = 0x31,
		.func = keyboard_handle_get_time },
	{ .action = LG_DEVICE_ACTION_GET, .first = 0x32,
		.func = keyboard_handle_get_date_day },
	{ .action = LG_DEVICE_ACTION_GET, .first = 0x33,
		.func = keyboard_handle_get_date_year },
	{ }
};

void lg_mx5500_keyboard_handle(struct lg_device *device, const u8 *buffer,
								size_t count)
{
	int i;
	int handeld = 0;
	struct lg_mx5500_keyboard *keyboard;
	struct lg_mx5500_keyboard_handler *handler;

	keyboard = lg_device_get_keyboard(device);

	for (i = 0; lg_mx5500_keyboard_handlers[i].action ||
		lg_mx5500_keyboard_handlers[i].first; i++) {
		handler = &lg_mx5500_keyboard_handlers[i];
		if (handler->action == buffer[2] &&
				handler->first == buffer[3]) {
			if (handler->func != LG_DEVICE_HANDLER_IGNORE)
				handler->func(keyboard, buffer, count);
			handeld = 1;
		}
	}

	if (!handeld)
		lg_device_err(device, "Unhandeld keyboard message %02x %02x", buffer[2], buffer[3]);

	wake_up_interruptible(&keyboard->received);
}

struct lg_mx5500_keyboard *lg_mx5500_keyboard_create(struct lg_device *device)
{
	struct lg_mx5500_keyboard *keyboard;

	keyboard = kzalloc(sizeof(*keyboard), GFP_KERNEL);
	if (!keyboard)
		return NULL;

	keyboard->device = device;
	keyboard->devnum = 1;
	keyboard->lcd_page = 0;
	keyboard->battery_level = -1;
	keyboard->initialized = 0;
	init_waitqueue_head(&keyboard->received);

	return keyboard;
}

int lg_mx5500_keyboard_init(struct lg_device *device)
{
	int ret;
	struct lg_mx5500_keyboard *keyboard;

	keyboard = lg_mx5500_keyboard_create(device);
	if (!keyboard) {
		ret = -ENOMEM;
		goto error;
	}

	ret = sysfs_create_files(&device->hdev->dev.kobj,
		(const struct attribute**)keyboard_attrs);
	if (ret)
		goto error_free;

	lg_device_set_data(device, keyboard);
	lg_device_set_hid_receive_handler(device, lg_mx5500_keyboard_handle);

	return ret;
error_free:
	kfree(keyboard);
error:
	return ret;
}

void lg_mx5500_keyboard_destroy(struct lg_mx5500_keyboard *keyboard)
{
	if (!keyboard)
		return;

	kfree(keyboard);
}

void lg_mx5500_keyboard_exit(struct lg_device *device)
{
	struct lg_mx5500_keyboard *keyboard;

	keyboard = lg_device_get_keyboard(device);

	if (keyboard == NULL)
		return;

	sysfs_remove_files(&device->hdev->dev.kobj,
		(const struct attribute **)keyboard_attrs);

	lg_mx5500_keyboard_destroy(keyboard);
}

struct lg_mx5500_keyboard *lg_mx5500_keyboard_init_on_receiver(
			struct lg_device *device,
			const u8 *buffer, size_t count)
{
	struct lg_mx5500_keyboard *keyboard;

	keyboard = lg_mx5500_keyboard_create(device);

	if (!keyboard)
		goto error;

	if (sysfs_create_group(&device->hdev->dev.kobj,
		&keyboard_attr_group))
		goto error_free;

	return keyboard;

error_free:
	kfree(keyboard);
error:
	return NULL;
}

void lg_mx5500_keyboard_exit_on_receiver(struct lg_mx5500_keyboard *keyboard)
{
	if (keyboard == NULL)
		return;

	sysfs_remove_group(&keyboard->device->hdev->dev.kobj,
				&keyboard_attr_group);

	lg_mx5500_keyboard_destroy(keyboard);
}

static struct lg_driver driver = {
	.device_id = { HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_LOGITECH,
			USB_DEVICE_ID_MX5500_KEYBOARD) },
	.init = lg_mx5500_keyboard_init,
	.exit = lg_mx5500_keyboard_exit,
	.receive_handler = lg_mx5500_keyboard_handle,
	.type = LG_MX5500_KEYBOARD,
};

struct lg_driver *lg_mx5500_keyboard_get_driver()
{
	return &driver;
}
