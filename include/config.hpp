#pragma once

#include "rom.hpp"

typedef struct {
    const char * name;
    rom_config_t * items;
} config_category_t;

void next_config_category();
void next_config();
const char * get_config_category_name();
const rom_config_t get_config();
void print_config();
