#!/bin/bash

cmake -S ./ -B build-sim
cmake --build build-sim --target build-tests -j 14
build-sim/Tests/TactilityCore/TactilityCoreTests --exit

