# Laser Spot Tracker
An application for tracking a laser spot using a Basler camera.

The executable and drivers in the Application folder were compiled and tested on Windows 7 x86 with Pylon 6.

# Building on Windows
The `PYLON_DEV_DIR` variable in [makefile](makefile) must be set to the file path to the Development directory of Pylon.  
The executable can be built with the `make` or `mingw32-make` utility provided by MinGW. This places camera.exe in the [Application](Application) directory.  
The dll files from the Pylon Runtime/x64 or Runtime/Win32 (depending on your windows version) directory must be copied into the same folder as `camera.exe` so that the executable can run (see [Pylon Deployment Guide](https://docs.baslerweb.com/pylonapi/pylon-deployment-guide#locating-the-pylon-dlls)).
