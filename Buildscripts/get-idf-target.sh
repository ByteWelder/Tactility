#!/bin/sh

config_idf_target=`cat sdkconfig | grep CONFIG_IDF_TARGET=`
echo ${config_idf_target:19:-1}