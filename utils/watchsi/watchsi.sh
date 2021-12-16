#!/bin/bash

# Test sub?
/usr/bin/mosquitto_sub -h localhost -t '#' -C 1 >/dev/null 2>&1
if test $? -ne 0; then
	echo "[$(date)] mosquitto_sub not working correctly"
	exit  1
fi

while true
do
	data=$(/usr/bin/mosquitto_sub -h localhost -t 'SolarD/Inverter/si/Data' -C 1 -W 22)
	if test -z "$data"; then
		echo "[$(date)] restarting SI"
		killall si
	else
		bv=$(echo "$data" | jq '.battery_voltage')
		if test $(echo "$bv < 10" | bc -l) -eq 1; then
			echo "[$(date)] voltage: $bv"
			killall si
		fi
	fi
done
