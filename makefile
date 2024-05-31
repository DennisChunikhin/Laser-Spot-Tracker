PYLON_DEV_DIR := C:\Program Files\Basler\pylon 6\Development
PYLON_LIB_VERSION := Win32
BUILD := .\Application

$(BUILD)\camera.exe: camera.o
	gcc -o $(BUILD)\camera.exe camera.o -L "$(PYLON_DEV_DIR)\lib\$(PYLON_LIB_VERSION)" -l PylonC

camera.o: camera.c
	gcc -c camera.c -I "$(PYLON_DEV_DIR)\include"