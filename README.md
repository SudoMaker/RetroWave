# RetroWave
Authentic sounds from vintage sound chips, on modern hardware!

## Introduction
RetroWave is a hardware sound board series that uses vintage sound chips and works with modern hardware.

They are in shape of Raspberry HATs, stackable, hackable, can be used on tiny single board computers like the Raspberry Pi / Jetson Nano, and modern desktop computer with USB ports.

![pic2](https://user-images.githubusercontent.com/34613827/115403406-f2bb1e80-a21e-11eb-870d-8f872d3b3b86.jpg)

![pic3](https://user-images.githubusercontent.com/34613827/115403614-2ac26180-a21f-11eb-85a6-18b21e8c10ba.jpg)

![pic1](https://user-images.githubusercontent.com/34613827/115405273-acff5580-a220-11eb-9b1d-b16763f35a67.jpg)

## Hardware

### Boards

#### OPL3
Uses the Yamaha YMF262-M chip.

![EE4A0701](https://user-images.githubusercontent.com/34613827/115404127-9b697e00-a21f-11eb-9cce-84bf7765dd9a.jpg)

#### Mini Blaster
Under development.

#### MasterGear
Under development.

... And there will be more ...

### Features
- High quality components
- In shape of a Raspberry HAT
- Uses SPI, a modern high-speed bus
- All control pins & chip registers are directly accessible by software
- Built-in crystal oscillator, no need to use an MCU
- Different RetroWave boards can be stacked together, and they use the same pins
- With a PotatoPi Lite as a USB adapter, the boards can be accessed via 12Mbps full speed USB CDC

### In depth look

#### Architecture
There are no abstraction between software and hardware at all.

A simple SPI IO expander is used, and all pins of the sound chips can be controlled by software directly.

With the PotatoPi Lite as USB adapter, you still have the ability to control all the pins directly. Since it just acts as simple "Serial to SPI" converter.

All boards have built-in crystal oscillators. For sound chips that can be used with different clock speeds, programmable oscillators will be used.

#### Pin allocation
Each board uses the same pins on the 40 pin Raspberry header (physical pin numbering):
- 5V (pin 2 and 4)
- All GND pins (pin 6, 9, 14, 20, 25, 30, 24, 39)
- SPI MOSI (pin 19)
- SPI MISO (pin 21)
- SPI CLK (pin 23)
- GPIO6 (pin 31)

Yes, the boards are stackable. And yes, different boards can function at the same time.

## Software
We provide a library for accessing the boards, a reference command-line VGM player that uses the library, and a modified DOSBox-X that supports using RetroWave boards for sound output.

All of them are free software.

### Library
Source files are in the `RetroWaveLib` directory.

#### Features
- Written in pure C
- Robust architecture using callbacks
- Easy to integrate to any project: use CMake or simply copy the files
- Provides ready-to-use platform drivers for: Linux/BSD/MacOS, Windows, and STM32 HAL

#### Problems
1. Many ARM-based Linux SBCs (including Raspberry Pi) will take a very long time locking SPI bus clock frequency if automatic CPU frequency scaling is enabled. This will lead to huge latency. In this case, please disable it (`cpufreq-set -g performance`).

### Player
Source files are in the `Player` directory.

#### Features
- Plays VGM or VGZ files
- Supports accessing the boards using serial port (on a desktop computer) and SPI (on a Linux SBC)
- Supports playback controls: Pause, Previous/Next, Fast Forward and Single Step
- Displays GD3 metadata information
- Nanoseconds accuracy on Linux/BSD and microseconds accuracy on MacOS 
- Cool real-time register map visualization!

![Screenshot_20210420_213349](https://user-images.githubusercontent.com/34613827/115404756-35312b00-a220-11eb-8dbe-0e69879cb04c.png)

#### Binary releases
See the [Releases](https://github.com/SudoMaker/RetroWave/releases) section.

Currently built targets:
- Linux x86_64
- Linux armhf
- Linux aarch64
- MacOS x86_64
- Windows x86_64 (Windows 7+)

#### Build
- Ensure you have the build tools, CMake 3.14+ and zlib dev package installed
- `cd` into the root path of this repo
- `mkdir build; cd build; cmake ..; make`

#### Problems
Currently all problems are Windows-specific.

If you are good at Windows APIs, feel free to create a pull request!

0. Emulated APIs

   Currently the Windows target uses Cygwin to emulate POSIX APIs. It works, but it may not be a good idea.


1. Possible inaccurate timing

   It seems that Windows doesn't have a monotonic self-increasing clock/timer that is unaffected by real world time changes. This may make the playback unstable. And the playback will be destroyed if a NTP time update happens in background.


2. Slow OSD

   The conhost.exe terminal is extremely laggy when repainting the whole console window. So, OSD refresh rate is set to 1 second and regmap visualization is disabled by default on Windows. If you want to see the register map visualization properly, try using MinTTY as your terminal.

### DOSBox-X
![Screenshot_20210420_004920](https://user-images.githubusercontent.com/34613827/115405808-2a2aca80-a221-11eb-8a16-93d76cd51b71.png)

Currently only OPL3 is supported.

For the source files, see [here](https://github.com/SudoMaker/dosbox-x).

Supports accessing the boards using serial port (on a desktop computer) and SPI (on a Linux SBC).

## License
All source code files in this repo are free software and use the AGPLv3 license.

Hardware design (C) 2021 SudoMaker, All rights reserved.
