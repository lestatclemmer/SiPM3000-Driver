# Bridgeport 3000 series USB breakout

## Update 2022
Migrated to `CMake`. To compile: `cd source-code && mkdir build && cd build && cmake .. && make`

## Intro
This repository contains C++ source to interface with the MCA-3000 boards from Bridgeport Instruments, as well as some tools to make the setup of this interface easier.

Not all detector  commands have been implemented. I've only written code for `fpga_ctrl`, `fpga_action`, `arm_ctrl`, `arm_version`, `fgpa_list_mode`, `fpga_histogram`, and a shell for `fpga_time_slice`. If you want more commands added, either add them following the program structure outlined below, or ask UMN and we can help adapt some new classes.

## Required libraries
`libusb-1.0` is the only required library to run this program. On macOS, Homebrew should install `libusb-1.0` correctly. I've had issues using package managers to install `libusb-1.0` on Linux, so I recommend [downloading the source from the website](https://libusb.info/) and installing it by hand. The file called `INSTALL` in the source tarball describes how to install the library.

Another library that comes packaged *only with Unix systems* is called `libdl`. This allows for the loading of dynamic libraries, `.so` files on Linux. This library is only required to use the simulator because the simulator is stored as a `.so` file.

## Running the example program
~~Out of the box, if you plug in the detector, `cd src && make && sudo ./main`, the `main` program will run and collect a spectrum from the detector using some `fpga_ctrl` and `arm_ctrl` settings that were used at EXACT's flight selection review (FSR).~~ (See above compilation instructions). Compiling the program requires GNU Make (and CMake), installed by default on Linux and macOS. The program will only compile on Unix platforms due to the simulator considerations above.

If you really want to use this interface on Windows, you'll have to figure out how to install `libusb-1.0` on Windows and link to it, and then delete the `SimManager` files. Because of this, I would recommend either using a dedicated Linux computer or even a Linux VM in Windows to use this software.

If you want to use the software on macOS, just realize the simulator won't work. Mike doesn't provide simulator files for macOS.

If you want to run the simulator, uncomment the lines in `source-code/main.cc` to switch from `SipmUsb::UsbManager man;` to `SipmUsb::SimManager man;`. The `lib` folder **must** be named `lib`, and it must be placed in the same directory from which you run the program. For example, if `main` is located in `src`, and you run it by doing `./source-code/main`, the `lib` folder must be in the folder that corresponds to your current directory (i.e. the one above `src`). The simulator is very fragile because Mike did not intend for it to be used by others in this way I believe.


## Program structure
The structure of the code to interface with the 3000 series detectors is somewhat complex because the communication protocol is nontrivial. There are two main chunks: the USB or simulator interfacing, and the commands that can be sent. The structure is object-oriented and employs templates and run-time polymorphism. I'll first outline the interfacing classes, and then the command classes.

First: the whole API is within the `namespace SipmUsb`. If the code is ever extended, please keep everything in this `namespace`.

### Interface classes
There are three interfacing classes: `BaseManager`, `UsbManager`, and `SimManager`. `UsbManager` handles communication with the detector over an actual USB connection. `SimManager` can communicate with the Bridgeport simulator, which is stored in the `lib` folder in a file called `sipm_3k_simusb.so`. `BaseManager` is an abstract base class that defines the public interface of both `SimManager` and `UsbManager`. `SimManager` and `UsbManager` both inherit from `BaseManager`. 

There are two things you can do with the `UsbManager` and `SimManager` classes. The first is to read from the detector, the second is to write settings or data to it. Say we have some `UsbManager` object called `man`. Then to read from the detector, you call `man.read_into(serial_number, io_con);`. To write data to the detector, you call `man.write_from(serial_number, io_con);`. The `serial_number` is a `std::string` that uniquely identifies the detector; see `source-code/main.cc` for an example of how to retrieve that serial number. `io_con` is an `IoContainer` object, which holds data either read from or to be written to the MCA-3000. Data is *written from* the `IoContainer` and *read into* it, hence the method naming. (Internally, the `IoContainer` object holds three buffers: one for write data, one for read data, and one for command data, but the user doesn't have to touch those buffers directly.)

### Command classes
I mention above the `IoContainer` class. `IoContainer` is an abstract base class that holds auto-managed buffers to facilitate easy USB transfer and thus detector (or simulator) communication. It allows for encoding/decoding of USB bytes to the appropriate `Registers` data type. Please see `source-code/main.cc` and read the section below on updating settings to know better what i mean by `Registers`.

In any event, the structure of the detector commands is as follows: each command is a subclass of `IoContainer`. For example, `ListModeContainer` inherits from `IoContainer`, so it gets all of the stuff `IoContainer` has and then some.

The public interface of `IoContainer` allows for updating what is written to the detector (method `update_write_args`), updating the direction of data flow (method `update_transfer_flags`), and construction of the appropriate data type `Registers` from the read or write bytes buffers (method `registers_from_buffer`).

Specific commands have further functionality. Look at the function `parse_list_buffer` in `source-code/io-containers/ListModeContainer.cc` to see the custom behavior specific to list mode. `parse_list_buffer` takes the `Registers` generated by `registers_from_buffer` and parses them to a `std::vector` of list mode data points, very akin to whwat Mike does.


I refer you to `source-code/main.cc` for an example of how this all fits together.


## Updating settings
Rather than encoding and decoding JSON messages, all settings are sent and received as *registers*. These registers are exactly the same as what Mike lays out in the Bridgeport MDS documentation. For an example, look at [fpga\_ctrl](http://bridgeportinstruments.com/products/software/wxMCA_doc/documentation/english/mds/mca3k/mca3k_fpga_ctrl.html). There are 15 registers that hold various settings, many of which are related to gain calibration.

Now, the current scheme to extract registers is a bit convoluted, but it's a low-effort solution on my (William's) part. Basically, you use the Bridgeport-provided software (like the GUI, or a Python script with the MCA Data Server), get settings you like, and save them to a JSON file, like `extract-registers/fsr_settings.json`. This file has information on the *fields* of each data structure. The `fields` in Mike language are human-readable versions of the registers.

However, we don't want to have to decode human-readable text to pass to the detector--we just want registers! So, the script `extract-registers/extract_registers.py` will extract those registers for you, using the `mca3k_data.py` file provided by Bridgeport to decode the fields into registers. `extract_registers.py` is quite short, so it might be useful to read through it.

Now, the process to get the registers you want is pretty simple:
1. Find settings you like using Mike's software.
2. Save the settings to a JSON file.
3. Run `extract_registers.py` to pull the registers out for each data structure.
4. Put the resulting registers into your code using the appropriate subclass of `IoContainer`'s registers.

Again, see `source-code/main.cc` for an example of this in action.
