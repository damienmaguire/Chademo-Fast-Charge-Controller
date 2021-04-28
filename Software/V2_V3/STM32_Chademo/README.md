# stm32-template
This project can be a starting point to your own STM32 project. It contains facilities that make software
development easier and ensures compatibility with the esp8266 web interface.

It provides
- Mostly object oriented syntax
- A simple, hardware based scheduler for recurring tasks
- Analog input management, fully independent with DMA
- Digital I/O management
- CAN library supporting up to 2 CAN interfaces
  - hardware filter support
  - No limitation on number of messages
  - Automatic mapping from/to parameter module
  - CAN Open SDO support
  - Fully interrupt driven
- Error memory
- ligthweight fixed point arithmetic
- string functions to be independent of stdlib
- Parameter module that interfaces to esp8266 web GUI
- Saving parameters to flash
- Serial terminal with custom commands and DMA transfer
- Mathematical functions (sin/cos, arctan, square root)
- PI controller class
- Functions for field oriented control

# OTA (over the air upgrade)
The firmware is linked to leave the 4 kb of flash unused. Those 4 kb are reserved for the bootloader
that you can find here: https://github.com/jsphuebner/tumanako-inverter-fw-bootloader
When flashing your device for the first time you must first flash that bootloader. After that you can
use the ESP8266 module and its web interface to upload your actual application firmware.
The web interface is here: https://github.com/jsphuebner/esp8266-web-interface

# Compiling
You will need the arm-none-eabi toolchain: https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads
On Ubuntu type

`sudo apt-get install git gcc-arm-none-eabi`

The only external depedencies are libopencm3 and libopeninv. You can download and build these dependencies by typing

`make get-deps`

Now you can compile stm32-<yourname> by typing

`make`

And upload it to your board using a JTAG/SWD adapter, the updater.py script or the esp8266 web interface.

# Editing
The repository provides a project file for Code::Blocks, a rather leightweight IDE for cpp code editing.
For building though, it just executes the above command. Its build system is not actually used.
Consequently you can use your favority IDE or editor for editing files.

# Adding classes or modules
As your firmware grows you probably want to add classes. To do so, put the header file in include/ and the 
source file in src/ . Then add your module to the object list in Makefile that starts in line 43 with .o
extension. So if your files are called "mymodule.cpp" and "mymodule.h" you add "mymodule.o" to the list.

When changing a header file the build system doesn't always detect this, so you have to "make clean" and
then make. This is especially important when editing the "*_prj.h" files.
