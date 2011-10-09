#ifndef __HID_LG_EXTENDED
#define __HID_LG_EXTENDED

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifdef __KERNEL__

#include <linux/hid.h>
#include <linux/list.h>

#define USB_VENDOR_ID_LOGITECH          0x046d

#define LG_DRIVER_NO_CODE 0x00

#define LG_DEVICE_HANDLER_IGNORE NULL

#define lg_device_err(device, fmt, arg...) hid_err(device.hdev, fmt, ##arg)
#define lg_device_dbg(device, fmt, arg...) hid_dbg(device.hdev, fmt, ##arg)

struct lg_device;

typedef void (*lg_device_hid_receive_handler)(struct lg_device *device,
                      const u8 *payload, size_t size);

struct lg_driver {
    char *name;
    char *device_name;
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

enum lg_device_actions {
    LG_DEVICE_ACTION_SET = 0x80,
    LG_DEVICE_ACTION_GET = 0x81,
    LG_DEVICE_ACTION_DO = 0x83,
};

struct lg_device;

typedef void (*lg_device_hid_receive_handler)(struct lg_device *device,
                      const u8 *payload, size_t size);

struct lg_device_queue;

struct lg_device {
    struct hid_device *hdev;

    struct lg_driver *driver;

    struct lg_device_queue *out_queue;
    struct lg_device_queue *in_queue;
};

void lg_device_queue(struct lg_device *device, struct lg_device_queue *queue,
                        const u8 *buffer, size_t count);

#define lg_device_queue_out(device, buffer, count)  \
    lg_device_queue(&device, device.out_queue, buffer, count)

void lg_device_send_worker(struct work_struct *work);

void lg_device_receive_worker(struct work_struct *work);

int lg_device_event(struct hid_device *hdev, struct hid_report *report,
                u8 *raw_data, int size);

int lg_device_init(struct lg_device *device,
                    struct hid_device *hdev,
                    struct lg_driver *driver);

int lg_device_init_copy(struct lg_device *device,
                    struct lg_device *from,
                    struct lg_driver *driver);

void lg_device_destroy(struct lg_device *device);

#endif

#endif
