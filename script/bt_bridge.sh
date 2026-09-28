#!/bin/bash -x
# Copyright 2024 NXP
# SPDX-License-Identifier: BSD-3-Clause

cleanup() {
   echo "Killing bluez-alsa process."
   killall bluealsa
}

bt_reset() {
    echo "Resetting HCI Interface"
    hciconfig hci0 reset
    sleep 1
    hcitool -i hci0 cmd 3F 23 02 00 00
    hciconfig hci0 piscan
    hciconfig hci0 noencrypt
    hciconfig -a
}

module_init() {
    killall ofonod
    sleep 1

    echo "Setting Up Platform For Bridging Use-Case"
    /usr/share/dcc/bluealsa -p hfp-ag -p hfp-hf &
    sleep 2

    echo "Starting Pipewire & Wireplumber"
    systemctl --user start pipewire wireplumber
    sleep 2

    status=`ps -ef |grep -w pipewire|grep -v grep|wc -l`
    while [ $status -eq 0 ]; do
        systemctl --user start pipewire wireplumber
        sleep 2
        status=`ps -ef |grep -w pipewire|grep -v grep|wc -l`
    done

    echo "Configuring HCI0 as Master Role"
    hciconfig hci0 lm master,accept
    sleep 1

    hciconfig hci0 lp sniff
    sleep 1

    hcitool -i hci0 cmd 0x3F 0x0070 0x01
    hcitool -i hci0 cmd 0x3F 0x001D 0x01
    hcitool -i hci0 cmd 0x3F 0x0007 0x00
    hcitool -i hci0 cmd 0x3F 0x0028 0x03 0x00 0x04
    sleep 1

    hcitool -i hci0 cmd 0x3F 0x0029 0x04 0x00
    sleep 1

    hcitool -i hci0 cmd 0x3F 0x73 0x01
    sleep 1

    echo "bluealsa init finish"
}

bluealsa_check()
{
    ps -ef | grep bluealsa | grep -v grep
    if [ $? -ne 0 ]; then
        /usr/share/dcc/bluealsa -p hfp-ag -p hfp-hf &
        sleep 2
        echo "bluealsa started"
    else
        echo "runing"
    fi
}

bluealsa_stop()
{
    killall bluealsa
}

# ================ [ main ] ================
usage()
{
    echo -e "./bt_bridge.sh [cmd]"
    echo -e "\t  init "
    echo -e "\t  ot "
}

myself=$0
while [ $# -gt 0 ]
do
    echo -e "[$1]"
    case "$1" in
        init)
            module_init
            ;;
        reinit)
            cleanup
            module_init
            ;;
        clean)
            bt_reset
            module_init
            ;;
        check)
            bluealsa_check
            ;;
        exit)
            bluealsa_stop
            ;;
        help) usage; exit 0;;
        *) echo -n "Error, Unsupported cmd: [$1]"; exit -1;;
    esac
    shift
done