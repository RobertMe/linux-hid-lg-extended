#include "hid-lg-mx5500-keyboard.h"

struct lg_mx5500_keyboard {
	struct lg_mx5500 *device;
	u8 devnum;
	u8 initialized;

	short lcd_page;
};

struct lg_mx5500_keyboard_handler {
	u8 action;
	u8 first;
	void (*func)(struct lg_mx5500_keyboard *keyboard, const u8 *payload, size_t size);
};

static inline struct lg_mx5500_keyboard *lg_mx5500_keyboard_get_from_device(
			struct device *device)
{
	return lg_mx5500_get_keyboard(dev_get_drvdata(device));
}

static ssize_t keyboard_show_lcd_page(struct device *device,
			struct device_attribute *attr, char *buf)
{
	struct lg_mx5500_keyboard *keyboard;

	keyboard = lg_mx5500_keyboard_get_from_device(device);

	return scnprintf(buf, PAGE_SIZE, "%d\n", keyboard->lcd_page);
}

static DEVICE_ATTR(lcd_page, S_IRUGO, keyboard_show_lcd_page, NULL);

static struct attribute *keyboard_attrs[] = {
	&dev_attr_lcd_page.attr,
	NULL,
};

static struct attribute_group keyboard_attr_group = {
	.name = "keyboard",
	.attrs = keyboard_attrs,
};

static void keyboard_handle_lcd_page_changed_event(
		struct lg_mx5500_keyboard *keyboard, const u8 *buf,
		size_t size)
{
	keyboard->lcd_page = buf[4];
}

static struct lg_mx5500_keyboard_handler lg_mx5500_keyboard_handlers[] = {
	{ .action = 0x0b, .first = 0x00,
		.func = keyboard_handle_lcd_page_changed_event },
	{ }
};

void lg_mx5500_keyboard_handle(struct lg_mx5500 *device, const u8 *buffer,
								size_t count)
{
	int i;
	int handeld = 0;
	struct lg_mx5500_keyboard *keyboard;
	struct lg_mx5500_keyboard_handler *handler;

	keyboard = lg_mx5500_get_keyboard(device);

	for(i = 0; lg_mx5500_keyboard_handlers[i].action ||
		lg_mx5500_keyboard_handlers[i].first; i++) {
		handler = &lg_mx5500_keyboard_handlers[i];
		if(handler->action == buffer[2] &&
				handler->first == buffer[3]) {
			handler->func(keyboard, buffer, count);
			handeld = 1;
		}
	}

	if(!handeld)
		lg_mx5500_err(device, "Unhandeld keyboard message %02x %02x", buffer[2], buffer[3]);
}

struct lg_mx5500_keyboard *lg_mx5500_keyboard_create_on_receiver(
			struct lg_mx5500 *device,
			const u8 *buffer, size_t count)
{
	struct lg_mx5500_keyboard *keyboard;

	keyboard = kzalloc(sizeof(*keyboard), GFP_KERNEL);
	if(!keyboard)
		goto error;

	keyboard->device = device;
	keyboard->devnum = buffer[1];
	keyboard->lcd_page = 0;
	keyboard->initialized = 0;

	if(sysfs_create_group(&device->hdev->dev.kobj,
		&keyboard_attr_group))
		goto error_free;

	return keyboard;

error_free:
	kfree(keyboard);
error:
	return NULL;
}

void lg_mx5500_keyboard_destroy(struct lg_mx5500_keyboard *keyboard)
{
	if(keyboard == NULL)
		return;

	sysfs_remove_group(&keyboard->device->hdev->dev.kobj,
				&keyboard_attr_group);
	kfree(keyboard);
}