#
# Makefile for OS Protector project.
#
all: shadow_box_module shadow_box_helper_module os_protector_daemon
.ONESHELL:
shadow_box_module: 
	@echo 
	@echo ==================================
	@echo Build Shadow-Box Module...
	@echo ==================================
	cd shadow_box
	if [ ! -d "system.map" ]; then mkdir system.map; fi
	cp /boot/System.map-`uname -r` "system.map/`uname -v`.map"
	make -j
	cp shadow_box.ko ../../usr/share/gooroom/security/os-protector
	cd ..

shadow_box_helper_module:
	@echo 
	@echo ==================================
	@echo Build Shadow-Box Helper Module...
	@echo ==================================
	cd shadow_box_helper
	make -j
	cp shadow_box_helper.ko ../../usr/share/gooroom/security/os-protector
	cd ..

os_protector_daemon:
	@echo 
	@echo ==================================
	@echo Build OS Protector Daemon...
	@echo ==================================
	cd daemon
	make -j
	cp gop-helper ../../usr/share/gooroom/security/os-protector
	cd ..

clean:
	make -C shadow_box clean
	make -C shadow_box_helper clean
	make -C daemon clean
