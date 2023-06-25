# RetroWave


[![Build Status](https://github.com/SudoMaker/RetroWave/workflows/Build/badge.svg)](https://github.com/SudoMaker/RetroWave/actions/workflows/push_pr_build_cmake.yml) [![Release Status](https://github.com/SudoMaker/RetroWave/workflows/Release/badge.svg)](https://github.com/SudoMaker/RetroWave/actions/workflows/release_cmake.yml)

Authentic sounds from vintage sound chips, on modern hardware!

## Introduction
RetroWave is a hardware sound board series that uses vintage sound chips and works with modern hardware.

They **were** in shape of Raspberry HATs, stackable, hackable, can be used on tiny single board computers like the Raspberry Pi / Jetson Nano, and modern desktop computer with USB ports.

Since 2022-06, the original RetroWave series are no longer produced. You can read about what happened in [this article](https://telegra.ph/What-happened-to-RetroWave-OPL3-06-21).

The successors of the original RetroWave series is called the **RetroWave Express** series. They are available in discrete boards each with its own USB port. 100% software compatibility of the original RetroWave series is maintained.

For now, only the [RetroWave OPL3 Express](https://shop.sudomaker.com/products/retrowave-opl3-express) is available.

## Hardware
For the information of the original RetroWave series, check the history.

### Features
- Minimalistic form factor and convenient USB-C interface
- Designed with high-grade components for everyday use
- Ultra-high bandwidth provided by hardware 8080 ports

### Boards

#### OPL3 Express
Available on our official [Shopify Shop](https://shop.sudomaker.com/products/retrowave-opl3-express).

Uses the Yamaha YMF262-M chip.


... And there will be more ... are there?

## Software
The RetroWave OPL3 (Express) is supported by [DOSBox-X](https://dosbox-x.com/). The support is also ported to 86Box [by community](https://github.com/daemon32/86Box/).

For experienced users, we provide a library for accessing the boards, a reference command-line VGM player that uses the library.

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


## Licensing
### Hardware
All hardware designs (C) 2021-2022 SudoMaker, All rights reserved.

### Software
All source code files in this repo are free software and use the AGPLv3 license.

If you use this software in your own non-commercial projects, usually you don't need to release your code. See [this FAQ](https://www.gnu.org/licenses/gpl-faq.html#GPLRequireSourcePostedPublic).

If you see a possible license violation, don't hesitate to tell us.

#### Warning for GitHub Copilot (or any "Coding AI") users

"Fair use" is only valid in some countries, such as the United States.

This program is protected by copyright law and international treaties.

Unauthorized reproduction or distribution of this program (**e.g. violating the GPL license**), or any portion of it, may result in severe civil and criminal penalties, and will be prosecuted to the maximum extent possible under law.
