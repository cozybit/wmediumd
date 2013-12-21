#! /bin/bash

function cleanup() {
	read
	for i in ${NETNS_PID[*]}
	do
		kill $i
	done
}

NUM_PHYS=2
modprobe -rq mac80211_hwsim
modprobe mac80211_hwsim radios=$NUM_PHYS

i=0
# Assume most recently modified phy's are hwsim phys (hence the ls -t)
for phy in `ls -t /sys/class/ieee80211`; do
	if [ $i == 0 ]
	then
		# The usual stuff
		iw phy ${phy} interface add mesh${i} type mesh
		ifconfig mesh${i} hw ether 42:00:00:00:0${i}:00
		ip address add dev mesh${i} 192.168.77.$((10 + i))/24
		ip link set mesh${i} up
		iw dev mesh${i} mesh join bazooka
	else
		# This interface will live under a separate network namespace
		# bound to a different shell
		echo iw phy ${phy} set netns \$BASHPID > mesh${i}_up_in_netns.sh
		echo iw phy ${phy} interface add mesh${i} type mesh >> mesh${i}_up_in_netns.sh
		echo ifconfig mesh${i} hw ether 42:00:00:00:0${i}:00 >> mesh${i}_up_in_netns.sh
		echo ip address add dev mesh${i} 192.168.77.$((10 + i))/24 >> mesh${i}_up_in_netns.sh
		echo ip link set mesh${i} up >> mesh${i}_up_in_netns.sh
		echo iw dev mesh${i} mesh join bazooka >> mesh${i}_up_in_netns.sh
		echo sleep 10000 >> mesh${i}_up_in_netns.sh

		unshare -n -- /bin/bash mesh${i}_up_in_netns.sh & &> /dev/null
		NETNS_PID[$i]=$!
	fi

	i=$((i+1))
	[ $i == $NUM_PHYS ] && break
done

# Test 

# Only mesh0 should exist in this namespace
ifconfig | grep mesh0 &> /dev/null || { echo FAIL; cleanup; exit -1; }
ifconfig | grep mesh1 && { echo FAIL; cleanup; exit -1; }

ping 192.168.77.11 -W 5 -c 1 | grep '1 received' &> /dev/null || { echo FAIL; cleanup; exit -1; }

echo PASS
