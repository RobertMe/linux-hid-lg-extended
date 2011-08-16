#ifndef __HID_LG_MX5500_RECEIVER
#define __HID_LG_MX5500_RECEIVER

#include <linux/hid.h>

#include "hid-lg-mx5500.h"

struct lg_mx5500_receiver;

int lg_mx5500_receiver_init(struct lg_mx5500 *device);

void lg_mx5500_receiver_exit(struct lg_mx5500 *device);

#endif