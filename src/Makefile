hid-logitech-core-y	:= hid-lg-core.o hid-lg-device.o
hid-logitech-mx5500-y	:= hid-lg-mx5500.o hid-lg-mx5500-receiver.o hid-lg-mx5500-keyboard.o hid-lg-mx-revolution.o
hid-logitech-vx-revolution-y := hid-lg-vx-revolution.o

obj-m := hid-logitech-core.o
obj-m += hid-logitech-mx5500.o
obj-m += hid-logitech-vx-revolution.o

ccflags-y += -I$(src)/include/

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install

headers_install:
	install -m 0644 include/linux/hid-lg-extended.h $(KDIR)/include/linux/hid-lg-extended.h
