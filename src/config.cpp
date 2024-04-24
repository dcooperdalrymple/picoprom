#include "config.hpp"

static rom_config_t configs_eeprom[] = {
    {
        "AT28C256",
        32768,
        false,
        false,
        1,
        0,
        64,
        10,
        true,
        false
    },
    {
        "AT28C256F",
        32768,
        false,
        false,
        1,
        0,
        64,
        3,
        true,
        false
    },
    {
        "AT28C64",
        8192,
        false,
        false,
        1,
        1000,
        0,
        10,
        false,
        false
    },
    {
        "AT28C64B",
        8192,
        false,
        false,
        1,
        0,
        64,
        10,
        true,
        false
    },
    {
        "AT28C64E",
        8192,
        false,
        false,
        1,
        200,
        0,
        10,
        false,
        false
    },
    {
        "AT28C16",
        2048,
        false,
        false,
        1,
        1000,
        0,
        10,
        false,
        false
    },
    {
        "AT28C16E",
        2048,
        false,
        false,
        1,
        200,
        0,
        10,
        false,
        false
    },
    {
        "M28C16",
        2048,
        false,
        false,
        1,
        0,
        64,
        3,
        true,
        false
    },
    {
        NULL
    }
};

static rom_config_t configs_atari[] = {
    {
        "2K Cartridge",
        2048,
        true,
        true,
        1
    },
    {
        "4K Cartridge",
        4096,
        true,
        true,
        1
    },
    {
        NULL
    }
};

static config_category_t configs[] = {
    {
        "EEPROM",
        &configs_eeprom[0]
    },
    {
        "Atari 2600/VCS",
        &configs_atari[0]
    },
    {
        NULL
    }
};

static int config_category_index = 0;
static int config_index = 0;

void next_config_category() {
    config_category_index++;
    if (!configs[config_category_index].name) config_category_index = 0;
    config_index = 0;
};

void next_config() {
    config_index++;
    if (!configs[config_category_index].items[config_index].name) config_index = 0;
};

const char * get_config_category_name() {
    return configs[config_category_index].name;
};

const rom_config_t get_config() {
    return configs[config_category_index].items[config_index];
};

void print_config() {
    printf("Category: %s\r\n", get_config_category_name());
    configs[config_category_index].items[config_index].print();
};
