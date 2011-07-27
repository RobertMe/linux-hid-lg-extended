#include <linux/device.h>
#include <linux/hid.h>
#include <linux/module.h>
#include <linux/spinlock.h>

#define USB_VENDOR_ID_LOGITECH          0x046d
#define USB_DEVICE_ID_MX5500_RECEIVER   0xc71c

#define LG_MX5500_BUFSIZE 32

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

struct lg_mx5500_keyboard {
};

struct lg_mx5500_mouse {
};

struct lg_mx5500_receiver {
	struct hid_device *hdev;

	struct lg_mx5500_queue out_queue;
	struct lg_mx5500_queue in_queue;

	struct lg_mx5500_keyboard *keyboard;
	struct lg_mx5500_mouse *mouse;
};

static ssize_t lg_mx5500_hid_send(struct hid_device *hdev, u8 *buffer,
								size_t count)
{
	u8 *buf;
	ssize_t ret;

	if (!hdev->hid_output_raw_report)
		return -ENODEV;

	buf = kmemdup(buffer, count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	ret = hdev->hid_output_raw_report(hdev, buf, count, HID_OUTPUT_REPORT);

	kfree(buf);
	return ret;
}

static void lg_mx5500_send_worker(struct work_struct *work)
{
	struct lg_mx5500_queue *queue = container_of(work, struct lg_mx5500_queue,
									worker);
	struct lg_mx5500_receiver *receiver = container_of(queue, struct lg_mx5500_receiver,
									out_queue);
	unsigned long flags;

	spin_lock_irqsave(&queue->qlock, flags);

	while (queue->head != queue->tail) {
		spin_unlock_irqrestore(&queue->qlock, flags);
		lg_mx5500_hid_send(receiver->hdev, queue->queue[queue->tail].data,
						queue->queue[queue->tail].size);
		spin_lock_irqsave(&queue->qlock, flags);

		queue->tail = (queue->tail + 1) % LG_MX5500_BUFSIZE;
	}

	spin_unlock_irqrestore(&queue->qlock, flags);
}

static void lg_mx5500_hid_receive(struct hid_device *hdev, u8 *buffer,
								size_t count)
{
}

static void lg_mx5500_receive_worker(struct work_struct *work)
{
	struct lg_mx5500_queue *queue = container_of(work, struct lg_mx5500_queue,
									worker);
	struct lg_mx5500_receiver *receiver = container_of(queue, struct lg_mx5500_receiver,
									in_queue);
	unsigned long flags;

	spin_lock_irqsave(&queue->qlock, flags);

	while (queue->head != queue->tail) {
		spin_unlock_irqrestore(&queue->qlock, flags);
		lg_mx5500_hid_receive(receiver->hdev, queue->queue[queue->tail].data,
						queue->queue[queue->tail].size);
		spin_lock_irqsave(&queue->qlock, flags);

		queue->tail = (queue->tail + 1) % LG_MX5500_BUFSIZE;
	}

	spin_unlock_irqrestore(&queue->qlock, flags);
}

static void lg_mx5500_queue(struct lg_mx5500_receiver *receiver, struct lg_mx5500_queue *queue, const u8 *buffer,
								size_t count)
{
	unsigned long flags;
	u8 newhead;

	if (count > HID_MAX_BUFFER_SIZE) {
		hid_warn(receiver->hdev, "Sending too large output report\n");
		return;
	}

	spin_lock_irqsave(&queue->qlock, flags);

	memcpy(queue->queue[queue->head].data, buffer, count);
	queue->queue[queue->head].size = count;
	newhead = (queue->head + 1) % LG_MX5500_BUFSIZE;

	if (queue->head == queue->tail) {
		queue->head = newhead;
		schedule_work(&queue->worker);
	} else if (newhead != queue->tail) {
		queue->head = newhead;
	} else {
		hid_warn(receiver->hdev, "Queue is full");
	}

	spin_unlock_irqrestore(&queue->qlock, flags);
}

static int lg_mx5500_event(struct hid_device *hdev, struct hid_report *report,
				u8 *raw_data, int size)
{
	struct lg_mx5500_receiver *receiver;

	if(report->id < 0x10)
	{
		return 0;
	}

	receiver = hid_get_drvdata(hdev);
	
	lg_mx5500_queue(receiver, &receiver->in_queue, raw_data, size);

	return 0;
}

static struct lg_mx5500_receiver *lg_mx5500_receiver_create(struct hid_device *hdev)
{
	struct lg_mx5500_receiver *receiver;

	receiver = kzalloc(sizeof(*receiver), GFP_KERNEL);
	if(!receiver)
		return NULL;

	receiver->hdev = hdev;
	receiver->keyboard = NULL;
	receiver->mouse = NULL;
	hid_set_drvdata(hdev, receiver);

	spin_lock_init(&receiver->out_queue.qlock);
	INIT_WORK(&receiver->out_queue.worker, lg_mx5500_send_worker);
	
	spin_lock_init(&receiver->in_queue.qlock);
	INIT_WORK(&receiver->in_queue.worker, lg_mx5500_receive_worker);

	return receiver;
}

static void lg_mx5500_receiver_destroy(struct lg_mx5500_receiver *receiver)
{
	kfree(receiver);
}

static int lg_mx5500_hid_probe(struct hid_device *hdev,
				const struct hid_device_id *id)
{
	struct lg_mx5500_receiver *receiver;
	unsigned int connect_mask = HID_CONNECT_DEFAULT;
	int ret;

	receiver = lg_mx5500_receiver_create(hdev);
	if(!receiver)
	{
		hid_err(hdev, "Can't alloc device\n");
		return -ENOMEM;
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

	return 0;
err_free:
	lg_mx5500_receiver_destroy(receiver);
	return ret;
}

static void lg_mx5500_hid_remove(struct hid_device *hdev)
{
	struct lg_mx5500_receiver *receiver = hid_get_drvdata(hdev);
	
	hid_hw_stop(hdev);
	lg_mx5500_receiver_destroy(receiver);
}

static const struct hid_device_id lg_mx5500_hid_devices[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_LOGITECH,
			USB_DEVICE_ID_MX5500_RECEIVER) },
	{ }
};

MODULE_DEVICE_TABLE(hid, lg_mx5500_hid_devices);

static struct hid_driver lg_mx5500_hid_driver = {
	.name = "MX5500",
	.id_table = lg_mx5500_hid_devices,
	.probe = lg_mx5500_hid_probe,
	.remove = lg_mx5500_hid_remove,
	.raw_event = lg_mx5500_event,
};

static int __init lg_mx5500_init(void)
{
	int ret;

	ret = hid_register_driver(&lg_mx5500_hid_driver);
	if (ret)
		pr_err("Can't register mx5500 hid driver\n");

	return ret;
}

static void __exit lg_mx5500_exit(void)
{
	hid_unregister_driver(&lg_mx5500_hid_driver);
}

module_init(lg_mx5500_init);
module_exit(lg_mx5500_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Robert Meijers <robert.meijers@gmail.com>");
MODULE_DESCRIPTION("Logitech MX5500 sysfs information");
MODULE_VERSION("0.1");
