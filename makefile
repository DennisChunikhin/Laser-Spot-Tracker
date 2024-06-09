PYLON_DEV_DIR := C:\Program Files\Basler\pylon 6\Development
PYLON_LIB_VERSION := Win32
BUILD := .\Application

$(BUILD)\camera.exe: camera.o initialize.o processing.o
	gcc -o $(BUILD)\camera.exe initialize.o processing.o camera.o -L "$(PYLON_DEV_DIR)\lib\$(PYLON_LIB_VERSION)" -l PylonC

camera.o: camera.c initialize.h processing.h
	gcc -c camera.c -I "$(PYLON_DEV_DIR)\include"

initialize.o: initialize.c initialize.h
	gcc -c initialize.c -I "$(PYLON_DEV_DIR)\include"

processing.o: processing.c processing.h
	gcc -c processing.c -I "$(PYLON_DEV_DIR)\include"