rm sdkconfig
cp ../../sdkconfig sdkconfig
cat sdkconfig.override >> sdkconfig
# First we must run "build" because otherwise "idf.py elf" is not a valid command
idf.py build
idf.py elf
