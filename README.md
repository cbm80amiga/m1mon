# m1mon
Apple Silicon Macs Performance Monitor

Simple wrapper for the macOS 'powermetrics' command to monitor Apple Silicon 
performance

Tested on M1 MacBook Air, should work on M1 Pro and Max machines as well

## Binary

Download binary for macOS arm64 here

## To compile
Requires Xcode and ncurses to compile

`g++ -lncurses -o m1mon m1mon.cpp `

## Usage
`m1mon [-i interval]`

`m1mon` (default interval 1000ms)

`m1mon -i500` (use 500ms interval for powermetrics)

Start m1mon from terminal. User password will be required by powermetrics command.
Can be used for remote Mac monitoring, tested from Linux terminal and Putty on Win10/11.

## Features
- CPU utilization for each core and cluster
- GPU utilization, curre
- Each CPU core speed and throttling
- Current and maximum observed power consumption:
 - Entire SoC
 - CPU
 - Efficiency Cluster
 - Performance Cluster
 - GPU
 - RAM
 - Apple Neural Engine
 - RAM

If you find it useful and want to buy me a coffee or a beer:

https://www.paypal.me/cbm80amiga
