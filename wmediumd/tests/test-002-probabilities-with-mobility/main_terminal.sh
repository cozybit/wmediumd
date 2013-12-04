#! /bin/bash

WIFI_SSID="WLAN_JV"
WIFI_KEY="A5BF7B01D44B13DB4670A8CA36"
#See http://askubuntu.com/questions/138472/how-do-i-connect-to-a-wpa-wifi-network-using-the-command-linen for WPA wiFi

PATH_WMEDIUMD=$(cd ../.. && pwd)

showmenu ()
{
    typeset ii
	for ii in "${options[@]}"
	do
	    echo " $ii"
	done
	read -e -p 'Select an action : ---------- ' -a answer
}

typeset -r c1=#0--Stop_network_manager_and_configuring_network
typeset -r c2=#1--Load_emuled_radios_with_mac80211_hwsim
typeset -r c3=#6--Asign_radios_to_network_namespaces
typeset -r c4=#9--Clean_and_COMPILE_wmediumd
typeset -r c5=#10-LAUNCH_wmediumd_probabilities
typeset -r c6=#12-LAUNCH_wmediumd_mobility
typeset -r c7=#-1--Quit
typeset -ra options=($c1 $c2 $c3 $c4 $c5 $c6 $c7)
typeset -a answer
typeset -i kk

while true
do
    showmenu
    for kk in "${answer[@]}"
    do
	case $kk in
	0)
	    echo 'Stop network manager and configuring network'
		sudo /etc/init.d/network-manager stop
		echo 2
		sudo ifconfig wlan0 up
		echo 3
		#sudo iwconfig wlan0 essid $WIFI_SSID
		echo 4
		sleep 3 
		#sudo iwconfig wlan0 essid $WIFI_SSID key $WIFI_KEY
		#sleep 3
		#sudo iwconfig wlan0 essid $WIFI_SSID key $WIFI_KEY
		sudo wpa_supplicant -B -iwlan0 -c/etc/wpa_supplicant.conf -Dwext && dhclient wlan0
		echo 5
		sudo dhclient wlan0
		sleep 3
		ping -c 5 8.8.8.8
		echo 'DONE'
	    ;;
	1)
	    echo 'Load emuled radios with mac80211_hwsim'
		sudo rmmod mac80211_hwsim  
		sudo modprobe mac80211_hwsim radios=2  
		echo '##########'
		sudo find /sys/kernel/debug/ieee80211 -name hwsim | cut -d/ -f 6 | sort
		echo '##########'
		echo 'DONE' 
	    ;;
	6)
	    echo 'Asign radios to network namespaces'
		read -e -p 'Number of first phy interface: ' -a phy1
		read -e -p 'PID of first network namespace: ' -a pid1
		phy2=$(($phy1 + 1))
		read -e -p 'PID of second network namespace: ' -a pid2
		sudo iw phy phy$phy1 set netns $pid1
		sudo iw phy phy$phy2 set netns $pid2
		echo 'DONE'
	    ;;
	9)
	    echo 'Compile wmediumd'
		echo 'START CLEAN'
		make clean -C $PATH_WMEDIUMD
		echo 'END CLEAN'
		echo '=============================================================='
		echo 'START COMPILE'
	    make -C $PATH_WMEDIUMD
		echo 'END COMPILE'
		echo '=============================================================='
	    ;;
	10)
	    echo 'Launch wmediumd with probabilities'
	    sudo $PATH_WMEDIUMD/wmediumd -c $PATH_WMEDIUMD/cfg-examples/probabilities.cfg
	    ;;
	12)
	    echo 'Launch wmediumd with mobility'
	    sudo $PATH_WMEDIUMD/wmediumd -m $PATH_WMEDIUMD/tests/test-002-probabilities-with-mobility/mobility.cfg
	    ;;
	-1)
	    echo 'Program Exit'
	    exit 0
	    ;;
	esac
    done 
done
