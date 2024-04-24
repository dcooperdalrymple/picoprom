#pragma once
#include "pico/stdlib.h"
#include <lfs.h>

#include "command.hpp"

// Use upper half of 2MB of flash on standard Pico (1MB total)
#define ROOT_SIZE 0x100000
#define ROOT_OFFSET 0x100000

bool file_exists(const char * path);
bool valid_filename(const char * fn, bool output);
bool valid_filename(const char * fn);

void init_filesystem();
bool reformat_filesystem();

bool write_file(const char * path, const uint8_t * buffer, size_t size);
size_t read_file(const char * path, uint8_t * buffer, size_t buffer_size);
bool delete_file(const char * path);

size_t dir_count(const char * path, bool include_dir);
size_t dir_count(const char * path);
size_t dir_count();

size_t get_dir_items(char items[][LFS_NAME_MAX+1], size_t size, const char * path, bool include_dir);
size_t get_dir_items(char items[][LFS_NAME_MAX+1], size_t size, const char * path);
size_t get_dir_items(char items[][LFS_NAME_MAX+1], size_t size);

void print_dir_items(const char * path, bool include_dir);
void print_dir_items(const char * path);
void print_dir_items();
