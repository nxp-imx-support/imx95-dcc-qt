/*
 * BlueALSA - rfcomm.c
 * Copyright (c) 2024 NXP
 *
 * This file is a part of bluez-alsa.
 *
 * This project is licensed under the terms of the MIT license.
 *
 */

#ifndef RFCOMM_H
#define RFCOMM_H

#ifdef __cplusplus
extern "C" {
#endif

int rfcomm_loop(const char* dev_path, bool* ready, int* fd);
void rfcomm_send(int fd, char *line);

void exitreadline();

#ifdef __cplusplus
}
#endif

#endif
