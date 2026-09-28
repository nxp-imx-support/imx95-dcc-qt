/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef BTRFCOMM_H
#define BTRFCOMM_H

#include <pthread.h>
#include <string>

class Rfcomm
{
public:
    Rfcomm(std::string addr);
    ~Rfcomm();

    // phone call
    bool rfcomm_run();
    void rfcomm_stop();
    void send_cmd(const char* cmd);

private:
    static void* rfcomm_thread_func(void *arg);

    pthread_t thread_id;
    std::string address;
    int rfcomm_id;
    bool rfcomm_ready;
};

#endif // BLUETOOTHCTL_H
