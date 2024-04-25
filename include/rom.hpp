#pragma once
#include "pico/stdlib.h"
#include <stdio.h>

typedef struct {
    // General
    const char * name;
    size_t size;
    bool readonly;

    // Clock
    bool invertClock;
    uint pulseDelayUs;
    uint byteDelayUs;

    // Paging
    size_t pageSize;
    uint pageDelayMs;

    // Write Protection
    bool writeProtect;
    bool writeProtectDisable;

    void print() {
        printf("Device: %s\r\n", name);
        printf("\tCapacity: %dK bytes\r\n", size / 1024);
        printf("\tRead-only: %s\r\n", readonly ? "yes" : "no");

        printf("\tInverted clock: %s\r\n", invertClock ? "on" : "off");
        printf("\tPulse delay: %dus\r\n", pulseDelayUs);
        printf("\tByte delay: %dus\r\n", byteDelayUs);

        if (!readonly) {
            printf("\tPaging: %s\r\n", pageSize ? "on" : "off");
            if (pageSize) {
                printf("\tPage size: %d bytes\r\n", pageSize);
                printf("\tPage delay: %dms\r\n", pageDelayMs);
            }
            printf("\tWrite protect: %s\r\n", writeProtect ? "enable" : (writeProtectDisable ? "disable" : "no action / not supported"));
        }
    };
} rom_config_t;

typedef uint8_t (*data_func_t)(size_t address);

class ROM {

public:
    ROM(rom_config_t config);
    ~ROM();

    const rom_config_t * get_config() const;
    size_t get_size() const;
    size_t get_page_size() const;

    bool read(uint8_t * data, size_t size, size_t offset, bool print_status);
    bool read(uint8_t * data, size_t size, size_t offset);
    bool read(uint8_t * data, size_t size);
    bool read(uint8_t * data);

    bool write_image(const uint8_t * data, size_t size, size_t offset, bool print_status);
    bool write_image(const uint8_t * data, size_t size, size_t offset);
    bool write_image(const uint8_t * data, size_t size);
    bool write_value(uint8_t value, bool print_status);
    bool write_value(uint8_t value);
    bool write_random(bool print_status);
    bool write_random();
    bool write_index(bool print_status);
    bool write_index();

    size_t verify_image(uint8_t * data, size_t size, size_t offset, bool print_status);
    size_t verify_image(uint8_t * data, size_t size, size_t offset);
    size_t verify_image(uint8_t * data, size_t size);
    size_t verify_value(uint8_t value, bool print_status);
    size_t verify_value(uint8_t value);
    size_t verify_index(bool print_status);
    size_t verify_index();

    void print();

private:
    rom_config_t config;

    bool write(data_func_t cb, size_t size, size_t offset, bool print_status);

    size_t verify(data_func_t cb, size_t size, size_t offset, bool print_status);

    void status(size_t address, bool output);
    void set_address(size_t address);

    void set_data_direction(bool direction);
    void set_data(uint8_t value);
    uint8_t get_data();

    bool write_byte(size_t address, uint8_t value);
    uint8_t read_byte(size_t address);

};
