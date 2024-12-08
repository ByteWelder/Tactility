# External Apps

## Building

Steps to build:

- Build the main project with `idf.py build` in the root folder of the project
- Open a terminal at `ExternalApps/${AppName}`
- The first time, run `./build.sh` (during consecutive runs, you can run `idf.py build`)
- Find the executable `build/${AppName}.app.elf` and copy it to an SD card.

## Developing apps

Currently, only C is supported. The `TactilityC` project converts Tactility's C++ code into C.

Methods need to be manually exposed to the ELF apps, by editing `TactilityC/Source/TactilityC.cpp`

You can put apps in `Data/config` or `Data/assets` during development, because putting it on an SD card takes more effort.
