#!/bin/bash

if [[ $EUID -ne 0 ]]
then
	echo "Binding the devices required root privileges"
	exit 1
fi

function bind
{
	devicepaths=$(ls -d1 /sys/bus/hid/devices/$1* 2>/dev/null)
	driver=$2
	for devicepath in $devicepaths
	do
		device=$(basename $devicepath)
		echo $device > $devicepath/driver/unbind
		echo $device > /sys/bus/hid/drivers/$driver/bind

		echo Bound device $device to $driver driver
	done
}

bind 0003:046D:C71C logitech-mx5500-receiver
bind 0005:046D:B007 logitech-mx-revolution
bind 0005:046D:B30B logitech-mx5500
bind 0003:046D:C521 logitech-vx-revolution