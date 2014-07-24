#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "types.h"
#include "ui.h"

#include "regs.h"

#define CONSOLE_LINES	15
#define CONSOLE_COLS	COLS
#define DISPLAY_LINES	(LINES - CONSOLE_LINES)
#define DISPLAY_COLS	COLS
#define DISPLAY_Y	0
#define DISPLAY_X	0
#define CONSOLE_Y	(DISPLAY_Y + DISPLAY_LINES)
#define CONSOLE_X	0

/* Global Variables */
HookRegisters *hook_head;

/* UI Design
 * Split the whole window to two parts:
 *    1 - The upper part is used to print large information. ex. display registers
 *    2 - The lower part is command line(console) which interacts with user.
 * Upper part use maximum window height - 15, lower use remains.
 */

/* Display Design
 * Show display registers.
 */
WINDOW *display_win;
static void display_init()
{
	/* Create a window which size is LINES - 15 * COLS
	 * and with border around */
	display_win = newwin(DISPLAY_LINES, DISPLAY_COLS, DISPLAY_Y, DISPLAY_X);
	box(display_win, 0, 0);

	wrefresh(display_win);
}

void display_update(FetcherPacket packet)
{
	HookRegisters *it;
	int y = DISPLAY_Y + 1, x = DISPLAY_X + 1;

	wclear(display_win);
	box(display_win, 0, 0);

	for(it = hook_head; it != NULL; it = it->next) {
		ARMCPRegInfo tmp = reg_array[it->pos].array[it->index];
		mvwprintw(display_win, y, x, "%-16s = ", tmp.name);
		x += 19;
		switch(tmp.type) {
		case ARM_CP_UNIMPL:
			mvwprintw(display_win, y, x, "UNIMPLEMENTED");
			break;
		case ARM_CP_CONST:
			mvwprintw(display_win, y, x, "0x%lx", tmp.const_value);
			break;
		case ARM_CP_NORMAL_L:
			mvwprintw(display_win, y, x, "0x%x", *(uint32_t *)((uint8_t *)(&packet) + tmp.fieldoffset));
			break;
		case ARM_CP_NORMAL_H:
			mvwprintw(display_win, y, x, "0x%lx", *(uint64_t *)((uint8_t *)(&packet) + tmp.fieldoffset));
			break;
		}
		y++;
		x = DISPLAY_X + 1;
	}

	wrefresh(display_win);
}

/* Free dynamic allocate memory */
static void display_destructor(void)
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

/* Add hook registers which need to be trace */
void display_add(char *input)
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
		console_puts("Invalid register name\n");
		return;
	}

	HookRegisters *tmp = (HookRegisters *)malloc(sizeof(HookRegisters));
	tmp->pos = i;
	tmp->index = j;
	tmp->next = NULL;
	console_puts("Add register ");
	console_puts(input);
	console_puts(" to hook list\n");

	/* First element */
	if(it == NULL) {
		hook_head = tmp;
		return;
	}

	/* Iterate to last element */
	for(; it != NULL; it = it->next) {
		if(!strcmp(reg_array[it->pos].array[it->index].name, input)) {
			console_puts("Register ");
			console_puts(input);
			console_puts(" has already in hook list\n");
			return;
		}

		if(it->next == NULL) {
			it->next = tmp;
			break;
		}
	}
}


/* Console Design
 * We need to handle console ourself. Try to make it like normal stdout act.
 * Provide some API for programmer use:
 *    * putc - put a character on console
 */
WINDOW *console_win;
pthread_mutex_t mutex;
static void console_init()
{
	pthread_mutex_init(&mutex, NULL);

	console_win = newwin(CONSOLE_LINES, CONSOLE_COLS, CONSOLE_Y, CONSOLE_X);
	wrefresh(console_win);
}

static void console_putc(char c)
{
	static int y = 0, x = 0; // Record current cursor pos

	/* Manipulate output character */
	switch(c) {
	case '\n':
		y++;
		x = 0;
		break;
	default:
		mvwaddch(console_win, y, x, c);
		x++;
	}

	/* Manipulate cursor postion */
	if(x >= CONSOLE_COLS) { // reach column limit, new line
		x = 0;
		y++;
	}
	if(y >= CONSOLE_LINES - 1) { // reach line limit, clear console
		/* Clear page notice */
		mvwaddstr(console_win, y, 0, "---- Type any key to continue ----");
		wrefresh(console_win);
		getchar();
		wclear(console_win);

		x = 0;
		y = 0;
	}

	wrefresh(console_win);
}

void console_puts(char *str)
{
	pthread_mutex_lock(&mutex);
	char *cptr;

	for(cptr = str; *cptr != '\0'; cptr++) {
		console_putc(*cptr);
	}
	pthread_mutex_unlock(&mutex);
}

static void console_parser(char *str)
{
	char *pch;

	/* First level parse */
	pch = strtok(str, " \n");

	if(!strcmp(pch, "add")) {
		pch = strtok(NULL, " \n");
		display_add(pch);
	}
	else {
		console_puts("Invalid command!\n");
	}
}

void console_prompt(void)
{
	char line_buf[128];
	char c;
	int count = 0;

	console_puts("-> ");
	while(1) {
		c = getch();
		if(c == '\n') {
			console_putc(c);
			line_buf[count++] = '\0';
			if(count > 1) {
				console_parser(line_buf);
			}
			count = 0;
			console_puts("-> ");
		}
		/* FIXME can not recongize backspace */
		else if(c == 127 || c == 8) {
			console_putc('b');
			count--;
		}
		else {
			console_putc(c);
			line_buf[count++] = c;
		}
	}
}

/* UI initiallize and destructor */
void ui_init(void)
{
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	refresh();

	display_init();
	console_init();
}

void ui_destroy(void)
{
	display_destructor();
	endwin();
}
