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

typeset -r c1=#4--Create_new_network_namespace
typeset -r c2=#5--After_Kill_echo_PID
typeset -r c3=#8--Configure_already_asigned_interface
typeset -r c4=#11-Do_PINGs_to_the_other_station_192.168.4.1
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
	4)
	    echo 'Create new network namespace. Script will day call it again using next step. ./second_terminal.sh'
		echo './second_terminal.sh'
		sudo unshare -n bash
	    ;;
	5)
		echo '##########'
		echo $$
		echo '##########'
		echo 'DONE'
	    ;;
	8)
	    echo 'Configure already asigned interface'
		read -e -p 'Number of second phy interface: ' -a phy2
		iwconfig 
		sudo iw phy phy$phy2 interface add mesh2 type mesh
		sudo ip address add dev mesh2 192.168.4.2/24
		sudo ip link set mesh2 up
		sudo iw dev mesh2 mesh join bazooka
		sudo ifconfig lo up
		sudo ping -c 10 192.168.4.1
		sudo ifconfig 
		echo 'DONE' 
	    ;;
	11)
	    echo 'Doing pings to 192.168.4.1'
	    ping 192.168.4.1
	    ;;
	13)
		echo 'TEST capture traffic on interface'
		read -e -p 'Number of second phy interface: ' -a phy2
		sudo iw phy phy$phy2 interface add mon2 type monitor
        sudo iw dev mon2 set channel 1 HT40+
        sudo ip link set mon2 up
        mgen input listen_mgen.mgn &
        sudo tcpdump -i mon2 -p -w /tmp/mon2_phy2.cap
		;;
	-1)
	    echo 'Program Exit'
	    exit 0
	    ;;
	esac
    done 
done
