#ifndef __HID_LG_MX5500_KEYBOARD
#define __HID_LG_MX5500_KEYBOARD

#include <linux/hid.h>

#include "hid-lg-device.h"
#include "hid-lg-mx5500-receiver.h"

struct lg_driver *lg_mx5500_keyboard_get_driver(void);

#endif