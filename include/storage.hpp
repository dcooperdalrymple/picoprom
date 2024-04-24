#pragma once
#include "pico/stdlib.h"

#include "command.hpp"

// Use upper half of 2MB of flash on standard Pico (1MB total)
#define ROOT_SIZE 0x100000
#define ROOT_OFFSET 0x100000

bool file_exists(const char *fname);
bool valid_filename(const char * fn, bool output);
bool valid_filename(const char * fn);
bool valid_file(const struct dirent * ep);

void init_filesystem();

bool write_file(const char * path, const uint8_t * buffer, size_t size);
size_t read_file(const char * path, uint8_t * buffer, size_t bufferSize);
bool delete_file(const char * path);

size_t dir_count(const char * path, bool include_dir);
size_t dir_count(const char * path);
size_t dir_count();

Command * get_dir_items(const char * path, bool include_dir);
Command * get_dir_items(const char * path);
Command * get_dir_items();

void print_dir_items(const char * path, bool include_dir);
void print_dir_items(const char * path);
void print_dir_items();
