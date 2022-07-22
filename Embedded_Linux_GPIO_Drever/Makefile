KERNEL_DIR  ?= /usr/lib/modules/$(shell uname -r)/build
MODULE_NAME = gpioled_Drv

obj-m := $(MODULE_NAME).o

DTC=dtc
DTC_FLAGS=-@ -I dts -O dtb -o
DT_NAME=gpio101_demo
DTS=$(DT_NAME).dts
DTBO=$(DT_NAME).dtbo

all:
	make -C $(KERNEL_DIR) M=$(PWD) modules;

dtc: 
	$(DTC) $(DTC_FLAGS) $(DTBO) $(DTS)

clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean;
	rm *.dtbo


