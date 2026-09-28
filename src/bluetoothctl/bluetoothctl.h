/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef BLUETOOTHCTL_H
#define BLUETOOTHCTL_H

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

#include "message.h"

class BluetoothCtl
{
public:

    BluetoothCtl();
    ~BluetoothCtl();

    void run();
    void main_loop();
    void exit();

    // cmd
    void agent_on();
    void show();
    void scan_on();
    void scan_off();
    // void get_device(const char* addr, bt_node* device);
    void connect(const char* addr);
    void disconnect(const char* addr);
    void remove(const char* addr);
    // void pop_node(node_queue* queue);

    // player
    void play();
    void pause();
    void next();
    void previous();
    void get_player_info();

    bool agent_registered;
    bool player_registered;
    message_a2b send_buffer;

private:
    int msqid_tx;
    key_t key_tx;

    pthread_t shell_thread;
    pthread_t agent_thread;
};

#endif // BLUETOOTHCTL_H
