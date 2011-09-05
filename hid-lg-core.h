#ifndef __HID_LG_CORE
#define __HID_LG_CORE

#include <linux/hid.h>
#include <linux/list.h>

#define USB_VENDOR_ID_LOGITECH          0x046d
#define USB_DEVICE_ID_MX5500_RECEIVER   0xc71c
#define USB_DEVICE_ID_MX5500_KEYBOARD   0xb30b
#define USB_DEVICE_ID_MX5500_MOUSE      0xb007

struct lg_device;

enum lg_devices {
	LG_MX5500_RECEIVER,
	LG_MX5500_KEYBOARD,
	LG_MX5500_MOUSE,
};

typedef void (*lg_device_hid_receive_handler)(struct lg_device *device,
					  const u8 *payload, size_t size);

struct lg_driver {
	struct hid_device_id device_id;
	int (*init)(struct lg_device *device);
	void (*exit)(struct lg_device *device);
	lg_device_hid_receive_handler receive_handler;
	enum lg_devices type;

	struct list_head list;
};

void lg_add_driver(struct lg_driver *driver);

#endif
