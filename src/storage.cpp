#include "storage.hpp"

#include <stddef.h>
#include <stdint.h>
#include <pfs.h>
#include "lfs.h"
#include <cstdio>
#define _DEFAULT_SOURCE // Enables file type constants
#include <dirent.h>
#include "pico/stdlib.h"

static struct pfs_pfs *pfs;
static struct lfs_config cfg;

void init_filesystem() {
    ffs_pico_createcfg(&cfg, ROOT_OFFSET, ROOT_SIZE);
    pfs = pfs_ffs_create(&cfg);
    pfs_mount(pfs, "/");
};

static char invalid_chars[] = {
    ' ', '^', '<', '>',
    ';', '|', '\'', '/',
    ',', '\\', ':', '=',
    '?', '"', '*'
};

bool file_exists(const char *fname) {
    FILE *file;
    if ((file = fopen(fname, "rb")) != NULL) {
        fclose(file);
        return true;
    }
    return false;
}

bool valid_filename(const char * fn, bool output) {
    if (!fn[0] || fn[0] == '.') return false;
    size_t i = 0, j;
    do {
        for (j = 0; j < sizeof(invalid_chars) / sizeof(invalid_chars[0]); j++) {
            if (fn[i] != invalid_chars[j]) continue;
            if (output) printf("Invalid character detected: \"%c\"\r\n", fn[i]);
            return false;
        }
    } while (fn[++i]);
    return true;
};
bool valid_filename(const char * fn) {
    return valid_filename(fn, true);
};

bool valid_file(const struct dirent * ep) {
    return valid_filename(ep->d_name, false);
};

bool write_file(const char * path, const uint8_t * buffer, size_t size) {
    FILE * fp = fopen(path, "wb");
    if (fp == NULL) return false;
    fwrite(buffer, size, 1, fp);
    fclose(fp);
    return true;
};

size_t read_file(const char * path, uint8_t * buffer, size_t bufferSize) {
    FILE * fp = fopen(path, "rb");
    size_t size = 0;
    uint8_t c;
    while ((c = (uint8_t)getc(fp)) != EOF && size < bufferSize) {
        buffer[size++] = c;
    }
    fclose(fp);
    return size;
};

bool delete_file(const char * path) {
    if (!file_exists(path)) return false;
    return remove(path) == 0;
};

size_t dir_count(const char * path, bool include_dir) {
    size_t count = 0;
    DIR * dp;
    struct dirent * ep;
    dp = opendir(path);
    while ((ep = readdir(dp)) != NULL) {
        if (!include_dir && !valid_file(ep)) continue;
        count++;
    }
    closedir(dp);
    return count;
};
size_t dir_count(const char * path) {
    return dir_count(path, false);
};

size_t dir_count() {
    return dir_count("/");
};

Command * get_dir_items(const char * path, bool include_dir) {
    size_t count = dir_count(path, include_dir);
    if (!count) return NULL;
    Command * items = new Command[count+1];

    DIR * dp;
    struct dirent * ep;
    dp = opendir(path);
    size_t i = 0;
    while (i < count && (ep = readdir(dp)) != NULL) {
        if (!include_dir && !valid_file(ep)) continue;
        items[i] = {
            (char)(i < 10 ? 0x30+i : 0x61+(i-10)), // BUG: max is 35 (0-9, a-z) before wonkiness
            (const char *)ep->d_name
        };
        i++;
    }
    closedir(dp);
    items[i] = { 0 };

    return items;
};
Command * get_dir_items(const char * path) {
    return get_dir_items(path, false);
};
Command * get_dir_items() {
    return get_dir_items("/", false);
};

void print_dir_items(const char * path, bool include_dir) {
    DIR * dp;
    struct dirent * ep;
    dp = opendir(path);
    while ((ep = readdir(dp)) != NULL) {
        if (!include_dir && !valid_file(ep)) continue;
        printf("\t%s\r\n", ep->d_name);
    }
    closedir(dp);
};
void print_dir_items(const char * path) {
    print_dir_items(path, false);
};
void print_dir_items() {
    print_dir_items("/", false);
};
