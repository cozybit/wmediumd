#! /bin/bash
showmenu ()
{
    	typeset ii
	for ii in "${options[@]}"
	do
	    echo " $ii"
	done
	read -e -p 'Select an action : ---------- ' -a answer
}

typeset -r c1=#2--Create_new_network_namespace
typeset -r c2=#3--After_Kill_echo_PID
typeset -r c3=#7--Configure_already_asigned_interface
typeset -r c4=#11-Do_PINGs_to_the_other_station_192.168.4.2 
typeset -r c5=#13-TEST_capture_traffic_on_the_interface
typeset -r c6=#-1--Quit
typeset -ra options=($c1 $c2 $c3 $c4 $c5 $c6)
typeset -a answer
typeset -i kk

while true
do
    showmenu
    for kk in "${answer[@]}"
    do
	case $kk in
	2)
	    echo 'Create new network namespace. Script will day call it again using next step. ./first_terminal.sh'
		echo './first_terminal.sh'
		sudo unshare -n bash
	    ;;
	3)
		echo '##########'
		echo $$
		echo '##########'
		echo 'DONE'
	    ;;
	7)
	    echo 'Configure already asigned interface'
		read -e -p 'Number of first phy interface: ' -a phy1
		sudo iwconfig 
		sudo iw phy phy$phy1 interface add mesh1 type mesh 
		sudo ifconfig 
		sudo ip address add dev mesh1 192.168.4.1/24
		sudo ip link set mesh1 up
		sudo iw dev mesh1 mesh join bazooka
		sudo ifconfig lo up
		sudo ifconfig 
		sudo ping -c 20 192.168.4.2
		sudo ifconfig 
		echo 'DONE' 
	    ;;
	11)
	    echo 'Doing pings to 192.168.4.2'
	    ping 192.168.4.2
	    ;;
	13)
		echo 'TEST capture traffic on interface'
		read -e -p 'Number of first phy interface: ' -a phy1
		sudo iw phy phy$phy1 interface add mon1 type monitor
        sudo iw dev mon1 set channel 1 HT40+
        sudo ip link set mon1 up
        sudo tcpdump -i mon1 -p -w /tmp/mon1_phy1.cap &
        #ping -c 50 192.168.4.2
		mgen input send_mgen.mgn
		;;
	-1)
	    echo 'Program Exit'
	    exit 0
	    ;;	
	esac
    done 
done
