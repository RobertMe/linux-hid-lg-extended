#!/bin/bash

MINLEVEL=${1:-25}
FILES=$(find /sys/bus/hid/devices/*/ -name battery)

if [ -z "$DBUS_SESSION_BUS_ADDRESS" ]
then
	if [ -z "$DISPLAY" ]
	then
		export $(grep DISPLAY -z /proc/$(ps U $(id -u) | grep dbus-daemon | head -1 | cut -d' ' -f2)/environ)
	fi
	export $(grep ^DBUS_SESSION_BUS_ADDRESS $HOME/.dbus/session-bus/$(dbus-uuidgen --get)-$(echo $DISPLAY | sed -e 's/\([^:]*:\)//g' -e 's/\..*$//g'))
fi

for FILE in $FILES
do
	LEVEL=$(cat $FILE)
	LEVEL=${LEVEL//%/}
	if [ $LEVEL -lt $MINLEVEL ]
	then
		NAME=$(cat $(dirname $FILE)/name)
		notify-send "Battery level low" "The batteries of device ${NAME} are low. Only ${LEVEL}% remaining" -t 5000
	fi
done
