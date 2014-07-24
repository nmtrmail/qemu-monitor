#ifndef __UI_H_
#define __UI_H_

#include "types.h"

void ui_init(void);
void ui_destroy(void);

void display_update(FetcherPacket packet);
void display_add(char *input);

void console_puts(char *str);
void console_prompt(void);

#endif
