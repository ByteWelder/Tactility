#!/bin/sh

# Usage:
#   flash.sh [port]
#
# Arguments:
#   port - the port of the device (e.g. /dev/ttyUSB0, ...)
#
# Requirements:
#   jq - run 'pip install jq'
#   esptool.py - run 'pip install esptool'
#
# Documentation:
#   https://docs.espressif.com/projects/esptool/en/latest/esp32/
#

# Source: https://stackoverflow.com/a/53798785
function is_bin_in_path {
  builtin type -P "$1" &> /dev/null
}

function require_bin {
  program=$1
  tip=$2
  if ! is_bin_in_path $program; then
    echo -e "\e[31m⚠️ $program not found!\n\t$tip\e[0m"
  fi
}

require_bin esptool.py "install esptool from your package manager or install python and run 'pip install esptool'"
require_bin jq "install jq from your package manager or install python and run 'pip install jq'"

if [[ $1 -eq 0 ]]; then
    echo -e "\e[31m⚠️ Must Specify port as argument. For example:\n\tflash.sh /dev/ttyACM0\n\tflash.sh /dev/ttyUSB0\e[0m"
    exit -1
fi

cd Binaries
# Create flash command based on partitions
KEY_VALUES=`jq -r '.flash_files  | keys[] as $k | "\($k) \(.[$k])"' flasher_args.json  | tr "\n" " "`
esptool.py --port $1 erase_flash
esptool.py --port $1 --connect-attempts 10 -b 460800 write_flash $KEY_VALUES
