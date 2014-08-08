#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <ncurses.h>
#include <errno.h>

#include "types.h"
#include "ui.h"
#include "console.h"

/* IPC socket address */
#define ADDRESS "fetcher"

/* Global variables */
FILE *fp;

/* Used for unused parameters to silence gcc warnings */
#define UNUSED __attribute__((__unused__))

/* IPC socket connection for TUI */
static void tui_conn(void)
{
        int ns, s, len;
	socklen_t fromlen UNUSED;

	fromlen = 0;

        struct sockaddr_un saun, fsaun;
        if((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
                console_puts("Server: Socket");
        }

        saun.sun_family = AF_UNIX;
        strcpy(saun.sun_path, ADDRESS);

        unlink(ADDRESS);
        len = sizeof(saun.sun_family) + strlen(saun.sun_path);

	// XXX bind() need a const struct sockaddr pointer as argument...
	const struct sockaddr *saun_const = (struct sockaddr *)&saun;
        if(bind(s, saun_const, len) < 0) {
                console_puts("Server: Bind");
        }

        if(listen(s, 5) < 0) {
                console_puts("Server: Listen");
        }

        if((ns = accept(s, (struct sockaddr *)&fsaun, &fromlen)) < 0) {
                console_puts("Server: Accept");
        }

	fp = fdopen(ns, "r");
}

static void *tui_conn_thread(void *arg)
{
	FetcherPacket packet = {0};

	while(1) {
		/* Connect to QEMU */
		tui_conn();
		display_status(1);

		/* Handle each packet received from QEMU */
		while(fread(&packet, sizeof(FetcherPacket), 1, fp)) {
			display_update(packet);
		}
		display_status(1);
	}

	return 0;
}

static void *tui_prompt_thread(void *arg)
{
	console_prompt();

	pthread_exit(0);
}

/* IPC socket connection */
static void conn(void)
{
        int ns, s, len;
	socklen_t fromlen UNUSED;

	fromlen = 0;

        struct sockaddr_un saun, fsaun;
        if((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
                printf("Server: Socket\n");
        }

        saun.sun_family = AF_UNIX;
        strcpy(saun.sun_path, ADDRESS);

        unlink(ADDRESS);
        len = sizeof(saun.sun_family) + strlen(saun.sun_path);

	// XXX bind() need a const struct sockaddr pointer as argument...
	const struct sockaddr *saun_const = (struct sockaddr *)&saun;
        if(bind(s, saun_const, len) < 0) {
                printf("Server: Bind\n");
        }

        if(listen(s, 5) < 0) {
                printf("Server: Listen\n");
        }

        if((ns = accept(s, (struct sockaddr *)&fsaun, &fromlen)) < 0) {
                printf("Server: Accept\n");
        }

	fp = fdopen(ns, "r");
}

static void *conn_thread(void *arg)
{
	FetcherPacket packet = {0};

	while(1) {
		/* Connect to QEMU */
		printf("Listen for QEMU... ");
		fflush(stdout);
		conn();
		printf("connect success!\n");
		printf("-> ");
		fflush(stdout);

		/* Handle each packet received from QEMU */
		while(fread(&packet, sizeof(FetcherPacket), 1, fp)) {
			console_handle(packet);
		}
		printf("\nConnection closed!\n");
	}

	return 0;
}

static void *prompt_thread(void *arg)
{
	_console_prompt();

	pthread_exit(0);
}

int main(int argc, char *argv[])
{
	pthread_t c_thread, p_thread;

	if(argc == 2 && !strcmp("-tui", argv[1])) {
		/* UI initialize */
		ui_init();

		/* Create two thread:
		 * 1. connection with QMEU, and receive packet
		 * 2. Interact with user.
		 */
		pthread_create(&c_thread, NULL, tui_conn_thread, NULL);
		pthread_create(&p_thread, NULL, tui_prompt_thread, NULL);

		/* Block until lost connection */
		pthread_join(p_thread, NULL);

		/* UI destroy */
		ui_destroy();
	}
	else {
		pthread_create(&c_thread, NULL, conn_thread, NULL);
		pthread_create(&p_thread, NULL, prompt_thread, NULL);
		/* Block until lost connection */
		pthread_join(p_thread, NULL);
	}

	return 0;
}

