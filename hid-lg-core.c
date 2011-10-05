/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "hid-lg-core.h"
#include "hid-lg-device.h"

static struct lg_driver drivers;


int lg_probe(struct hid_device *hdev,
				const struct hid_device_id *id);

void lg_remove(struct hid_device *hdev);

struct lg_device *lg_find_device_on_lg_device(struct lg_device *device,
				       struct hid_device_id device_id)
{
	if (device->driver->device_id.bus == device_id.bus &&
		device->driver->device_id.vendor == device_id.vendor &&
		device->driver->device_id.product == device_id.product)
		return device;

	if (device->driver->find_device)
		return device->driver->find_device(device, device_id);

	return NULL;
}
EXPORT_SYMBOL_GPL(lg_find_device_on_lg_device);

struct lg_device *lg_find_device_on_device(struct device *device,
				       struct hid_device_id device_id)
{
	struct lg_device *lg_device = dev_get_drvdata(device);

	if (!lg_device)
		return NULL;

	return lg_find_device_on_lg_device(lg_device, device_id);
}
EXPORT_SYMBOL_GPL(lg_find_device_on_device);

struct lg_device *lg_create_on_receiver(struct lg_device *receiver,
					u8 device_code,
					const u8 *buffer, size_t count)
{
	struct lg_driver *driver;
	u8 found;

	list_for_each_entry(driver, &drivers.list, list)
	{
		if (driver->device_code == LG_DRIVER_NO_CODE)
			continue;

		if (driver->device_code == device_code) {
			found = 1;
			break;
		}
	}

	if (!found)
		return NULL;

	if (!driver->init_on_receiver)
		return NULL;

	return driver->init_on_receiver(receiver, buffer, count);
}
EXPORT_SYMBOL_GPL(lg_create_on_receiver);

int lg_register_driver(struct lg_driver *driver)
{
	int ret;
	struct hid_device_id *device_ids;

	device_ids = kzalloc(sizeof(struct hid_device_id) * 2, GFP_KERNEL);
	if (!device_ids) {
		ret = -ENOMEM;
		goto error;
	}

	memcpy(device_ids, &driver->device_id, sizeof(driver->device_id));
	memset(&device_ids[1], 0, sizeof(struct hid_device_id));
	driver->hid_driver.name = driver->name;
	driver->hid_driver.id_table = device_ids;
	driver->hid_driver.probe = lg_probe;
	driver->hid_driver.remove = lg_remove;
	driver->hid_driver.raw_event = lg_device_event;

	list_add(&driver->list, &drivers.list);

	ret = hid_register_driver(&driver->hid_driver);
	if (ret) {
		pr_err("Can't register %s hid driver\n", driver->name);
		goto error_free;
	}

	return 0;
error_free:
	kfree(device_ids);
error:
	return ret;
}
EXPORT_SYMBOL_GPL(lg_register_driver);

void lg_unregister_driver(struct lg_driver *driver)
{
	list_del(&driver->list);
	hid_unregister_driver(&driver->hid_driver);
	kfree(driver->hid_driver.id_table);
	if (list_empty(&driver->list))
		INIT_LIST_HEAD(&drivers.list);
}
EXPORT_SYMBOL_GPL(lg_unregister_driver);

static struct lg_driver *lg_find_driver(struct hid_device *hdev)
{
	struct lg_driver *driver;
	u8 found;

	list_for_each_entry(driver, &drivers.list, list)
	{
		if (hdev->bus == driver->device_id.bus &&
			hdev->vendor == driver->device_id.vendor &&
			hdev->product == driver->device_id.product) {
			found = 1;
			break;
		}
	}

	if (!found)
		return NULL;

	return driver;
}
EXPORT_SYMBOL_GPL(lg_find_driver);

void lg_destroy(struct lg_device *device)
{
	if (device->driver)
		device->driver->exit(device);
}

int lg_probe(struct hid_device *hdev,
				const struct hid_device_id *id)
{
	struct lg_driver *driver;
	unsigned int connect_mask = HID_CONNECT_DEFAULT;
	int ret;

	driver = lg_find_driver(hdev);
	if (!driver) {
		ret = -EINVAL;
		goto err;
	}

	ret = hid_parse(hdev);
	if (ret) {
		hid_err(hdev, "parse failed\n");
		goto err;
	}

	ret = hid_hw_start(hdev, connect_mask);
	if (ret) {
		hid_err(hdev, "hw start failed\n");
		goto err;
	}

	ret = driver->init(hdev);
	if (ret)
		goto err_stop;

	return 0;
err_stop:
	hid_hw_stop(hdev);
err:
	return ret;
}

void lg_remove(struct hid_device *hdev)
{
	struct lg_device *device = hid_get_drvdata(hdev);

	hid_hw_stop(hdev);

	if (!device)
		return;

	lg_destroy(device);
}

static int __init lg_init(void)
{
	INIT_LIST_HEAD(&drivers.list);

	return 0;
}

static void __exit lg_exit(void)
{
	struct list_head *cur, *next;

	list_for_each_safe(cur, next, &drivers.list) {
		lg_unregister_driver(list_entry(cur, struct lg_driver, list));
	}
}

module_init(lg_init);
module_exit(lg_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Robert Meijers <robert.meijers@gmail.com>");
MODULE_DESCRIPTION("Logitech HID extended driver");
MODULE_VERSION("0.1");