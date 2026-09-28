/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <cstdio>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <cstring>
#include <unistd.h>

#include "bluetoothctl.h"

#define MAX_CONNECTED_MUM 3

BluetoothCtl::BluetoothCtl()
{
	send_buffer.mtype = (long)UNDEGINED;
	send_buffer.message.idx = 0;
	strcpy(send_buffer.message.address, "00:00:00:00:00:00");

	key_tx = -1;
	msqid_tx = -1;

	agent_registered = false;
	player_registered = false;

}

BluetoothCtl::~BluetoothCtl()
{
	this->exit();
}

void *btctl_shell_func(void *arg)
{

	if(system("/usr/share/dcc/bluetoothctl-dcc ") < 0)
		printf("Error BluetoothCtl start failed !\n");

	return NULL;
}

void *btctl_agent_func(void *arg)
{
	BluetoothCtl* controller = (BluetoothCtl *) arg;

	while(controller->agent_registered == false) {
		printf("BluetoothCtl agent not ready !\n");
		sleep(1);
		controller->agent_on();
	}

	return NULL;
}

void BluetoothCtl::run()
{

    // Generate cmd TX queue
    if ((key_tx = ftok(SIG_PATH, SIG_A2B)) == -1) {
        perror("BluetoothCtl tx ftok");
    }

	if ((msqid_tx = msgget(key_tx, 0666 | IPC_CREAT)) == -1) {
		perror("BluetoothCtl tx msgget");
	}


	printf("---> 1 key_tx = %d, msqid_tx = %d! \n", key_tx, msqid_tx);

	pthread_create(&shell_thread, NULL, btctl_shell_func, NULL);
	pthread_create(&agent_thread, NULL, btctl_agent_func, this);
	pthread_setname_np(shell_thread, "bluetoothctl");

	agent_on();
}

void BluetoothCtl::main_loop()
{

}

void BluetoothCtl::exit()
{
	printf("BluetoothCtl exit successful !\n");
}

void BluetoothCtl::agent_on()
{
	send_buffer.mtype = (long)CONTROL;
	send_buffer.message.idx = AGENT;

    if (msgsnd(msqid_tx, &send_buffer, sizeof(send_buffer.message), 0) == -1) {
        perror("send scan on cmd failed");
    }

	printf("BluetoothCtl download agent on cmd \n");
}

void BluetoothCtl::show()
{
	//TODO
}

void BluetoothCtl::scan_on()
{
	if( agent_registered ==  false ) {
		perror("BluetoothCtl: controller agent not ready");
	}

	send_buffer.mtype = (long)CONTROL;
	send_buffer.message.idx = SCAN_ON;

    if (msgsnd(msqid_tx, &send_buffer, sizeof(send_buffer.message), 0) == -1) {
        perror("send scan on cmd failed");
    }

}

void BluetoothCtl::scan_off()
{
	if( agent_registered ==  false ) {
		perror("BluetoothCtl: controller agent not ready");
		return;
	}

	send_buffer.mtype = (long)CONTROL;
	send_buffer.message.idx = SCAN_OFF;

    if (msgsnd(msqid_tx, &send_buffer, sizeof(send_buffer.message), 0) == -1) {
        perror("send scan off cmd failed");
    }
}

void BluetoothCtl::connect( const char* addr)
{
	if( agent_registered ==  false ) {
		perror("BluetoothCtl: controller agent not ready");
		return;
	}

	send_buffer.mtype = (long)CONTROL;
	send_buffer.message.idx = CONNECT;
	strcpy(send_buffer.message.address, addr);

    if (msgsnd(msqid_tx, &send_buffer, sizeof(send_buffer.message), 0) == -1) {
        perror("send connect cmd failed");
    }
}

void BluetoothCtl::disconnect( const char* addr)
{
	if( agent_registered ==  false ) {
		perror("BluetoothCtl: controller agent not ready");
		return;
	}

	send_buffer.mtype = (long)CONTROL;
	send_buffer.message.idx = DISCONNECT;
	strcpy(send_buffer.message.address, addr);

    if (msgsnd(msqid_tx, &send_buffer, sizeof(send_buffer.message), 0) == -1) {
        perror("send disconnect cmd failed");
    }
}

void BluetoothCtl::remove( const char* addr)
{
	if( agent_registered ==  false ) {
		perror("BluetoothCtl: controller agent not ready");
		return;
	}

	send_buffer.mtype = (long)CONTROL;
	send_buffer.message.idx = REMOVE;
	strcpy(send_buffer.message.address, addr);

    if (msgsnd(msqid_tx, &send_buffer, sizeof(send_buffer.message), 0) == -1) {
        perror("send remove cmd failed");
    }
}

void BluetoothCtl::play()
{
	send_buffer.mtype = (long)PLAYER;
	send_buffer.message.idx = MEDIA_PLAY;

    if (msgsnd(msqid_tx, &send_buffer, sizeof(send_buffer.message), 0) == -1) {
        perror("send play cmd failed");
    }
}

void BluetoothCtl::pause()
{
	send_buffer.mtype = (long)PLAYER;
	send_buffer.message.idx = MEDIA_PAUSE;

    if (msgsnd(msqid_tx, &send_buffer, sizeof(send_buffer.message), 0) == -1) {
        perror("send pause cmd failed");
    }
}

void BluetoothCtl::next()
{
	send_buffer.mtype = (long)PLAYER;
	send_buffer.message.idx = MEDIA_NEXT_TRACK;

    if (msgsnd(msqid_tx, &send_buffer, sizeof(send_buffer.message), 0) == -1) {
        perror("send next cmd failed");
    }
}

void BluetoothCtl::previous()
{
	send_buffer.mtype = (long)PLAYER;
	send_buffer.message.idx = MEDIA_PREV_TRACK;

    if (msgsnd(msqid_tx, &send_buffer, sizeof(send_buffer.message), 0) == -1) {
        perror("send previous cmd failed");
    }
}

void BluetoothCtl::get_player_info()
{
	send_buffer.mtype = (long)PLAYER;
	send_buffer.message.idx = PLAYER_PROPERTY;

    if (msgsnd(msqid_tx, &send_buffer, sizeof(send_buffer.message), 0) == -1) {
        perror("send get_player_info cmd failed");
    }
}
