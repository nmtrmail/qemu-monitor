#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "packet.h"
#include "fetcher.h"

/* IPC socket address */
#define ADDRESS "fetcher"

/* Print format register information */
#define print_element(tmp)										\
	{												\
		printf("%-16s = ", tmp.name);								\
		switch(tmp.type) {									\
		case ARM_CP_UNIMPL:									\
			printf("UNIMPLEMENTED\n");							\
			break;										\
		case ARM_CP_CONST:									\
			printf("0x%lx\n", tmp.const_value);						\
			break;										\
		case ARM_CP_NORMAL_L:									\
			printf("0x%x\n", *(uint32_t *)((uint8_t *)(&packet) + tmp.fieldoffset));	\
			break;										\
		case ARM_CP_NORMAL_H:									\
			printf("0x%lx\n", *(uint64_t *)((uint8_t *)(&packet) + tmp.fieldoffset));	\
			break;										\
		}											\
	}

/* Global variables */
FILE *fp;
int s;
HookRegisters *hook_head = NULL;

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

/* Print all pre-defined registers */
static void print_all_registers(FetcherPacket packet)
{
	int i, j;

	for(i = 0; i < sizeof(reg_array) / sizeof(struct ARMCPRegArray); i++) {
		printf("======== %s\n", reg_array[i].name);
		for(j = 0; j < reg_array[i].size; j++) {
			ARMCPRegInfo tmp = reg_array[i].array[j];
			print_element(tmp);
		}
	}

	printf("\n");
}

/* Add hook registers which need to be trace */
static void hook_add_register(char *input)
{
	HookRegisters *it = hook_head;
	int i, j;
	int valid = 0;

	/* Search register in registers array */
	for(i = 0; i < sizeof(reg_array) / sizeof(struct ARMCPRegArray); i++) {
		for(j = 0; j < reg_array[i].size; j++) {
			if(!strcmp(reg_array[i].array[j].name, input)) {
				valid = 1;
				break;
			}
		}

		if(valid) {
			break;
		}
	}

	if(!valid) {
		printf("Invalid register name\n");
		return;
	}

	HookRegisters *tmp = (HookRegisters *)malloc(sizeof(HookRegisters));
	tmp->pos = i;
	tmp->index = j;
	tmp->next = NULL;
	printf("Add register %s to hook list\n", input);

	/* First element */
	if(it == NULL) {
		hook_head = tmp;
		return;
	}

	/* Iterate to last element */
	for(; it->next != NULL; it = it->next) {
		if(!strcmp(reg_array[it->pos].array[it->index].name, input)) {
			printf("Register %s has already in hook list\n", input);
			return;
		}
	}
	it->next = tmp;
}

/* Print all hook registers */
static void hook_print(FetcherPacket packet)
{
	HookRegisters *it;

	for(it = hook_head; it != NULL; it = it->next) {
		ARMCPRegInfo tmp = reg_array[it->pos].array[it->index];
		printf("%-16s = ", tmp.name);
		switch(tmp.type) {
		case ARM_CP_UNIMPL:
			printf("UNIMPLEMENTED\n");
			break;
		case ARM_CP_CONST:
			printf("0x%lx\n", tmp.const_value);
			break;
		case ARM_CP_NORMAL_L:
			printf("0x%x\n", *(uint32_t *)((uint8_t *)(&packet) + tmp.fieldoffset));
			break;
		case ARM_CP_NORMAL_H:
			printf("0x%lx\n", *(uint64_t *)((uint8_t *)(&packet) + tmp.fieldoffset));
			break;
		}
	}

	printf("\n");
}

/* Free dynamic allocate memory */
static void hook_destructor(void)
{
	HookRegisters *it, *pre;

	/* Do not have hook register */
	if(hook_head == NULL) {
		return;
	}

	pre = hook_head;
	it = hook_head->next;
	while(pre) {
		free(pre);
		pre = it;
		if(it) {
			it = it->next;
		}
	}
}

int main(int argc, char *argv[])
{
	FetcherPacket packet = {0};
	char c;
	int all = 0;
	char input[20];

	if(argc > 1) {
		FILE *fptr = fopen(argv[1], "r");
		if(fptr) {
			while(fscanf(fptr, " %s", input) != EOF) {
				hook_add_register(input);
			}
		}
	}

	int go = 1;
	while(go) {
		printf("Action: (a)Display all registers, (d)Select register, (s)Start: ");
		scanf(" %c", &c);
		switch(c) {
		case 'd':
			printf("Register name: ");
			scanf(" %s", input);
			hook_add_register(input);
			break;
		case 'a':
			printf("Display all registers, auto start\n");
			all = 1;
			/* Fall through */
		case 's':
			printf("Start connection with QEMU\n");
			go = 0;
			break;
		default:
			printf("Invalid operation\n");
		}
	}

	/* Connect to QEMU */
        conn();

	/* Handle each packet received from QEMU */
        while(fread(&packet, sizeof(FetcherPacket), 1, fp)) {
		if(all) {
			print_all_registers(packet);
		}
		else {
			hook_print(packet);
		}
        }

	/* Free dynamic allocate memory */
	hook_destructor();

	return 0;
}
