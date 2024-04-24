#include "storage.hpp"

#include <lfs.h>
#include <hardware/flash.h>
#include <hardware/sync.h>

// littlefs configuration

static lfs_t lfs;
static lfs_file_t file;
static lfs_info info;
static lfs_dir_t dir;

static int lfs_pico_read(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
    uint8_t *ffs_mem  = (uint8_t *) cfg->context;

	// check if read is valid
	LFS_ASSERT (off  % cfg->read_size == 0);
	LFS_ASSERT (size % cfg->read_size == 0);
	LFS_ASSERT (block < cfg->block_count);

	// read data
	memcpy (buffer, &ffs_mem[block*cfg->block_size + off], size);

	return 0;
};

static int lfs_pico_prog(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    uint8_t *ffs_mem  = (uint8_t *) cfg->context;

	// check if write is valid
	LFS_ASSERT (off  % cfg->prog_size == 0);
	LFS_ASSERT (size % cfg->prog_size == 0);
	LFS_ASSERT (block < cfg->block_count);

	// program data
	uint32_t ints = save_and_disable_interrupts ();
	flash_range_program (&ffs_mem[block*cfg->block_size + off]
		-(uint8_t *)XIP_BASE, (const uint8_t *)buffer, size);
	restore_interrupts (ints);

	return 0;
};

static int lfs_pico_erase(const struct lfs_config *cfg, lfs_block_t block) {
    uint8_t *ffs_mem  = (uint8_t *) cfg->context;

	// check if erase is valid
	LFS_ASSERT (block < cfg->block_count);

	uint32_t ints = save_and_disable_interrupts ();
	flash_range_erase (&ffs_mem[block*cfg->block_size]
		-(uint8_t *)XIP_BASE, cfg->block_size);
	restore_interrupts (ints);

	return 0;
};

static int lfs_pico_sync(const struct lfs_config *cfg) {
	// sync does nothing because we aren't backed by anything real
	return 0;
};

struct lfs_config cfg;

void init_filesystem() {
    // Setup configuration
    memset(&cfg, 0, sizeof(struct lfs_config));
    cfg.context         = (void *) (XIP_BASE + ROOT_OFFSET);
    cfg.read            = lfs_pico_read;
    cfg.prog            = lfs_pico_prog;
    cfg.erase           = lfs_pico_erase;
    cfg.sync            = lfs_pico_sync;
    cfg.read_size       = 1;
    cfg.prog_size       = FLASH_PAGE_SIZE;
    cfg.block_size      = FLASH_SECTOR_SIZE;
    cfg.block_count     = ROOT_SIZE / FLASH_SECTOR_SIZE;
    cfg.cache_size      = FLASH_PAGE_SIZE; // 256?
    cfg.lookahead_size  = 32;
    cfg.block_cycles    = 256;

    if (lfs_mount(&lfs, &cfg)) {
        // Format if first boot
        lfs_format(&lfs, &cfg);
        lfs_mount(&lfs, &cfg);
    }
};

bool reformat_filesystem() {
    lfs_unmount(&lfs);
    lfs_format(&lfs, &cfg);
    lfs_mount(&lfs, &cfg);
    // TODO: Error handling?
    return true;
};

// File helpers

static char invalid_chars[] = {
    ' ', '^', '<', '>',
    ';', '|', '\'', '/',
    ',', '\\', ':', '=',
    '?', '"', '*'
};

bool file_exists(const char * path) {
    int err = lfs_stat(&lfs, path, &info);
    return err >= 0;
};

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

// File operations

bool write_file(const char * path, const uint8_t * buffer, size_t size) {
    if (file_exists(path)) delete_file(path);
    lfs_file_open(&lfs, &file, path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL);
    lfs_ssize_t write_size = lfs_file_write(&lfs, &file, buffer, size);
    lfs_file_close(&lfs, &file);
    return write_size > 0;
};

size_t read_file(const char * path, uint8_t * buffer, size_t buffer_size) {
    lfs_file_open(&lfs, &file, path, LFS_O_RDONLY);
    lfs_ssize_t size = lfs_file_read(&lfs, &file, buffer, buffer_size);
    lfs_file_close(&lfs, &file);
    if (size < 0) return 0;
    return (size_t)size;
};

bool delete_file(const char * path) {
    return lfs_remove(&lfs, path) >= 0;
};

// Directory operations

bool valid_dir_item(bool include_dir) {
    if (!include_dir && info.type == LFS_TYPE_DIR) return false;
    if (info.type == LFS_TYPE_REG && !valid_filename(info.name, false)) return false;
    return true;
};

size_t dir_count(const char * path, bool include_dir) {
    size_t count = 0;
    lfs_dir_open(&lfs, &dir, path);
    while (lfs_dir_read(&lfs, &dir, &info)) {
        if (!valid_dir_item(include_dir)) continue;
        count++;
    }
    lfs_dir_close(&lfs, &dir);
    return count;
};
size_t dir_count(const char * path) {
    return dir_count(path, false);
};
size_t dir_count() {
    return dir_count("/");
};

size_t get_dir_items(char items[][LFS_NAME_MAX+1], size_t size, const char * path, bool include_dir) {
    lfs_dir_open(&lfs, &dir, path);
    size_t i = 0, j;
    while (i < size-1 && lfs_dir_read(&lfs, &dir, &info)) {
        if (!valid_dir_item(include_dir)) continue;
        // NOTE: Could potentially use strcpy here
        for (j = 0; j < LFS_NAME_MAX; j++) {
            items[i][j] = info.name[j];
            if (!info.name[j]) {
                // Add size to file name
                // NOTE: Could potentially use sprintf here
                if (j < LFS_NAME_MAX - 6) {
                    items[i][j++] = ' ';
                    items[i][j++] = '(';
                    if (info.size/1024>10) items[i][j++] = (char)(0x30 + info.size/1024/10);
                    items[i][j++] = (char)(0x30 + info.size/1024%10);
                    items[i][j++] = 'K';
                    items[i][j++] = ')';
                    items[i][j] = 0;
                }
                break;
            }
        }
        i++;
    }
    lfs_dir_close(&lfs, &dir);
    items[i][0] = 0;
    return i;
};
size_t get_dir_items(char items[][LFS_NAME_MAX+1], size_t size, const char * path) {
    return get_dir_items(items, size, path, false);
};
size_t get_dir_items(char items[][LFS_NAME_MAX+1], size_t size) {
    return get_dir_items(items, size, "/", false);
};

void print_dir_items(const char * path, bool include_dir) {
    lfs_dir_open(&lfs, &dir, path);
    while (lfs_dir_read(&lfs, &dir, &info)) {
        if (!valid_dir_item(include_dir)) continue;
        printf("\t%s (%dK)\r\n", info.name, info.size/1024);
    }
    lfs_dir_close(&lfs, &dir);
};
void print_dir_items(const char * path) {
    print_dir_items(path, false);
};
void print_dir_items() {
    print_dir_items("/", false);
};
