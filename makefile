PYLON_DEV_DIR := C:/Program Files/Basler/pylon 6/Development
PYLON_LIB_VERSION := x64
BUILD := ./Application

FLAGS = -Wall -Wextra `pkg-config --cflags gtk4`

$(BUILD)/app.exe: gui.o camera.o initialize.o processing.o
	gcc $(flags) -o $(BUILD)/app.exe initialize.o processing.o camera.o gui.o -L "$(PYLON_DEV_DIR)/lib/$(PYLON_LIB_VERSION)" -l PylonC `pkg-config --libs gtk4`

gui.o: gui.c camera.h
	gcc $(flags) `pkg-config --cflags gtk4` -c gui.c -I "$(PYLON_DEV_DIR)/include"

camera.o: camera.c camera.h initialize.h processing.h
	gcc $(flags) `pkg-config --cflags gtk4` -c camera.c -I "$(PYLON_DEV_DIR)/include"

initialize.o: initialize.c initialize.h
	gcc $(flags) -c initialize.c -I "$(PYLON_DEV_DIR)/include"

processing.o: processing.c processing.h
	gcc $(flags) -c processing.c -I "$(PYLON_DEV_DIR)/include"
