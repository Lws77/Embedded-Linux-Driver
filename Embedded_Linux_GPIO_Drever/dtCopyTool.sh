#!/bin/bash
clear
cat <<- _EOF_
		=====================================================
		= Please Select:                                    =	
		= 1.Enter 1 to copy current device tree from system =
		= 2.Enter 2 to put the .dtbo file into overlays     =
		= 3.Press Ctrl + C for quit"                        =
		=====================================================
_EOF_

NUM0=0
NUM1=1
NUM2=2

DTBO="gpio101_demo.dtbo"
OVLAY_PATH="/boot/dtbs/4.19.193-33-rockchip-g5add69115a1c/rockchip/overlay/"

while true; do
	read -p "Enter selection [0~2] > "  INNUM 
	echo $INNUM
	if [[ $INNUM =~ ^[0-2]$ ]]
	then
		if [ $INNUM -eq $NUM0 ] 
		then
			echo "Exit"
			break
		elif [ $INNUM -eq $NUM1 ] 
		then
			echo "device tree is being copied by user..."
			dtc -I fs /proc/device-tree > new_dt
			break
		elif [ $INNUM -eq $NUM2 ] 
		then
			echo "2"
			sudo make dtc
					
			echo ./$DTBO ${OVLAY_PATH}${DTBO}
			sudo cp ./$DTBO ${OVLAY_PATH}${DTBO}
			echo "${DTBO}"
			ls -l | grep "${DTBO}"
			echo "${OVLAY_PATH}"
			ls -l ${OVLAY_PATH} | grep "${DTBO}"
					
			break
		fi
	else
		echo "Please enter number 1 or number 2 or number 0 for exit."
		continue
	fi
done