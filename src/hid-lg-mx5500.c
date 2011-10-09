/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <linux/hid-lg-extended.h>

#include "hid-lg-mx5500.h"
#include "hid-lg-mx5500-keyboard.h"
#include "hid-lg-mx5500-receiver.h"
#include "hid-lg-mx-revolution.h"

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

static int __init lg_mx5500_init(void)
{
	lg_register_driver(lg_mx5500_keyboard_get_driver());
	lg_register_driver(lg_mx5500_receiver_get_driver());
	lg_register_driver(lg_mx_revolution_get_driver());

	return 0;
}

static void __exit lg_mx5500_exit(void)
{
	lg_unregister_driver(lg_mx5500_keyboard_get_driver());
	lg_unregister_driver(lg_mx5500_receiver_get_driver());
	lg_unregister_driver(lg_mx_revolution_get_driver());
}

module_init(lg_mx5500_init);
module_exit(lg_mx5500_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Robert Meijers <robert.meijers@gmail.com>");
MODULE_DESCRIPTION("Logitech MX5500 sysfs information");
MODULE_VERSION("0.1");