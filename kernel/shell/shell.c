#include <string.h>
#include <stdio.h>
#include <drivers/screen.h>

void shell_execute_command(char *input) {
    if (strcmp(input, "CLEAR") == 0) {
        clear_screen();
    } 
    else if (strncmp(input, "ECHO ", 4) == 0) {
        kprint("\n");
        kprint(input + 5);
    } 
    else if (strcmp(input, "TETRIS") == 0) {
        // open da game
    }
    else {
        kprint("\nUnknown command: ");
        kprint(input);
    }
    for (int i = 0; i < 256; i++) {
        input[i] = NULL;
    }
}