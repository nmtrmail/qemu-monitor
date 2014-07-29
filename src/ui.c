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

#define MAX_LINE_WORDS	128

/* Global Variables */
HookRegisters *hook_head;
FetcherPacket prev_packet;

/* Command prototype */
void cmd_display(int argc, char *argv[]);
void cmd_undisplay(int argc, char *argv[]);
void cmd_print(int argc, char *argv[]);
void cmd_refresh(int argc, char *argv[]);
void cmd_help(int argc, char *argv[]);

/* Command array */
CMDDefinition cmd[] = {
	{.name = "display", .handler = cmd_display, .desc = "Display a register. -> display register_name"},
	{.name = "undisplay", .handler = cmd_undisplay, .desc = "Undisplay a register. -> undisplay register_name"},
	{.name = "print", .handler = cmd_print, .desc = "Print a register value. -> print register_name [end_bit [start_bit]]"},
	{.name = "refresh", .handler = cmd_refresh, .desc = "Refresh display register window."},
	{.name = "help", .handler = cmd_help, .desc = "Show this help guide."},
};


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
		mvwprintw(display_win, y, x, "%d: %-23s = ", it->id, it->name);
		x += 30;
		switch(it->type) {
		case ARM_CP_UNIMPL:
			mvwprintw(display_win, y, x, "UNIMPLEMENTED");
			break;
		case ARM_CP_CONST:
			mvwprintw(display_win, y, x, "0x%lx", it->const_value);
			break;
		case ARM_CP_NORMAL_L:
			if((*(uint32_t *)((uint8_t *)(&packet) + it->fieldoffset) & it->mask)
			   != (*(uint32_t *)((uint8_t *)(&prev_packet) + it->fieldoffset) & it->mask)) {
				wattron(display_win, A_BOLD | A_UNDERLINE);
			}
			mvwprintw(display_win, y, x, "0x%x",
			          (*(uint32_t *)((uint8_t *)(&packet) + it->fieldoffset) & it->mask) >> it->start_bit);
			wattroff(display_win, A_BOLD | A_UNDERLINE);
			break;
		case ARM_CP_NORMAL_H:
			if((*(uint64_t *)((uint8_t *)(&packet) + it->fieldoffset) & it->mask)
			   != (*(uint64_t *)((uint8_t *)(&prev_packet) + it->fieldoffset) & it->mask)) {
				wattron(display_win, A_BOLD | A_UNDERLINE);
			}
			mvwprintw(display_win, y, x, "0x%lx",
			          (*(uint64_t *)((uint8_t *)(&packet) + it->fieldoffset) & it->mask) >> it->start_bit);
			wattroff(display_win, A_BOLD | A_UNDERLINE);
			break;
		}
		y++;
		x = DISPLAY_X + 1;
	}

	memcpy(&prev_packet, &packet, sizeof(FetcherPacket));

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
	case '\b':
		x--;
		wmove(console_win, y, x);
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

void console_puts(const char *str)
{
	pthread_mutex_lock(&mutex);
	const char *cptr;

	for(cptr = str; *cptr != '\0'; cptr++) {
		console_putc(*cptr);
	}
	pthread_mutex_unlock(&mutex);
}

static void parse_line(char *str)
{
	int count = 0;
	int i;
	char *pch;
	char *words[MAX_LINE_WORDS];

	for(pch = strtok(str, " \n"); pch != NULL; pch = strtok(NULL, " \n")) {
		strcpy(words[count] = malloc((strlen(pch) + 1) * sizeof(char)), pch);
		count++;
	}

	if(count == 0) { // empty string
		return;
	}

	/* Find command */
	for(i = 0; i < sizeof(cmd) / sizeof(CMDDefinition); i++) {
		if(!strcmp(cmd[i].name, words[0])) {
			break;
		}
	}

	if(i == sizeof(cmd) / sizeof(CMDDefinition)) { // cmd not found
		console_puts("Undefined command: \"");
		console_puts(words[0]);
		console_puts("\"\n");
	}
	else {
		cmd[i].handler(count - 1, (words + 1));
	}

	/* Free dynamic allocate memory for words */
	for(i = 0; i < count; i++) {
		free(words[i]);
	}
}

void console_prompt(void)
{
	char line_buf[128];
	int c;
	int count = 0;

	console_puts("-> ");
	while(1) {
		c = getch();
		switch(c) {
		case '\n':
			console_putc('\n');
			line_buf[count++] = '\0';
			parse_line(line_buf);
			count = 0;
			console_puts("-> ");
			break;
		// TODO: command history
		case KEY_DOWN:
			break;
		case KEY_UP:
			break;
		// TODO: console command cursor movement
		case KEY_LEFT:
			break;
		case KEY_RIGHT:
			break;
		case KEY_BACKSPACE:
			if(count > 0) {
				console_putc('\b');
				console_putc(' ');
				console_putc('\b');
				count--;
			}
			break;
		default:
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

/* Command handler implementation */
void cmd_display(int argc, char *argv[])
{
	HookRegisters *it = hook_head;
	int i, j;
	int valid = 0;
	int start_bit = 0, end_bit;
	uint64_t mask = 0xFFFFFFFFFFFFFFFF;
	char *pch;
	char reg_name[64];

	if(argc > 1) {
		console_puts("Too many arguments\n");
		return;
	}

	/* Parse register name.
	 * Register name format: leading dollar sign and optional bit field
	 * $reg_name, $reg_name[end_bit:start_bit]
	 */
	if(argv[0][0] != '$') {
		console_puts("Invalid register name\n");
		return;
	}
	if((pch = strchr(argv[0], '[')) != NULL) {
		sscanf(pch, "[%d:%d]", &end_bit, &start_bit);
		int len = end_bit - start_bit;

		mask >>= (63 - len);
		mask <<= start_bit;
		strncpy(reg_name, argv[0] + 1, pch - argv[0] - 1);
	}
	else {
		strncpy(reg_name, argv[0] + 1, 64);
	}

	/* Search register in registers array */
	for(i = 0; i < sizeof(reg_array) / sizeof(struct ARMCPRegArray); i++) {
		for(j = 0; j < reg_array[i].size; j++) {
			if(!strcmp(reg_array[i].array[j].name, reg_name)) {
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
	strncpy(tmp->name, argv[0] + 1, 64);
	tmp->next = NULL;
	tmp->mask = mask;
	tmp->const_value = reg_array[i].array[j].const_value;
	tmp->type = reg_array[i].array[j].type;
	tmp->fieldoffset = reg_array[i].array[j].fieldoffset;
	tmp->start_bit = start_bit;

	console_puts("Add register \"");
	console_puts(argv[0]);
	console_puts("\" to hook list\n");

	/* First element */
	if(it == NULL) {
		tmp->id = 0;
		hook_head = tmp;
		display_update(prev_packet);
		return;
	}

	/* Iterate to last element */
	for(; it != NULL; it = it->next) {
		if(!strcmp(it->name, argv[0] + 1)) {
			console_puts("Register \"");
			console_puts(argv[0] + 1);
			console_puts("\" has already in hook list\n");
			return;
		}

		if(it->next == NULL) {
			tmp->id = it->id + 1;
			it->next = tmp;
			break;
		}
	}

	display_update(prev_packet);
}

void cmd_undisplay(int argc, char *argv[])
{
	/*
	HookRegisters *it = hook_head, *prev;

	if(it == NULL) {
		console_puts("Display list is empty!\n");
		return;
	}

	for(prev = NULL; it != NULL; prev = it, it = it->next) {
		if(!strcmp(reg_array[it->pos].array[it->index].name, argv[0])) {
			if(prev == NULL) {
				hook_head = it->next;
				free(it);
			}
			else {
				prev->next = it->next;
				free(it);
			}
			console_puts("Undisplay \"");
			console_puts(argv[0]);
			console_puts("\"\n");
			display_update(prev_packet);
			return;
		}
	}
	console_puts("Register \"");
	console_puts(argv[0]);
	console_puts("\" is not in the display list!\n");
	*/
}

void cmd_print(int argc, char *argv[])
{
	ARMCPRegInfo tmp;
	uint64_t value;
	int valid = 0, i, j;
	char str[128] = {0};

	/* Search register in registers array */
	for(i = 0; i < sizeof(reg_array) / sizeof(struct ARMCPRegArray); i++) {
		for(j = 0; j < reg_array[i].size; j++) {
			if(!strcmp(reg_array[i].array[j].name, argv[0])) {
				valid = 1;
				tmp = reg_array[i].array[j];
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

	switch(tmp.type) {
	case ARM_CP_UNIMPL:
		console_puts("UNIMPLEMENTED\n");
		return;
	case ARM_CP_CONST:
		value = tmp.const_value;
		break;
	case ARM_CP_NORMAL_L:
		value = *(uint32_t *)((uint8_t *)(&prev_packet) + tmp.fieldoffset);
		break;
	case ARM_CP_NORMAL_H:
		value = *(uint64_t *)((uint8_t *)(&prev_packet) + tmp.fieldoffset);
		break;
	}

	uint64_t mask = 0xFFFFFFFFFFFFFFFF;
	if(argc == 1) {
		sprintf(str, "%s = 0x%lx\n", tmp.name, value);
	}
	else if(argc == 2) {
		int end_bit = atoi(argv[1]);
		mask >>= (63 - end_bit);
		sprintf(str, "%s[%d:0] = 0x%lx\n", tmp.name, end_bit, value & mask);
	}
	else if(argc == 3) {
		int end_bit, start_bit;
		end_bit = atoi(argv[1]);
		start_bit = atoi(argv[2]);
		int len = end_bit - start_bit;

		mask >>= (63 - len);
		mask <<= start_bit;
		sprintf(str, "%s[%d:%d] = 0x%lx\n", tmp.name, end_bit, start_bit, (value & mask) >> start_bit);
	}
	console_puts(str);
}

void cmd_refresh(int argc, char *argv[])
{
	display_update(prev_packet);
}

void cmd_help(int argc, char *argv[])
{
	int i;

	for(i = 0; i < sizeof(cmd) / sizeof(CMDDefinition); i++) {
		console_puts(cmd[i].name);
		console_puts(" : ");
		console_puts(cmd[i].desc);
		console_puts("\n");
	}
}
