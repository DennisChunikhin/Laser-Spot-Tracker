# Laser Spot Tracker
An application for tracking a laser spot using a Basler camera.

The executable and drivers in the Application folder were compiled and tested on Windows 7 x86 with Pylon 6 and GTK 4.

# Installation
## GTK 4
The GUI is built on GTK 4, which can be installed on Windows 7 through MSYS2. The last MSYS2 release that supports windows 7 can be installed from [https://github.com/msys2/msys2-installer/releases/tag/2022-10-28](https://github.com/msys2/msys2-installer/releases/tag/2022-10-28). Once installed, use the Mingw-w64 shell. Add the Mingw-w64 shell binaries (`C:\msys64\mingw64\bin`) to the system PATH variable.

GTK 4 installation is detailed at [https://www.gtk.org/docs/installations/windows/#using-gtk-from-msys2-packages](https://www.gtk.org/docs/installations/windows/#using-gtk-from-msys2-packages). To install, open the MSYS2 Mingw-w64 shell and run:

`pacman -S mingw-w64-ucrt-x86_64-gtk4`

If not already installed, install `pkg-config` by running:

`pacman -S mingw-w64-x86_64-pkg-config`

## Pylon 6
Download the Pylon 6 Software Suite from [https://www2.baslerweb.com/en/downloads/software-downloads/software-pylon-6-3-0-windows/](https://www2.baslerweb.com/en/downloads/software-downloads/software-pylon-6-3-0-windows/).

## Building
If git is not installed, install it by running:

`pacman -S git`

Clone this repository by running:

`git clone https://github.com/DennisChunikhin/Laser-Spot-Tracker`

Update the `PYLON_DEV_DIR` variable in [makefile](makefile) to the file path to the Development directory of Pylon (most likely `C:\Program Files\Basler\pylon 6\Development`).

Navigate into the the directory with `cd Laser-Spot-Tracker`. Compile the executable by running `make` or `mingw32-make`. This places `app.exe` in the [Application](Application) directory.

The dll files from `C:\Program Files\Basler\pylon 6\Runtime\Runtime\x64` or `C:\Program Files\Basler\pylon 6\Runtime\Win32` (depending on your windows version--by default the makefile is set to compile for x64) must be copied into the same folder as `app.exe` so that the executable can run (see [Pylon Deployment Guide](https://docs.baslerweb.com/pylonapi/pylon-deployment-guide.html#locating-the-pylon-dlls)). Alternatively, the aforementioned file path can be added to the system PATH variable.

The application can now be started by running `app.exe`
