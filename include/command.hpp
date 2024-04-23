#pragma once
#include "pico/stdlib.h"

typedef void (*command_action_t)();

typedef struct Command {
    char key;
    const char * name;
    command_action_t action;
    void print() {
        printf("\t%c = %s\r\n", key, name);
    };
} command_t;

static Command * command_prompt(Command * items, const char * prompt, bool allow_return = false) {
    int i, c;
    while (true) {
        // Print prompt and command list

        printf("%s:\r\n", prompt);
        i = 0;
        while (items[i].key) items[i++].print();

        printf("\r\nEnter command");
        if (allow_return) printf(" (q to return)");
        printf(": ");

        c = getchar();
        if (c == PICO_ERROR_TIMEOUT) continue;
        printf("%c\r\n", c);

        if (allow_return && c == 'q') {
            printf("\r\n");
            return NULL;
        }

        i = 0;
        while (items[i].key && items[i].key != c) i++;
        if (items[i].key) break;

        printf("Invalid command, try again..\r\n\r\n");
    }
    printf("\r\n");
    return &items[i];
};
