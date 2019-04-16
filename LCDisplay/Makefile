TARGET_MODULE := lcdisplay

BUILDSYSTEM_DIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

obj-m += $(TARGET_MODULE).o

# Run kernel build system to make module
all:
	$(MAKE) -C $(BUILDSYSTEM_DIR) M=$(PWD) modules

# Run kernel build system to cleanup in current directory
clean:
	$(MAKE) -C $(BUILDSYSTEM_DIR) M=$(PWD) clean

# Load the module
load:
	insmod ./$(TARGET_MODULE).ko
	sudo chmod 777 load.sh
	sudo ./load.sh

# Unload the module
unload:
	rmmod ./$(TARGET_MODULE).ko
	sudo chmod 777 unload.sh
	sudo ./unload.sh

# Compile and run the application test
run:
	gcc $(APPLICATION).c -o $(APPLICATION) -pthread -Wall
	./$(APPLICATION)
