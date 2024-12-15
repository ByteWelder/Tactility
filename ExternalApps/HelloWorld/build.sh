rm sdkconfig
cp ../../sdkconfig sdkconfig
cat sdkconfig.override >> sdkconfig
idf.py elf_app
