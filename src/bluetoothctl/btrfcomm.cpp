/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <cstdlib>
#include <algorithm>
#include <string.h>
#include <iostream>
#include <fstream>
#include <memory>
#include <array>
#include <unistd.h>

#include "btrfcomm.h"
#include "bluealsa/rfcomm.h"

#define CMD_RESULT_BUF_SIZE 256

Rfcomm::Rfcomm(std::string addr)
{
    address = addr;
    rfcomm_id = -1;
    rfcomm_ready = false;
}

Rfcomm::~Rfcomm()
{
    rfcomm_ready = false;
    rfcomm_stop();
    printf("Bluealsa Rfcomm stoped \n");
}


/*==============================================*/
/* Bluealsa rfcomm */

void* Rfcomm::rfcomm_thread_func(void *arg)
{
    Rfcomm* bluealsa_rfcomm = (Rfcomm*)arg;

    std::replace(bluealsa_rfcomm->address.begin(), bluealsa_rfcomm->address.end(), ':', '_');

    std::string s1 = "/org/bluealsa/hci0/dev_";
    std::string s2 = s1 + bluealsa_rfcomm->address;

    bluealsa_rfcomm->rfcomm_ready = false;
	int ret = rfcomm_loop(s2.c_str(), &bluealsa_rfcomm->rfcomm_ready, &bluealsa_rfcomm->rfcomm_id);
    if(ret > 0) {
        printf("Bluealsa Rfcomm log: the %s error happend \n", bluealsa_rfcomm->address.c_str());
    }

	return NULL;
}

bool Rfcomm::rfcomm_run()
{
    int time = 10;
	pthread_create(&thread_id, NULL, rfcomm_thread_func, (void*)this);
    pthread_setname_np(thread_id, "phone_rfcomm");
    while( !rfcomm_ready && time > 0 ) {
        usleep(100000);
        time--;

    }

    if ( !rfcomm_ready ) {
        printf("Bluealsa Rfcomm log: need to restart bluealasa \n");
        return false;
    }

    return true;
}

void Rfcomm::rfcomm_stop()
{
    // TODO establish eixt thread
}

void Rfcomm::send_cmd(const char *cmd)
{
    char cmd_copy[20];
    strcpy(cmd_copy, cmd);
    rfcomm_send(rfcomm_id, cmd_copy);
}
