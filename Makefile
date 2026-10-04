all:
	@echo "=== [1/2] Building Linux Kernel Biosensor Driver ==="
	$(MAKE) -C driver
	@echo "=== [2/2] Building C++ Biosensor Guardian Daemon ==="
	$(MAKE) -C src

driver:
	$(MAKE) -C driver

app:
	$(MAKE) -C src

load:
	@echo "=== Loading Smartwatch Kernel Module ==="
	sudo insmod driver/smart_watch_bio.ko
	sudo chmod 666 /dev/smart_watch_bio
	@dmesg | tail -n 5

unload:
	@echo "=== Unloading Smartwatch Kernel Module ==="
	sudo rmmod smart_watch_bio
	@dmesg | tail -n 5

clean:
	$(MAKE) -C driver clean
	$(MAKE) -C src clean

.PHONY: all driver app load unload clean
