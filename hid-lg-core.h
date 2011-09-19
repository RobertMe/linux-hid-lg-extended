#ifndef __HID_LG_CORE
#define __HID_LG_CORE

#include <linux/hid.h>
#include <linux/list.h>

#define USB_VENDOR_ID_LOGITECH          0x046d
#define USB_DEVICE_ID_MX5500_RECEIVER   0xc71c
#define USB_DEVICE_ID_MX5500_KEYBOARD   0xb30b
#define USB_DEVICE_ID_MX5500_MOUSE      0xb007

#define LG_DRIVER_NO_CODE 0x00

struct lg_device;

enum lg_devices {
	LG_MX5500_RECEIVER,
	LG_MX5500_KEYBOARD,
	LG_MX5500_MOUSE,
};

typedef void (*lg_device_hid_receive_handler)(struct lg_device *device,
					  const u8 *payload, size_t size);

struct lg_driver {
	enum lg_devices type;
	char *name;
	struct hid_device_id device_id;
	struct hid_driver hid_driver;
	u8 device_code;

	int (*init)(struct hid_device *hdev);
	void (*exit)(struct lg_device *device);
	lg_device_hid_receive_handler receive_handler;
	struct lg_device *(*find_device)(struct lg_device *device,
					 struct hid_device_id device_id);

	struct list_head list;
};

struct lg_device *lg_find_device_on_lg_device(struct lg_device *device,
				       struct hid_device_id device_id);

struct lg_device *lg_find_device_on_device(struct device *device,
				       struct hid_device_id device_id);

int lg_register_driver(struct lg_driver *driver);

void lg_unregister_driver(struct lg_driver *driver);

#endif
