#!/bin/bash

echo 21 > /sys/class/gpio/export

while [ 1 ]; do
	current=`cat /sys/class/gpio/gpio21/value`
	if [ $current == "0" ]; then
		./makePhoto.sh
	fi

	sleep 0.2
done
