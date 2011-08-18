#ifndef __HID_LG_MX5500
#define __HID_LG_MX5500

#include <linux/hid.h>
#include <linux/workqueue.h>

#define LG_MX5500_BUFSIZE 32

#define lg_mx5500_err(device, fmt, arg...) hid_err(device->hdev, fmt, ##arg)
#define lg_mx5500_dbg(device, fmt, arg...) hid_dbg(device->hdev, fmt, ##arg)

enum lg_mx5500_actions {
	LG_MX5500_ACTION_SET = 0x80,
	LG_MX5500_ACTION_GET = 0x81,
	LG_MX5500_ACTION_DO = 0x83,
};

enum lg_mx5500_device {
	LG_MX5500_RECEIVER,
	LG_MX5500_KEYBOARD,
	LG_MX5500_MOUSE,
};

struct lg_mx5500_buf {
	u8 data[HID_MAX_BUFFER_SIZE];
	size_t size;
};

struct lg_mx5500_queue {
	spinlock_t qlock;
	u8 head;
	u8 tail;
	struct lg_mx5500_buf queue[LG_MX5500_BUFSIZE];
	struct work_struct worker;
};

struct lg_mx5500;

typedef void (*lg_mx5500_hid_receive_handler)(struct lg_mx5500 *device,
					  const u8 *payload, size_t size);

struct lg_mx5500 {
	struct hid_device *hdev;

	void *data;
	enum lg_mx5500_device type;

	struct lg_mx5500_queue out_queue;
	struct lg_mx5500_queue in_queue;

	lg_mx5500_hid_receive_handler receive_handler;
};

void lg_mx5500_queue(struct lg_mx5500 *device, struct lg_mx5500_queue *queue,
						const u8 *buffer, size_t count);

#define lg_mx5500_queue_out(device, buffer, count)	\
	lg_mx5500_queue(device, &device->out_queue, buffer, count)

void lg_mx5500_send_worker(struct work_struct *work);

void lg_mx5500_receive_worker(struct work_struct *work);

void lg_mx5500_set_data(struct lg_mx5500 *device, void *data);

void *lg_mx5500_get_data(struct lg_mx5500 *device);

void lg_mx5500_set_hid_receive_handler(struct lg_mx5500 *device,
					lg_mx5500_hid_receive_handler receive_handler);

#endif