#include "types.h"

extern char key_buffer[256];

void init_keyboard();
void keyboard_init();
char keyboard_getchar();
int check_keyboard_interrupt();
