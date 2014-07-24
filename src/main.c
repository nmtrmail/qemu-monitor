#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "packet.h"
#include "fetcher.h"
#include "ui.h"

/* IPC socket address */
#define ADDRESS "fetcher"

/* Global variables */
FILE *fp;
int s;

/* IPC socket connection */
static void conn(void)
{
        int len;
        struct sockaddr_un saun;
        if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
                printf("client: socket");
        }

        saun.sun_family = AF_UNIX;
        strcpy(saun.sun_path, ADDRESS);

        len = sizeof(saun.sun_family) + strlen(saun.sun_path);

        if (connect(s, (struct sockaddr *)&saun, len) < 0) {
                printf("client: connect");
        }

        fp = fdopen(s, "r");
}

static void *conn_thread(void *arg)
{
	FetcherPacket packet = {0};

	/* Connect to QEMU */
        conn();

	/* Handle each packet received from QEMU */
        while(fread(&packet, sizeof(FetcherPacket), 1, fp)) {
		display_update(packet);
        }

	pthread_exit(0);
}

static void *prompt_thread(void *arg)
{
	console_prompt();

	pthread_exit(0);
}

int main(int argc, char *argv[])
{
	pthread_t c_thread, p_thread;

	/* UI initialize */
	ui_init();

	/* Create two thread:
	 * 1. connection with QMEU, and receive packet
	 * 2. Interact with user.
	 */
	pthread_create(&c_thread, NULL, conn_thread, NULL);
	pthread_create(&p_thread, NULL, prompt_thread, NULL);

	while(1);

	/* UI destroy */
	ui_destroy();

	return 0;
}

