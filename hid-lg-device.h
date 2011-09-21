#ifndef __HID_LG_DEVICE
#define __HID_LG_DEVICE

#include <linux/hid.h>
#include <linux/workqueue.h>

#include "hid-lg-core.h"

#define LG_DEVICE_BUFSIZE 32

#define LG_DEVICE_HANDLER_IGNORE NULL

#define lg_device_err(device, fmt, arg...) hid_err(device.hdev, fmt, ##arg)
#define lg_device_dbg(device, fmt, arg...) hid_dbg(device.hdev, fmt, ##arg)

enum lg_device_actions {
	LG_DEVICE_ACTION_SET = 0x80,
	LG_DEVICE_ACTION_GET = 0x81,
	LG_DEVICE_ACTION_DO = 0x83,
};

struct lg_device_buf {
	u8 data[HID_MAX_BUFFER_SIZE];
	size_t size;
};

struct lg_device_queue {
	spinlock_t qlock;
	u8 head;
	u8 tail;
	struct lg_device_buf queue[LG_DEVICE_BUFSIZE];
	struct work_struct worker;

	struct lg_device *main_device;
};

struct lg_device;

typedef void (*lg_device_hid_receive_handler)(struct lg_device *device,
					  const u8 *payload, size_t size);

struct lg_device {
	struct hid_device *hdev;

	struct lg_driver *driver;

	struct lg_device_queue *out_queue;
	struct lg_device_queue *in_queue;
};

void lg_device_queue(struct lg_device *device, struct lg_device_queue *queue,
						const u8 *buffer, size_t count);

#define lg_device_queue_out(device, buffer, count)	\
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