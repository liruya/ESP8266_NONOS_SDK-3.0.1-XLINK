#!/bin/bash

echo "gen_misc.sh version 20150511"
echo ""

led_fw_version=1
socket_fw_version=1
monsoon_fw_version=1

echo "choose product type(1=EXOTerraStrip, 2=EXOTerraSocket, 3=EXOTerraMonsoon)"
echo "enter(1/2/3, default 1):"
read input

if [ -z "$input" ]; then
	prdt_type=1
	fw_version=$led_fw_version
    user_bin_name=exoterra_strip
elif [ $input == 1 ]; then
	prdt_type=1
	fw_version=$led_fw_version
    user_bin_name=exoterra_strip
elif [ $input == 2 ]; then
	prdt_type=2
	fw_version=$socket_fw_version
    user_bin_name=exoterra_socket
elif [ $input == 3 ]; then
	prdt_type=3
	fw_version=$monsoon_fw_version
    user_bin_name=exoterra_monsoon
else
	prdt_type=1
	fw_version=$led_fw_version
	user_bin_name=exoterra_strip
fi
echo ""

echo "Please follow below steps(1-5) to generate specific bin(s):"
echo "STEP 1: choose boot version(0=boot_v1.1, 1=boot_v1.2+, 2=none)"
echo "enter(0/1/2, default 2):"

boot=new
echo "boot mode: $boot"
echo ""


echo "STEP 2: choose bin generate(0=eagle.flash.bin+eagle.irom0text.bin, 1=user1.bin, 2=user2.bin)"
echo "enter (0/1/2, default 0):"

val=`expr $fw_version % 2`
if [ $val == 1 ]; then
	app=1
else
	app=2
fi
echo ""


echo "STEP 3: choose spi speed(0=20MHz, 1=26.7MHz, 2=40MHz, 3=80MHz)"
echo "enter (0/1/2/3, default 2):"

spi_speed=40
echo "spi speed: $spi_speed MHz"
echo ""


echo "STEP 4: choose spi mode(0=QIO, 1=QOUT, 2=DIO, 3=DOUT)"
echo "enter (0/1/2/3, default 0):"

spi_mode=QIO
echo "spi mode: $spi_mode"
echo ""


echo "STEP 5: choose spi size and map"
echo "    0= 512KB( 256KB+ 256KB)"
echo "    2=1024KB( 512KB+ 512KB)"
echo "    3=2048KB( 512KB+ 512KB)"
echo "    4=4096KB( 512KB+ 512KB)"
echo "    5=2048KB(1024KB+1024KB)"
echo "    6=4096KB(1024KB+1024KB)"
echo "    7=4096KB(2048KB+2048KB) not support ,just for compatible with nodeMCU board"
echo "    8=8192KB(1024KB+1024KB)"
echo "    9=16384KB(1024KB+1024KB)"
echo "enter (0/2/3/4/5/6/7/8/9, default 0):"

spi_size_map=3
echo ""


touch user/user_main.c

echo ""
echo "start..."
echo ""

make COMPILE=gcc BOOT=$boot APP=$app SPI_SPEED=$spi_speed SPI_MODE=$spi_mode SPI_SIZE_MAP=$spi_size_map PRDT_TYPE=$prdt_type USER_BIN_NAME=$user_bin_name FW_VERSION=$fw_version
