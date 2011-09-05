#include "hid-lg-core.h"
#include "hid-lg-mx5500-keyboard.h"
#include "hid-lg-mx5500-receiver.h"
#include "hid-lg-mx-revolution.h"

static struct lg_driver drivers;

void lg_add_driver(struct lg_driver *driver)
{
	list_add(&driver->list, &drivers.list);
}

static struct lg_driver *lg_find_driver(struct hid_device *hdev)
{
	struct lg_driver *driver;
	list_for_each_entry(driver, &drivers.list, list)
	{
		if (hdev->product == driver->product_id)
			break;
	}

	if (hdev->product != driver->product_id)
		return NULL;

	return driver;
}

void lg_destroy(struct lg_device *device)
{
	if (device->driver)
		device->driver->exit(device);

	lg_device_destroy(device);
}

int lg_probe(struct hid_device *hdev,
				const struct hid_device_id *id)
{
	struct lg_device *device;
	struct lg_driver *driver;
	unsigned int connect_mask = HID_CONNECT_DEFAULT;
	int ret;

	driver = lg_find_driver(hdev);
	if (!driver) {
		ret = -EINVAL;
		goto err;
	}

	device = lg_device_create(hdev, driver);
	if (!device) {
		hid_err(hdev, "Can't alloc device\n");
		ret = -ENOMEM;
		goto err;
	}

	ret = hid_parse(hdev);
	if (ret) {
		hid_err(hdev, "parse failed\n");
		goto err_free;
	}

	ret = hid_hw_start(hdev, connect_mask);
	if (ret) {
		hid_err(hdev, "hw start failed\n");
		goto err_free;
	}

	ret = driver->init(device);
	if (ret)
		goto err_stop;

	return 0;
err_stop:
	hid_hw_stop(hdev);
err_free:
	lg_destroy(device);
err:
	return ret;
}

void lg_remove(struct hid_device *hdev)
{
	struct lg_device *device = hid_get_drvdata(hdev);

	if (!device)
		return;

	hid_hw_stop(hdev);

	lg_destroy(device);
}

static const struct hid_device_id lg_hid_devices[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_LOGITECH,
			USB_DEVICE_ID_MX5500_RECEIVER) },
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_LOGITECH,
			USB_DEVICE_ID_MX5500_KEYBOARD) },
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_LOGITECH,
			USB_DEVICE_ID_MX5500_MOUSE) },
	{ }
};

MODULE_DEVICE_TABLE(hid, lg_hid_devices);

static struct hid_driver lg_hid_driver = {
	.name = "MX5500",
	.id_table = lg_hid_devices,
	.probe = lg_probe,
	.remove = lg_remove,
	.raw_event = lg_device_event,
};


static int __init lg_init(void)
{
	int ret;

	INIT_LIST_HEAD(&drivers.list);

	lg_add_driver(lg_mx5500_keyboard_get_driver());
	lg_add_driver(lg_mx5500_receiver_get_driver());
	lg_add_driver(lg_mx_revolution_get_driver());

	ret = hid_register_driver(&lg_hid_driver);
	if (ret)
		pr_err("Can't register mx5500 hid driver\n");

	return ret;
}

static void __exit lg_exit(void)
{
	hid_unregister_driver(&lg_hid_driver);
}

module_init(lg_init);
module_exit(lg_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Robert Meijers <robert.meijers@gmail.com>");
MODULE_DESCRIPTION("Logitech MX5500 sysfs information");
MODULE_VERSION("0.1");