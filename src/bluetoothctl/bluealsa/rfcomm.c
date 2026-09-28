/*
 * BlueALSA - rfcomm.c
 * Copyright (c) 2016-2022 Arkadiusz Bokowy
 *
 * This file is a part of bluez-alsa.
 *
 * This project is licensed under the terms of the MIT license.
 *
 */


#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <pthread.h>
#include <semaphore.h>

#include <bluetooth/bluetooth.h>
#include <dbus/dbus.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "shared/dbus-client.h"
#include "shared/dbus-client-rfcomm.h"
#include "shared/log.h"

#include "rfcomm.h"
#include "message.h"

#define gettid() syscall(SYS_gettid)

static __thread int rfcomm_fd = -1;
static __thread bool main_loop_on = true;
static __thread bool input_is_tty = true;
static __thread bool output_is_tty = true;
static __thread bool properties = false;

bool accept_call = false;
bool raise_ring = false;
int phone_sco_handle = 0;
int hf_sco_handle = 0;
int previous_ring_handle = 0;
int previous_accept_handle = 0;

pthread_mutex_t mutex;

call_info call_msg = {
	.msg_mutex = PTHREAD_MUTEX_INITIALIZER,
    .msg_cond =  PTHREAD_COND_INITIALIZER,
	.handle_mask = false,
	.phone_number = "",
	.ring = false,
	.handle_idx = 0,
	.call_type = NOTHING,
};

static int path2hci(const char *path) {
	int id;
	if ((path = strstr(path, "/hci")) == NULL ||
			sscanf(path, "/hci%d/", &id) != 1)
		return -1;
	return id;
}

static int path2ba(const char *path, bdaddr_t *ba) {

	unsigned int x[6];
	if ((path = strstr(path, "/dev_")) == NULL ||
			sscanf(&path[5], "%x_%x_%x_%x_%x_%x",
				&x[5], &x[4], &x[3], &x[2], &x[1], &x[0]) != 6)
		return -1;

	size_t i;
	for (i = 0; i < 6; i++)
		ba->b[i] = x[i];

	return 0;
}

static char *strtrim(char *str) {
	while (isspace(*str))
		str++;
	if (*str == '\0')
		return str;
	char *end = &str[strlen(str) - 1];
	while (end > str && isspace(*end))
		end--;
	end[1] = '\0';
	return str;
}

static bool print_properties(struct ba_dbus_ctx *dbus_ctx, const char* path, DBusError *err) {

	struct ba_rfcomm_props props = { 0 };
	if (!ba_dbus_rfcomm_props_get(dbus_ctx, path, &props, err)) {
		ba_dbus_rfcomm_props_free(&props);
		return false;
	}

	printf("Transport: %s\n", props.transport);
	printf("Features:");
	for (size_t i = 0; i < props.features_len; i++)
		printf(" %s", props.features[i]);
	printf("\n");
	printf("Battery: %d\n", props.battery);

	ba_dbus_rfcomm_props_free(&props);

	return true;
}

static char *build_rfcomm_command(char *buffer, size_t size, const char *cmd) {
	bool at = strncmp(cmd, "AT", 2) == 0;
	snprintf(buffer, size, "%s%s%s", at ? "" : "\r\n", cmd, at ? "\r" : "\r\n");
	return buffer;
}

static void rl_callback_handler(char *line) {

	if (line == NULL) {
		rl_callback_handler_remove();
		main_loop_on = false;
		return;
	}

	line = strtrim(line);
	if (strlen(line) == 0)
		return;

	char cmd[512];
	build_rfcomm_command(cmd, sizeof(cmd), line);

	if (write(rfcomm_fd, cmd, strlen(cmd)) == -1)
		warn("Couldn't send RFCOMM command: %s", strerror(errno));

	if (input_is_tty)
		add_history(line);
}

void rfcomm_send(int fd, char *line) {

	if(strcmp(line, "ATA") == 0)
		accept_call = true;

	line = strtrim(line);
	if (strlen(line) == 0)
		return;

	char cmd[512];
	build_rfcomm_command(cmd, sizeof(cmd), line);

	if (write(fd, cmd, strlen(cmd)) == -1)
		warn("RFCOMM Couldn't send command: %s", strerror(errno));

	if (input_is_tty)
		add_history(line);
}

int rfcomm_loop(const char* dev_path, bool* ready, int* fd)
{

	int handle_state = 0;
	int hci_dev_id;
	bdaddr_t addr;
	char dbus_ba_service[32] = BLUEALSA_SERVICE;

	if ((hci_dev_id = path2hci(dev_path)) == -1 ||
			path2ba(dev_path, &addr) == -1) {
		error("%s Invalid BT device path: %s", __FUNCTION__, dev_path);
		return EXIT_FAILURE;
	}
	DBusError err = DBUS_ERROR_INIT;
	struct ba_dbus_ctx dbus_ctx;

	if (!ba_dbus_connection_ctx_init(&dbus_ctx, dbus_ba_service, &err)) {
		error("%s Couldn't initialize D-Bus context: %s", __FUNCTION__, err.message);
		return EXIT_FAILURE;
	}
	char rfcomm_path[128];
	sprintf(rfcomm_path, "/org/bluealsa/hci%d/dev_%.2X_%.2X_%.2X_%.2X_%.2X_%.2X/rfcomm",
			hci_dev_id, addr.b[5], addr.b[4], addr.b[3], addr.b[2], addr.b[1], addr.b[0]);

	if (properties) {
		if (print_properties(&dbus_ctx, rfcomm_path, &err))
			return EXIT_SUCCESS;
		error("%s D-Bus error: %s", __FUNCTION__, err.message);
		return EXIT_FAILURE;
	}

	if (!ba_dbus_rfcomm_open(&dbus_ctx, rfcomm_path, &rfcomm_fd, &err)) {
		error("%s Couldn't open RFCOMM: %s", __FUNCTION__, err.message);
		return EXIT_FAILURE;
	}

	*fd = rfcomm_fd;

	input_is_tty = isatty(fileno(stdin));
	output_is_tty = isatty(fileno(stdout));
	const char *prefix = "";
	char prompt[32] = "";

	if (input_is_tty) {
		sprintf(prompt, "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X> ",
			addr.b[5], addr.b[4], addr.b[3], addr.b[2], addr.b[1], addr.b[0]);
		rl_bind_key('\t', rl_insert);
		prefix = "> ";
	}
	else
		/* libreadline echos its input to stdout which is a problem when
		 * stdin is not the terminal. The function rl_tty_set_echoing() is
		 * broken when used with the callback API, so as a workaround we
		 * redirect the (useless) output to /dev/null */
		rl_outstream = fopen("/dev/null", "w");

	if (!output_is_tty)
		/* Force line buffered output to be sure each event will be flushed
		 * immediately. */
		setvbuf(stdout, NULL, _IOLBF, 0);

	// rl_callback_handler_install(prompt, rl_callback_handler);

	struct pollfd pfds[] = {
		{ fileno(stdin), POLLIN, 0 },
		{ rfcomm_fd, POLLIN, 0 },
	};

	*ready = true;
	printf(" %s was successfully created \n", __FILE__);

	char str1[4];
	char str2[4];
	int str1_len, str2_len;

	while (main_loop_on) {

		poll(pfds, 2, -1);

		if (pfds[0].revents & POLLIN) {
			// rl_callback_read_char();
			if (!main_loop_on)
			break;
		}
		else if (pfds[0].revents & POLLHUP)
			break;

		if (pfds[1].revents & POLLIN) {

			char buffer[256];
			ssize_t ret;

			if ((ret = read(rfcomm_fd, buffer, sizeof(buffer) - 1)) <= 0) {
				if (output_is_tty) {
					rl_replace_line("rfcomm disconnected", 0);
					rl_message("");
					rl_redisplay();
				}
				break;
			}

			buffer[ret] = '\0';
			char *old_text = NULL;
			int old_point = 0;

			printf("%s %d pid 0x%X---------- %s, data:%s\n", __FILE__, __LINE__,(unsigned int)pthread_self(),prefix, strtrim(buffer));

			char *ptr_l;
			char *ptr_r;
			char *string;
			int   handle;
			string = strtrim(buffer);

			// get phone number
			if(strstr(string, "CLIP:") != NULL) {
				// +CLIP: "181****7025",129
				ptr_l = strchr(string, ' ') + 2;
				ptr_r = strchr(string, ',') - 1;
				str1_len = ptr_r - ptr_l;
				strncpy(call_msg.phone_number, ptr_l, str1_len);
				call_msg.phone_number[str1_len] = '\0';
				call_msg.call_type = PHONE_CALL_INTO;
				sco_message();
			}

			if(strstr(string, "CIEV:") != NULL) {
				// +CIEV: 2,1
				ptr_l = strchr(string, ' ') + 1;
				ptr_r = strchr(string, ',');
				str1_len = ptr_r - ptr_l;
				strncpy(str1, ptr_l, str1_len);
				str1[str1_len] = '\0';
				handle = atoi(str1);

				str2_len = strlen(string) - (ptr_r - string) - 1;
				strncpy(str2, ptr_r + 1, str2_len);
				str2[str2_len] = '\0';
				handle_state = atoi(str2);

				printf("%s %d----------------handle_idx  %d , handle_state %d, raise_ring %d \n", __FILE__, __LINE__,call_msg.handle_idx, handle_state, raise_ring);

				if(phone_sco_handle == 0 && raise_ring == false && handle_state == 1)
					previous_ring_handle = handle;

				if(phone_sco_handle == handle && handle_state == 0 && hf_sco_handle == 0 && accept_call == true)
					hf_sco_handle = previous_accept_handle;

				if(accept_call == true && handle == hf_sco_handle && handle_state == 0) {
					call_msg.ring = false;
					call_msg.call_type = TERMINATE;
					sco_message();

					phone_sco_handle = 0;
					hf_sco_handle = 0;
					raise_ring = false;
					accept_call= false;
				}

				if(accept_call == false && handle == phone_sco_handle && handle_state == 0) {
					call_msg.ring = false;
					call_msg.call_type = REJECT;
					sco_message();

					phone_sco_handle = 0;
					// hf_sco_handle = 0;
					raise_ring = false;
					accept_call= false;
				}

				previous_accept_handle = handle;

			} else {
				if(strcmp(string, ":RING") == 0) {
					printf("%s %d phone number %s:%s\n", __FILE__, __LINE__, string, call_msg.phone_number);
					phone_sco_handle = previous_ring_handle;
					call_msg.ring = true;
					raise_ring = true;
				}
					
			}

			if (output_is_tty) {
				old_point = rl_point;
				old_text = rl_copy_text(0, rl_end);
				rl_save_prompt();
				rl_replace_line("", 0);
				rl_redisplay();
			}

			if (output_is_tty) {
				rl_restore_prompt();
				rl_replace_line(old_text, 0);
				rl_point = old_point;
				rl_redisplay();
				free(old_text);
			}

		}

		sleep(1);
	}

	// rl_callback_handler_remove();
	fprintf(stdout, "\n");
	printf("%s EXIT_SUCCESS \n", __FUNCTION__);

	return EXIT_SUCCESS;
}

void sco_message()
{
	pthread_mutex_lock(&call_msg.msg_mutex);
	call_msg.handle_mask = true;

	pthread_cond_signal(&call_msg.msg_cond);
	pthread_mutex_unlock(&call_msg.msg_mutex);
}
