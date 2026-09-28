#!/bin/sh -x
# Copyright 2024 NXP
# SPDX-License-Identifier: BSD-3-Clause

rfcomm_call() {
/usr/share/dcc/bluealsa-rfcomm "/org/bluealsa/hci0/dev_$1" <<EOF
+CIEV:1,1
+CIEV:5,5
RING;123456
RING;123456
RING;123456
+CIEV:2,1
EOF
}

rfcomm_end(){
/usr/share/dcc/bluealsa-rfcomm "/org/bluealsa/hci0/dev_$1" <<EOF
+CIEV:2,0
+CIEV:5,0
+CIEV:1,0
EOF
}

accept_call() {

    SOURCE_DEVICE_MAC=`echo "$1" | tr '[a-z]' '[A-Z]'`
    SINK_DEVICE_MAC=`echo "$2" | tr '[a-z]' '[A-Z]'`

    /usr/share/dcc/btnxp_accept_call.sh $SOURCE_DEVICE_MAC $SINK_DEVICE_MAC
    /usr/share/dcc/btnxp_call_bridging.sh $SOURCE_DEVICE_MAC $SINK_DEVICE_MAC "$3"
}

release_call() {
    hcitool -i hci0 cmd 0x3F 0x0070 0x01
    hcitool -i hci0 cmd 0x3F 0x001D 0x01
    hcitool -i hci0 cmd 0x3F 0x0007 0x00
    hcitool -i hci0 cmd 0x3F 0x0028 0x03 0x00 0x04
    hcitool -i hci0 cmd 0x3F 0x0029 0x04 0x00
    hcitool -i hci0 cmd 0x3F 0x73 0x01

    if [ $# -gt 0 ] ; then
        /usr/share/dcc/btnxp_voice_recognition.sh stop "$1"
    fi
}

end_call() {

	SINK_DEVICE=`echo "$2" | tr '[a-z]' '[A-Z]'`
	SINK_DEVICE_RFCOMM=`echo "$SINK_DEVICE" | tr : _`

/usr/share/dcc/bluealsa-rfcomm "/org/bluealsa/hci0/dev_$SINK_DEVICE_RFCOMM" <<EOF
+CIEV:2,0
+CIEV:5,0
+CIEV:1,0
EOF

}

inter_start() {

	HFP_DEV_1=`echo "$1" | tr '[a-z]' '[A-Z]'`
	HFP_DEV_1_RFCOMM=`echo "$HFP_DEV_1" | tr : _`

	HFP_DEV_2=`echo "$2" | tr '[a-z]' '[A-Z]'`
	HFP_DEV_2_RFCOMM=`echo "$HFP_DEV_2" | tr : _`

	output=$(hcitool con)
	echo "output:"
	echo $output

	ACL_HANDLE_HFP_DEV_1=$(echo "$output" | grep "ACL" | grep "$(echo "$HFP_DEV_1")" | awk '{print $5}')
	ACL_HANDLE_HFP_DEV_2=$(echo "$output" | grep "ACL" | grep "$(echo "$HFP_DEV_2")" | awk '{print $5}')

	echo "ACL_HANDLE_HFP_DEV_1 is :"
	echo $ACL_HANDLE_HFP_DEV_1

	echo "ACL_HANDLE_HFP_DEV_2 is :"
	echo $ACL_HANDLE_HFP_DEV_2

	echo "Setting Up Device $HFP_DEV_1"
	hcitool -i hci0 cmd 0x3F 0x0070 0x01
	hcitool -i hci0 cmd 0x3F 0x001D 0x01
	hcitool -i hci0 cmd 0x3F 0x0007 0x00
	hcitool -i hci0 cmd 0x3F 0x0028 0x03 0x00 0x04
	hcitool -i hci0 cmd 0x3F 0x0029 0x04 0x00
	hcitool -i hci0 cmd 0x3F 0x73 0x01
	sleep 2

	rfcomm_call "$HFP_DEV_1_RFCOMM"
    sleep 1

    hcitool -i hci0 cmd 0x01 0x28 $ACL_HANDLE_HFP_DEV_1 0x00 0x40 0x1f 0x00 0x00 0x40 0x1f 0x00 0x00 0x0d 0x00 0x03 0x00 0x02 0x3f 0x00
    sleep 2

    echo "Setting Up Device $HFP_DEV_2"
    hcitool -i hci0 cmd 0x3f 0x0029 0x45 0x00
    hcitool -i hci0 cmd 0x3F 0x73 0x01

	rfcomm_call "$HFP_DEV_2_RFCOMM"
	hcitool -i hci0 cmd 0x01 0x28 $ACL_HANDLE_HFP_DEV_2 0x00 0x40 0x1f 0x00 0x00 0x40 0x1f 0x00 0x00 0x0d 0x00 0x03 0x00 0x02 0x3f 0x00
    sleep 2

	SCO_HANDLE_HFP_DEV_1=8
	SCO_HANDLE_HFP_DEV_2=9

	echo "start intercom"
	echo "Starting Intercom!"
	hcitool -i hci0 cmd 0x3F 0x006F 0x00 0x01 $SCO_HANDLE_HFP_DEV_1 0x00 $SCO_HANDLE_HFP_DEV_2 0x00
	# /usr/share/dcc/btnxp_intercom.sh $SOURCE_DEVICE_MAC $SINK_DEVICE_MAC start
}

inter_end() {

	HFP_DEV_1=`echo "$1" | tr '[a-z]' '[A-Z]'`
	HFP_DEV_1_RFCOMM=`echo "$HFP_DEV_1" | tr : _`

	HFP_DEV_2=`echo "$2" | tr '[a-z]' '[A-Z]'`
	HFP_DEV_2_RFCOMM=`echo "$HFP_DEV_2" | tr : _`

	SCO_HANDLE_HFP_DEV_1=8
	SCO_HANDLE_HFP_DEV_2=9

    echo "End Intercom!"
	rfcomm_end "$HFP_DEV_1_RFCOMM"
	sleep 1
	hcitool -i hci0 cmd 0x01 0x06 $SCO_HANDLE_HFP_DEV_1 0x00 0x13
	sleep 1
	rfcomm_end "$HFP_DEV_2_RFCOMM"
	sleep 1
	hcitool -i hci0 cmd 0x01 0x06 $SCO_HANDLE_HFP_DEV_2 0x00 0x13
}

free_phonecall_sco()
{
	echo "Disconnecting SCO Connection with BT Headset"
	hcitool -i hci0 cmd 0x01 0x06 0x09 0x00 0x13
}

set_sink_node()
{
	if [ $# -gt 1 ] ; then
		device_name=$2
	else
		device_name=$1
	fi

    status=`ps -ef |grep -w pipewire|grep -v grep|wc -l`
    while [ $status -eq 0 ]; do
		sleep 1
        systemctl --user start pipewire wireplumber
        sleep 1
        status=`ps -ef |grep -w pipewire|grep -v grep|wc -l`
    done

	sleep 3

	id=`wpctl status | awk '/Sinks/ {flag=1} /'${device_name}'/ && flag {print int($3); flag=0}' `
	if [ "$id" -eq 0 ] ; then
		id=`wpctl status | awk '/Sinks/ {flag=1} /'${device_name}'/ && flag {print int($2); flag=0}' `
	fi

	wpctl set-default $id
}

set_src_node()
{
	id=`wpctl status | awk '/Sources/ {flag=1} /Built-in Audio Mono/ && flag {print int($2); flag=0}' `
	if [ $id -gt 0 ]
	then
		wpctl set-default $id
	else
		id=`wpctl status | awk '/Sources/ {flag=1} /Built-in Audio Mono/ && flag {print int($3); flag=0}' `
		wpctl set-default $id
	fi
}

vit_start()
{
	/usr/share/dcc/vit_dcc -i IMX9XA55 -d hw:0,0 -l ENGLISH &
}

vit_stop()
{
	killall vit_dcc
}

# ================ [ main ] ================
usage()
{
	echo -e "./bt_bridge.sh [cmd]"
    echo -e "\t  init "
	echo -e "\t  ot "
}

echo -e "($1) ($2) ($3) ($4)"
case "$1" in
	accept)
	accept_call $2 $3 $4
	;;
	reject)
	release_call
	;;
	end)
	end_call $2 $3 $4
	free_phonecall_sco
	sleep 2
	release_call $3
	;;
	intercom_start)
	inter_start $2 $3
	;;
	intercom_end)
	inter_end $2 $3
	;;
	wpctl_src_config)
	set_src_node $2 $3
	;;
	wpctl_sink_config)
	set_sink_node $2 $3
	;;
	help) usage; exit 0;;
	*) echo -n "Error, Unsupported cmd: [$3]"; exit -1;;
esac