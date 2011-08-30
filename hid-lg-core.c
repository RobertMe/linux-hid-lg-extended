#include "hid-lg-core.h"
#include "hid-lg-mx5500-keyboard.h"
#include "hid-lg-mx5500-receiver.h"
#include "hid-lg-mx-revolution.h"

static struct lg_driver drivers;

void lg_add_driver(struct lg_driver *driver)
{
	list_add(&driver->list, &drivers.list);
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
	.probe = lg_device_hid_probe,
	.remove = lg_device_hid_remove,
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