#include <linux/hid.h>
#include <linux/module.h>

#define USB_VENDOR_ID_LOGITECH          0x046d
#define USB_DEVICE_ID_MX5500_RECEIVER   0xc71c

struct lg_mx5500_keyboard {
};

struct lg_mx5500_mouse {
};

struct lg_mx5500_receiver {
	struct hid_device *hdev;
	
	struct lg_mx5500_keyboard *keyboard;
	struct lg_mx5500_mouse *mouse;
};

static int lg_mx5500_event(struct hid_device *hdev, struct hid_report *report,
				u8 *raw_data, int size)
{
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
