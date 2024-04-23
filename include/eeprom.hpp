#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "picoprom.hpp"

bool eeprom_init();

void eeprom_writeImage(const uint8_t* buffer, size_t size);
void eeprom_writeValue(uint8_t value);
void eeprom_writeRandom();
void eeprom_writeIndex();

size_t eeprom_verifyImage(const uint8_t* buffer, size_t size, size_t offset);
size_t eeprom_readImage(uint8_t* buffer, size_t size, size_t offset);
