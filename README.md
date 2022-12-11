Proc_Monitor
===========================

A wrapper program to monitor the stack/heap usage of a program, command, or process.


Usage
---------------------------

`./proc_monitor [-p | -g] [Command | PID]`
- `-p`:
  - If present, the subsequent argument is treated as the PID to be monitored.
  - Otherwise, the subsequent arguments are treated as a command, the launched program/command will be run and monitored.
- `-g`:
  - if the -g flag is present, a web-page will be launched in your preferred browser and the stack and heap usage will be plotted

If stdout is able to be captured (from a command or program), it will be displayed in a box within the console output using UTF-8 characters


Dependencies
---------------------------
- System:
  - Debian Linux (May be able to modify CMakeLists.txt and Boost package configuration for other distros/OSs)
  - Support for UTF-8 characters within console
  - CMake >= 3.5
  - C++14 compiler

- Packages:
  - libssl-dev
  - libboost-filesystem-dev
  - libboost-thread-dev

Can be installed via: `sudo apt-get install libssl-dev libboost-filesystem-dev libboost-thread-dev`

Build
----------------------------
- `cd [top-level directory]`
- `mkdir build`
- `cd build`
- `cmake ..`
- `make`
