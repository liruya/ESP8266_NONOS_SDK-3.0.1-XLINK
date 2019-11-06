#！/bin/bash

echo "choose product type(1=EXOTerraStrip, 2=EXOTerraSocket, 3=EXOTerraMonsoon)"
echo "enter(1/2/3, default 1):"
read input

if [ -z "$input" ]; then
    user_bin_name=exoterra_strip
elif [ $input == 1 ]; then
    user_bin_name=exoterra_strip
elif [ $input == 2 ]; then
    user_bin_name=exoterra_socket
elif [ $input == 3 ]; then
    user_bin_name=exoterra_monsoon
else
	user_bin_name=exoterra_strip
fi
name=$user_bin_name"_v1.bin"
echo "$name"

path=bin/$user_bin_name

esptool.py --port /dev/ttyUSB0 -b 230400 write_flash 0x1000 $path/$name
