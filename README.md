# m1mon
Apple Silicon Macs Performance Monitor

A simple wrapper for the macOS `powermetrics` command to monitor Apple Silicon SoC performance

Tested on M1 MacBook Air, should work on M1 Pro and M1 Max machines as well

![MBA](https://github.com/cbm80amiga/m1mon/blob/main/screenshots/m1mon1.jpg)

## Binary

Download binary for macOS arm64 [here](https://github.com/cbm80amiga/m1mon/releases/download/v1/m1mon)

## To compile
Requires Xcode and ncurses to compile

`g++ -lncurses -o m1mon m1mon.cpp `

## Usage
`m1mon [-i interval]`

`m1mon` (default interval 1000ms)

`m1mon -i500` (use 500ms interval for powermetrics)

Start m1mon from macOS terminal. User password will be required by powermetrics command.

Can be used for remote monitoring of Mac computers, tested with Linux terminal and Putty on Windows PC.

![Putty](https://github.com/cbm80amiga/m1mon/blob/main/screenshots/m1mon-putty.jpg)

## Features
- CPU utilization for each core and cluster
- GPU utilization
- Each CPU core speed and possible throttling
- Current and maximum observed power consumption:
  - Entire SoC
  - CPU
  - Efficiency Cluster
  - Performance Cluster
  - GPU
  - Apple Neural Engine
  - RAM

If you find it useful and want to buy me a coffee or a beer:

https://www.paypal.me/cbm80amiga
