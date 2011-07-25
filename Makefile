obj-m := hid-lg-mx5500.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(CMAKE) -C $(KDIR) M=$(PWD) clean