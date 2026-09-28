#!/bin/bash
# Copyright 2024 NXP
# SPDX-License-Identifier: BSD-3-Clause

SOURCE_DEVICE=`echo "$1" | tr '[a-z]' '[A-Z]'`
SOURCE_DEVICE_RFCOMM=`echo "$SOURCE_DEVICE" | tr : _`

SINK_DEVICE=`echo "$2" | tr '[a-z]' '[A-Z]'`
SINK_DEVICE_RFCOMM=`echo "$SINK_DEVICE" | tr : _`

/usr/share/dcc/bluealsa-rfcomm "/org/bluealsa/hci0/dev_$SINK_DEVICE_RFCOMM" <<EOF
+CIEV:1,1
+CIEV:5,5
RING;123456
RING;123456
RING;123456
+CIEV:2,1
EOF

output=$(hcitool con)
echo "output:"
echo $output

echo "SINK_DEVICE"
echo $SINK_DEVICE

ACL_HANDLE_SINK_DEVICE=$(echo "$output" | grep "ACL" | grep "$(echo "$SINK_DEVICE")" | awk '{print $5}')

echo "ACL_HANDLE_SINK_DEVICE is :"
echo $ACL_HANDLE_SINK_DEVICE

echo "Configuring - WBS for PCM Slot - 2"
hcitool -i hci0 cmd 0x3f 0x0029 0x45 0x00
hcitool -i hci0 cmd 0x3F 0x73 0x01

echo "Establishing SCO WBS for ACL Handle: "$ACL_HANDLE_SINK_DEVICE
hcitool -i hci0 cmd 0x01 0x28 $ACL_HANDLE_SINK_DEVICE 0x00 0x40 0x1f 0x00 0x00 0x40 0x1f 0x00 0x00 0x0d 0x00 0x03 0x00 0x02 0x3f 0x00

echo "VSC: Call Bridge"
hcitool -i hci0 cmd 0x3F 0x006F 0x00 0x01 0x08 0x00 0x09 0x00

echo "VSC: Call Bridge Cmd Sent"