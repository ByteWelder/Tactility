#!/bin/sh

cmake -S ./ -B build-sim
cmake --build build-sim --target build-tests -j 14
build-sim/Tests/TactilityCore/TactilityCoreTests --exit
build-sim/Tests/Tactility/TactilityTests --exit

