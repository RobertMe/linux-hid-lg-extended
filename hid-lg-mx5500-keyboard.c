#include <linux/sched.h>
#include <linux/wait.h>

#include "hid-lg-mx5500-keyboard.h"

struct lg_mx5500_keyboard {
	struct lg_device device;
	wait_queue_head_t received;
	u8 devnum;
	u8 initialized;
	struct attribute_group attr_group;

	short battery_level;
	short lcd_page;
	short time[3];
	short date[3];
};

int lg_mx5500_keyboard_init_new(struct hid_device *hdev);

struct lg_device *lg_mx5500_keyboard_init_on_receiver(
			struct lg_device *device,
			const u8 *buffer, size_t count);

void lg_mx5500_keyboard_exit(struct lg_device *device);

void lg_mx5500_keyboard_handle(struct lg_device *device, const u8 *buffer,
								size_t count);

static struct lg_driver driver = {
	.name = "Logitech MX5500",
	.device_id = { HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_LOGITECH,
			USB_DEVICE_ID_MX5500_KEYBOARD) },
	.device_code = 0xb3,

	.init = lg_mx5500_keyboard_init_new,
	.init_on_receiver = lg_mx5500_keyboard_init_on_receiver,
	.exit = lg_mx5500_keyboard_exit,
	.receive_handler = lg_mx5500_keyboard_handle,
};

#define get_on_lg_device(device) container_of( 				\
			lg_find_device_on_lg_device(device, driver.device_id),\
			struct lg_mx5500_keyboard, device)

#define get_on_device(device) container_of( 				\
			lg_find_device_on_device(device, driver.device_id),\
			struct lg_mx5500_keyboard, device)

struct lg_mx5500_keyboard_handler {
	u8 action;
	u8 first;
	void (*func)(struct lg_mx5500_keyboard *keyboard, const u8 *payload, size_t size);
};

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
	struct lg_mx5500_keyboard *keyboard = get_on_device(device);

	if (lg_mx5500_keyboard_request_battery(keyboard))
		return 0;

	return scnprintf(buf, PAGE_SIZE, "%d%%\n", keyboard->battery_level);
}

static DEVICE_ATTR(battery, S_IRUGO, keyboard_show_battery, NULL);

static ssize_t keyboard_show_lcd_page(struct device *device,
			struct device_attribute *attr, char *buf)
{
	struct lg_mx5500_keyboard *keyboard;

	keyboard = get_on_device(device);

	return scnprintf(buf, PAGE_SIZE, "%d\n", keyboard->lcd_page);
}

static DEVICE_ATTR(lcd_page, S_IRUGO, keyboard_show_lcd_page, NULL);

static ssize_t keyboard_show_name(struct device *device,
			struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%s\n", driver.name);
}

static DEVICE_ATTR(name, S_IRUGO, keyboard_show_name, NULL);

static ssize_t keyboard_show_time(struct device *device,
		struct device_attribute *attr, char *buf)
{
	struct lg_mx5500_keyboard *keyboard = get_on_device(device);

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

	keyboard = get_on_device(device);
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
	struct lg_mx5500_keyboard *keyboard = get_on_device(device);

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

	keyboard = get_on_device(device);
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
	&dev_attr_battery.attr,
	&dev_attr_date.attr,
	&dev_attr_lcd_page.attr,
	&dev_attr_name.attr,
	&dev_attr_time.attr,
	NULL,
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

	keyboard = get_on_lg_device(device);

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
		lg_device_err((*device), "Unhandeld keyboard message %02x %02x", buffer[2], buffer[3]);

	wake_up_interruptible(&keyboard->received);
}

struct lg_mx5500_keyboard *lg_mx5500_keyboard_create(char *name)
{
	struct lg_mx5500_keyboard *keyboard;

	keyboard = kzalloc(sizeof(*keyboard), GFP_KERNEL);
	if (!keyboard)
		return NULL;

	keyboard->devnum = 1;
	keyboard->lcd_page = 0;
	keyboard->battery_level = -1;
	keyboard->initialized = 0;
	keyboard->attr_group.name = name;
	keyboard->attr_group.attrs = keyboard_attrs;
	init_waitqueue_head(&keyboard->received);

	return keyboard;
}

int lg_mx5500_keyboard_init_new(struct hid_device *hdev)
{
	int ret;
	struct lg_mx5500_keyboard *keyboard;

	keyboard = lg_mx5500_keyboard_create(NULL);
	if (!keyboard) {
		ret = -ENOMEM;
		goto error;
	}

	ret = lg_device_init(&keyboard->device, hdev, &driver);
	if (ret)
		goto error_free;

	ret = sysfs_create_group(&keyboard->device.hdev->dev.kobj,
		&keyboard->attr_group);
	if (ret)
		goto error_free;


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

	lg_device_destroy(&keyboard->device);
	kfree(keyboard);
}

void lg_mx5500_keyboard_exit(struct lg_device *device)
{
	struct lg_mx5500_keyboard *keyboard;

	keyboard = get_on_lg_device(device);

	if (keyboard == NULL)
		return;

	sysfs_remove_group(&device->hdev->dev.kobj,
		&keyboard->attr_group);

	lg_mx5500_keyboard_destroy(keyboard);
}

struct lg_device *lg_mx5500_keyboard_init_on_receiver(
			struct lg_device *device,
			const u8 *buffer, size_t count)
{
	struct lg_mx5500_keyboard *keyboard;

	keyboard = lg_mx5500_keyboard_create("keyboard");

	if (!keyboard)
		goto error;

	if (lg_device_init_copy(&keyboard->device, device, &driver))
		goto error_free;

	if (sysfs_create_group(&keyboard->device.hdev->dev.kobj,
		&keyboard->attr_group))
		goto error_free;

	return &keyboard->device;

error_free:
	lg_mx5500_keyboard_destroy(keyboard);
error:
	return NULL;
}

struct lg_driver *lg_mx5500_keyboard_get_driver()
{
	return &driver;
}
