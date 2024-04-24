#pragma once
#include "pico/stdlib.h"
#include <stdio.h>
#include <lfs.h>

typedef void (*command_action_t)();

typedef struct Command {
    char key;
    const char * name;
    command_action_t action;
    void print() {
        printf("\t%c = %s\r\n", key, name);
    };
} command_t;

static Command * command_prompt(Command * items, const char * prompt, bool allow_return) {
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
static Command * command_prompt(Command * items, const char * prompt) {
    return command_prompt(items, prompt, false);
};

static int option_prompt(char options[][LFS_NAME_MAX+1], const char * prompt, bool allow_return) {
    int i, c;
    while (true) {
        // Print prompt and command list

        printf("%s:\r\n", prompt);
        i = 0;
        while (options[i][0]) printf("\t%c = %s\r\n", (char)(i < 10 ? 0x30+i : 0x61+(i-10)), options[i++]);

        printf("\r\nEnter selection");
        if (allow_return) printf(" (q to return)");
        printf(": ");

        c = getchar();
        if (c == PICO_ERROR_TIMEOUT) continue;
        printf("%c\r\n", c);

        i = -1;
        if (allow_return && c == 'q') break;
        if (c >= 0x30 && c <= 0x39) i = (int)(c - 0x30);
        else if (c >= 0x61 && c <= 0x7A) i = (int)(c - 0x61 + 10);
        if (i >= 0) break;

        printf("Invalid selection, try again..\r\n\r\n");
    }
    printf("\r\n");
    return i;
};
static int option_prompt(char options[][LFS_NAME_MAX+1], const char * prompt) {
    return option_prompt(options, prompt, false);
};
