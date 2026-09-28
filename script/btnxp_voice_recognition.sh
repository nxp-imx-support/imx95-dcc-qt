#!/bin/sh -x
ACL_HANDLE_SOURCE_DEVICE=0
SCO_HANDLE_SOURCE_DEVICE=8

bvra_start() {

sleep 2

hcitool -i hci0 cmd 0x3F 0x0070 0x01
hcitool -i hci0 cmd 0x3F 0x001D 0x01
hcitool -i hci0 cmd 0x3F 0x0007 0x00
hcitool -i hci0 cmd 0x3F 0x0028 0x03 0x00 0x04
hcitool -i hci0 cmd 0x3F 0x0029 0x04 0x00
hcitool -i hci0 cmd 0x3F 0x73 0x01

/usr/share/dcc/bluealsa-rfcomm "/org/bluealsa/hci0/dev_$1" <<EOF
+BVRA:1
EOF

# create sco
echo "Establishing eSCO WBS for ACL Handle: "$ACL_HANDLE_SOURCE_DEVICE
hcitool -i hci0 cmd 0x01 0x3d $ACL_HANDLE_SOURCE_DEVICE 0x00 0x40 0x1f 0x00 0x00 0x40 0x1f 0x00 0x00 0x05 0x00 0x00 0x00 0x00 0x05 0x00 0x00 0x00 0x00 0x3c 0x00 0x3c 0x00 0x00 0x7d 0x00 0x00 0x00 0x7d 0x00 0x00 0x04 0x00 0x00 0x00 0x00 0x04 0x00 0x00 0x00 0x00 0x08 0x00 0x08 0x00 0x02 0x02 0x00 0x00 0x01 0x01 0x00 0x00 0x0d 0x00 0x88 0x03 0x02

}

bvra_stop(){

SCO_HANDLE_SOURCE_DEVICE=8

/usr/share/dcc/bluealsa-rfcomm "/org/bluealsa/hci0/dev_$1" <<EOF
+BVRA:0
EOF

hcitool -i hci0 cmd 0x01 0x06 $SCO_HANDLE_SOURCE_DEVICE 0x00 0x13
sleep 2

hcitool -i hci0 cmd 0x3F 0x0029 0x04 0x00
hcitool -i hci0 cmd 0x3F 0x73 0x01

sleep 1
}

# ================ [ main ] ================
usage()
{
	echo -e "./hci_cmd.sh [cmd]"
	echo -e "\t  init "
	echo -e "\t  ot "
}

echo -e "($1) ($2) ($3)"
case "$1" in
	start)

    status=`ps -ef |grep -w bluealsa|grep -v grep|wc -l`
    while [ $status -eq 0 ]; do
        /usr/share/dcc/bluealsa -p hfp-ag -p hfp-hf &
        sleep 2
        status=`ps -ef |grep -w bluealsa|grep -v grep|wc -l`
    done

    SOURCE_DEVICE_MAC=`echo "$2" | tr '[a-z]' '[A-Z]'`
    SOURCE_DEVICE_MAC_RFCOMM=`echo "$SOURCE_DEVICE_MAC" | tr : _`

    output=$(hcitool con)
    echo "output:"
    echo $output

    ACL_HANDLE_SOURCE_DEVICE=$(echo "$output" | grep "ACL" | grep "$(echo "$SOURCE_DEVICE_MAC")" | awk '{print $5}')

	bvra_start "$SOURCE_DEVICE_MAC_RFCOMM"
	;;
	stop)
    SOURCE_DEVICE_MAC=`echo "$2" | tr '[a-z]' '[A-Z]'`
    SOURCE_DEVICE_MAC_RFCOMM=`echo "$SOURCE_DEVICE_MAC" | tr : _`
	bvra_stop "$SOURCE_DEVICE_MAC_RFCOMM"
	;;
	help) usage; exit 0;;
	*) echo -n "Error, Unsupported cmd: [$1]"; exit -1;;
esac