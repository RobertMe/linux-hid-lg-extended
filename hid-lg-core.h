#ifndef __HID_LG_CORE
#define __HID_LG_CORE

#include <linux/hid.h>
#include <linux/list.h>

#define USB_VENDOR_ID_LOGITECH          0x046d

#define LG_DRIVER_NO_CODE 0x00

struct lg_device;

typedef void (*lg_device_hid_receive_handler)(struct lg_device *device,
					  const u8 *payload, size_t size);

struct lg_driver {
	char *name;
	struct hid_device_id device_id;
	struct hid_driver hid_driver;
	u8 device_code;

	int (*init)(struct hid_device *hdev);
	struct lg_device *(*init_on_receiver)(struct lg_device *receiver,
						const u8 *buffer, size_t count);
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

struct lg_device *lg_create_on_receiver(struct lg_device *receiver,
					u8 device_code,
					const u8 *buffer, size_t count);

int lg_register_driver(struct lg_driver *driver);

void lg_unregister_driver(struct lg_driver *driver);

#endif
