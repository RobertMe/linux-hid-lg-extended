hid-logitech-mx5500-y	:= hid-lg-mx5500.o hid-lg-mx5500-receiver.o hid-lg-mx5500-keyboard.o hid-lg-mx5500-mouse.o

obj-m := hid-logitech-mx5500.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean