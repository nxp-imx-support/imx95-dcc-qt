/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef MESSAGE_H
#define MESSAGE_H

#define BT_NODE_ADDR_MAX_LEN 24
#define BT_NODE_NAME_MAX_LEN 40

#define BT_PLAYER_TITLE_MAX_LEN 128
#define BT_PLAYER_ARTIST_MAX_LEN 128

#define MAX_PHONE_NUMBER_COUNT 32

#define DISABLE_BLE true

#define SIG_PATH "/usr/share/dcc/bluealsa"
#define SIG_A2B  'T'
#define SIG_B2A  'R'

enum device_type {
    UNKNOWN   = 0,
    HEADSET,
    PHONE,
    ERROR,
};

enum cmd_type {
    UNDEGINED   = 0,
    CONTROL,
    PLAYER,
};

enum control_cmd {
    INVALID   = 0,
    AGENT,
    DEFAULT_PLAYER,
    SCAN_ON,
    SCAN_OFF,
    ADD,
    CONNECT,
    DISCONNECT,
    REMOVE,
};

enum player_cmd {
    EMPTY = 0,
    MEDIA_PLAY,
    MEDIA_PAUSE,
    MEDIA_NEXT_TRACK,
    MEDIA_PREV_TRACK,
    PLAYER_STATUS,
    PLAYER_PROPERTY,
};

enum call_cmd {
    NOTHING   = 0,
    ACCEPT,
    REJECT,
    TERMINATE,
    PHONE_CALL_INTO,
    PHONE_CALL_EXIT,
    INTERCON_INTO,
    INTERCON_EXIT,
    SET_SRC,
    SET_SINK,
    BT_VOICE_START,
    BT_VOICE_STOP,
    VR_START,
    VR_STOP,
};


typedef struct {
    char address[BT_NODE_ADDR_MAX_LEN];
    char name[BT_NODE_NAME_MAX_LEN];
    bool paired;
    bool connected;
    enum device_type type;
}dev_info;

typedef struct {
    bool player_status;
    char title[BT_PLAYER_TITLE_MAX_LEN];
    char artist[BT_PLAYER_ARTIST_MAX_LEN];
    uint32_t duration;
    uint32_t position;
} player_info;

typedef struct {
    pthread_mutex_t msg_mutex;
    pthread_cond_t  msg_cond;
    bool handle_mask;
    char phone_number[MAX_PHONE_NUMBER_COUNT];
    bool ring;
    int  handle_idx;
    enum call_cmd call_type;
} call_info;


// TX buffer
typedef struct {
    int  idx;
    char address[BT_NODE_ADDR_MAX_LEN];
} a2b_cmd;


typedef struct {
    long     mtype;
    a2b_cmd  message;
}message_a2b;

// RX buffer
typedef struct {
    int          idx;
    bool         agent;
    bool         default_player;
    dev_info     dev;
    player_info  player;
}b2a_info;

typedef struct {
    long      mtype;
    b2a_info  message;
}message_b2a;

void sco_message();

#endif // MESSAGE_H