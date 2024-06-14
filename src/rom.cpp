#include "rom.hpp"

#include "pico/rand.h"

static const uint LED_PIN = 25;

static const uint ADDR_MAP[] = { // A0-14
    12, 11, 10, 9, 8, 7, 6, 5, 28, 27, 22, 26, 4, 2 ,3
};

static const uint DATA_MAP[] = { // D0-7
    13, 14, 15, 16, 17, 18, 19, 20
};

static const uint CE_PIN = 21;
static const uint OE_PIN = 0;
static const uint WE_PIN = 1;

ROM::ROM(rom_config_t config) {
    this->config = config;

    gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, true);
	gpio_put(LED_PIN, true);

    for (uint8_t i = 0; i < sizeof(ADDR_MAP) / sizeof(*ADDR_MAP); i++) {
        gpio_init(ADDR_MAP[i]);
        gpio_set_dir(ADDR_MAP[i], true);
    }
    this->set_address(this->config.addressMask);

    for (uint8_t i = 0; i < sizeof(DATA_MAP) / sizeof(*DATA_MAP); i++) {
        gpio_init(DATA_MAP[i]);
        gpio_set_dir(DATA_MAP[i], false);
    }

    gpio_init(CE_PIN);
	gpio_set_dir(CE_PIN, true);
	gpio_put(CE_PIN, !this->config.invertClock);

    if (!this->config.readonly) {
        gpio_init(OE_PIN);
        gpio_set_dir(OE_PIN, true);
        gpio_put(OE_PIN, true);

        gpio_init(WE_PIN);
        gpio_set_dir(WE_PIN, true);
        gpio_put(WE_PIN, true);
    }
};

ROM::~ROM() {
    gpio_put(LED_PIN, false);
    gpio_deinit(LED_PIN);

    this->set_address(0);
    for (uint8_t i = 0; i < sizeof(ADDR_MAP) / sizeof(*ADDR_MAP); i++) {
        gpio_deinit(ADDR_MAP[i]);
    }

    for (uint8_t i = 0; i < sizeof(DATA_MAP) / sizeof(*DATA_MAP); i++) {
        gpio_deinit(DATA_MAP[i]);
    }

    gpio_put(CE_PIN, false);
    gpio_deinit(CE_PIN);

    if (!this->config.readonly) {
        gpio_put(OE_PIN, false);
        gpio_deinit(OE_PIN);

        gpio_put(WE_PIN, false);
        gpio_deinit(WE_PIN);
    }
};

const rom_config_t * ROM::get_config() const {
    return &this->config;
};

size_t ROM::get_size() const {
    return this->config.size;
};

size_t ROM::get_page_size() const {
    return this->config.pageSize;
};

bool ROM::read(uint8_t * data, size_t size, size_t offset, bool print_status) {
    if (offset > this->config.size) return false;
    if (!size) size = this->config.size;
    if (size > this->config.size - offset) size = this->config.size - offset;

    for (size_t address = offset; address < offset + size; address++) {
        data[address - offset] = this->read_byte(address);
        this->status(address, print_status);
    }
    return true;
};
bool ROM::read(uint8_t * data, size_t size, size_t offset) {
    return this->read(data, size, offset, true);
};
bool ROM::read(uint8_t * data, size_t size) {
    return this->read(data, size, 0, true);
};
bool ROM::read(uint8_t * data) {
    return this->read(data, this->config.size, 0, true);
};

const static uint8_t * _data_image;
uint8_t data_image(size_t address) {
    return _data_image[address];
};

static uint8_t _data_value = 0;
uint8_t data_value(size_t address) {
    return _data_value;
};

uint8_t data_random(size_t address) {
    return (uint8_t)(get_rand_32() & 0xff);
};

uint8_t data_index(size_t address) {
    return (uint8_t)(address & 0xff);
};

bool ROM::write_image(const uint8_t * data, size_t size, size_t offset, bool print_status) {
    _data_image = data;
    return this->write(data_image, size, offset, print_status);
};
bool ROM::write_image(const uint8_t * data, size_t size, size_t offset) {
    return this->write_image(data, size, offset, true);
};
bool ROM::write_image(const uint8_t * data, size_t size) {
    return this->write_image(data, size, 0, true);
};

bool ROM::write_value(uint8_t value, bool print_status) {
    _data_value = value;
    return this->write(data_value, this->config.size, 0, print_status);
};
bool ROM::write_value(uint8_t value) {
    return this->write_value(value, true);
};

bool ROM::write_random(bool print_status) {
    return this->write(data_random, this->config.size, 0, print_status);
};
bool ROM::write_random() {
    return this->write_random(true);
};

bool ROM::write_index(bool print_status) {
    return this->write(data_index, this->config.size, 0, print_status);
};
bool ROM::write_index() {
    return this->write_index(true);
};

bool ROM::write(data_func_t cb, size_t size, size_t offset, bool print_status) {
    if (this->config.readonly || size + offset > this->config.size) return false;
    if (this->config.writeProtectDisable) {
        this->write_byte(0x5555, 0xAA);
        this->write_byte(0x2AAA, 0x55);
        this->write_byte(0x5555, 0x80);
        this->write_byte(0x5555, 0xAA);
        this->write_byte(0x2AAA, 0x55);
        this->write_byte(0x5555, 0x20);
        if (this->config.pageDelayMs) sleep_ms(this->config.pageDelayMs);
    }
    for (size_t address = offset; address < offset + size; address++) {
        if (this->config.pageSize && (address % this->config.pageSize) == 0) {
            if (this->config.pageDelayMs) sleep_ms(this->config.pageDelayMs);
            if (this->config.writeProtect) {
                // Locking prefix
                this->write_byte(0x5555, 0xAA);
                this->write_byte(0x2AAA, 0x55);
                this->write_byte(0x5555, 0xA0);
            }
        }
        this->write_byte(address, cb(address - offset));
        this->status(address, print_status);
    }
    if (this->config.pageDelayMs) sleep_ms(this->config.pageDelayMs);
    return true;
};

size_t ROM::verify_image(uint8_t * data, size_t size, size_t offset, bool print_status) {
    _data_image = data;
    return this->verify(data_image, size, offset, print_status);
};
size_t ROM::verify_image(uint8_t * data, size_t size, size_t offset) {
    return this->verify_image(data, size, offset, true);
};
size_t ROM::verify_image(uint8_t * data, size_t size) {
    return this->verify_image(data, size, 0, true);
};

size_t ROM::verify_value(uint8_t value, bool print_status) {
    _data_value = value;
    return this->verify(data_value, this->config.size, 0, print_status);
};
size_t ROM::verify_value(uint8_t value) {
    return this->verify_value(value, true);
};

size_t ROM::verify_index(bool print_status) {
    return this->verify(data_index, this->config.size, 0, print_status);
};
size_t ROM::verify_index() {
    return this->verify_index(true);
};

size_t ROM::verify(data_func_t cb, size_t size, size_t offset, bool print_status) {
    if (size + offset > this->config.size) return -1;
    size_t error = 0;
    for (size_t address = offset; address < offset + size; address++) {
        if (this->read_byte(address) != cb(address - offset)) error += 1;
        this->status(address, print_status);
    }
    return error;
};

void ROM::print() {
    this->config.print();
};

void ROM::status(size_t address, bool output) {
    gpio_put(LED_PIN, (address & 0x100) != 0);
    if (output && ((address + 1) & 0x7FF) == 0) printf("%dK ", (address + 1) >> 10);
};

void ROM::set_address(size_t address) {
    address |= this->config.addressMask;
    for (uint8_t i = 0; i < sizeof(ADDR_MAP) / sizeof(*ADDR_MAP); i++) {
        gpio_put(ADDR_MAP[i], address & (1 << i));
    }
};

void ROM::set_data_direction(bool out) {
    if (this->config.readonly) return;
    for (uint8_t i = 0; i < sizeof(DATA_MAP) / sizeof(*DATA_MAP); i++) {
        gpio_set_dir(DATA_MAP[i], out);
    }
};

void ROM::set_data(uint8_t value) {
    if (this->config.readonly) return;
    this->set_data_direction(true);
    for (uint8_t i = 0; i < sizeof(DATA_MAP) / sizeof(*DATA_MAP); i++) {
        gpio_put(DATA_MAP[i], value & (1 << i));
    }
};

uint8_t ROM::get_data() {
    uint8_t value = 0;
    if (!this->config.readonly) this->set_data_direction(false);
    for (uint8_t i = 0; i < sizeof(DATA_MAP) / sizeof(*DATA_MAP); i++) {
        if (gpio_get(DATA_MAP[i])) value += 1 << i;
    }
    return value;
};

bool ROM::write_byte(size_t address, uint8_t value) {
    if (this->config.readonly) return false;
    gpio_put(OE_PIN, true);
    gpio_put(WE_PIN, false);
    gpio_put(CE_PIN, true);
    this->set_address(address);
    this->set_data(value);
    if (this->config.pulseDelayUs) busy_wait_us(this->config.pulseDelayUs);
    gpio_put(CE_PIN, false);
    if (this->config.pulseDelayUs) busy_wait_us(this->config.pulseDelayUs);
    gpio_put(CE_PIN, true);
    if (this->config.byteDelayUs) busy_wait_us(this->config.byteDelayUs);
    gpio_put(WE_PIN, true);
    return true;
};

uint8_t ROM::read_byte(size_t address) {
    uint8_t value;
    if (!this->config.readonly) this->set_data_direction(false);
    gpio_put(CE_PIN, !this->config.invertClock);
    if (!this->config.readonly) {
        gpio_put(OE_PIN, true);
        gpio_put(WE_PIN, true);
    }
    this->set_address(address);
    if (this->config.pulseDelayUs) busy_wait_us(this->config.pulseDelayUs);
    gpio_put(CE_PIN, this->config.invertClock);
    if (!this->config.readonly) gpio_put(OE_PIN, false);
    if (this->config.pulseDelayUs) busy_wait_us(this->config.pulseDelayUs);
    value = this->get_data();
    gpio_put(CE_PIN, !this->config.invertClock);
    if (!this->config.readonly) gpio_put(OE_PIN, true);
    if (this->config.pulseDelayUs) busy_wait_us(this->config.pulseDelayUs);
    if (!this->config.readonly) this->set_data_direction(true);
    return value;
};
