#include "eeprom.h"

#include <stdio.h>

#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico/rand.h"

#include "picoprom.h"
#include "configs.h"


/* Configure which EEPROM pin is connected to each GPIO pin.
 *
 * EEPROM pins are referenced here as follows:
 *    A0-A14      0-14
 *    CE          15
 *    I/O0-I/O7   16-23
 *    OE		  24 (VERIFY_ROM mode only)
 *    WE  		  25 (VERIFY_ROM mode only)
 *
 * VCC, GND, WE and OE are not driven and should be permanently connected
 * to the supply rails (OE to VCC, CE to GND). (not VERIFY_ROM mode)
 *
 * Negative numbers indicate none of the above are connected to this GPIO pin.
 */
static const int gPinMapping[] =
{
	#if VERIFY_ROM
	24, 25,
	#else
	-1, -1,
	#endif
	         13, 14, 12, 7,  6, 5, 4, 3,  2, 1, 0, 16,  17, 18,

	19, 20,  21, 22, 23, 15,  10,  -1, -1, -1,  11, 9,  8,
};

static const int gLedPin = 25;

int gChipEnablePin;
#if VERIFY_ROM
int gOutputEnablePin;
int gWriteEnablePin;
#endif

/* Mask of all GPIO bits used for EEPROM pins */
static uint32_t gAllBits;
#if VERIFY_ROM
static uint32_t gDataBits;
static uint32_t gControlBits;
static int gDataPins[8];
#endif


/* Pack address, data, chipEnable, outputEnable, and writeEnable into a GPIO word */
#if VERIFY_ROM
static uint32_t packGpioBits(uint16_t address, uint8_t data, bool chipEnable, bool outputEnable, bool writeEnable)
#else
static uint32_t packGpioBits(uint16_t address, uint8_t data, bool chipEnable)
#endif
{
	#if VERIFY_ROM
	uint32_t tempPacking = address | ((uint32_t)data << 16) | (!!chipEnable << 15) | (!!outputEnable << 24) | (!!writeEnable << 25);
	#else
	uint32_t tempPacking = address | ((uint32_t)data << 16) | (!!chipEnable << 15);
	#endif

	uint32_t realPacking = 0;
	for (int i = 0; i < sizeof gPinMapping / sizeof *gPinMapping; ++i)
	{
		if (gPinMapping[i] >= 0 && tempPacking & (1 << gPinMapping[i]))
			realPacking |= 1 << i;
	}

	return realPacking;
}


static void writeByte(uint16_t address, uint8_t data)
{
	uint32_t bits;
	#if VERIFY_ROM
	bits = packGpioBits(address, data, false, true, false);
	#else
	bits = packGpioBits(address, data, false);
	#endif

	gpio_put_masked(gAllBits, bits | (1 << gChipEnablePin));
	busy_wait_us(gConfig.pulseDelayUs);
	gpio_put_masked(gAllBits, bits);
	busy_wait_us(gConfig.pulseDelayUs);
	gpio_put_masked(gAllBits, bits | (1 << gChipEnablePin));
	busy_wait_us(gConfig.byteDelayUs);

	#if VERIFY_ROM
	gpio_put_masked(gAllBits, bits | (1 << gChipEnablePin) | (1 << gWriteEnablePin));
	busy_wait_us(gConfig.pulseDelayUs);
	#endif
}

typedef uint8_t (*image_func)(size_t address);

void writeImage(image_func cb, size_t size) {
	if (gConfig.writeProtectDisable)
	{
		writeByte(0x5555, 0xaa);
		writeByte(0x2aaa, 0x55);
		writeByte(0x5555, 0x80);
		writeByte(0x5555, 0xaa);
		writeByte(0x2aaa, 0x55);
		writeByte(0x5555, 0x20);
		sleep_ms(gConfig.pageDelayMs);
	}

	if (size > gConfig.size) size = gConfig.size;

	int prevAddress = 0xffffff;
	for (int address = 0; address < size; ++address)
	{
		if (gConfig.pageSize && prevAddress / gConfig.pageSize != address / gConfig.pageSize)
		{
			/* Page change - wait 10ms (worst case) for write to complete */
			sleep_ms(gConfig.pageDelayMs);

			if (gConfig.writeProtect)
			{
				/* Locking prefix */
				writeByte(0x5555, 0xaa);
				writeByte(0x2aaa, 0x55);
				writeByte(0x5555, 0xa0);
			}
		}

		prevAddress = address;
		
		writeByte(address, cb(address));

		gpio_put(gLedPin, (address & 0x100) != 0);

		if (((address + 1) & 0x7ff) == 0)
		{
			printf(" %dK", (address + 1) >> 10);
		}
	}
}

const static uint8_t * imgBuffer;
static size_t imgSize;
uint8_t retImage(size_t address) {
	return imgBuffer[address];
}
void eeprom_writeImage(const uint8_t* buffer, size_t size) {
	imgBuffer = buffer;
	imgSize = size;
	writeImage(retImage, size);
}

static uint8_t writeVal = 0;
uint8_t retValue(size_t address) {
	return writeVal;
}
void eeprom_writeValue(uint8_t value) {
	writeVal = value;
	writeImage(retValue, gConfig.size);
}

uint8_t retRandom(size_t address) {
	return (uint8_t)(get_rand_32() & 0xff);
}
void eeprom_writeRandom() {
	writeImage(retRandom, gConfig.size);
}

uint8_t retIndex(size_t address) {
	return (uint8_t)(address & 0xff);
}
void eeprom_writeIndex() {
	writeImage(retIndex, gConfig.size);
}

#if VERIFY_ROM

static uint8_t readByte(uint16_t address) {
	uint32_t bits = packGpioBits(address, 0x00, false, false, true);
	uint32_t dataBits;
	uint8_t data = 0;

	// Set data bits as input
	gpio_set_dir_in_masked(gDataBits);
	
	// Prepare address bits
	gpio_put_masked(gControlBits, bits | (1 << gChipEnablePin) | (1 << gOutputEnablePin));
	busy_wait_us(gConfig.pulseDelayUs);

	// Active-low Chip Enable and Output Enable
	gpio_put_masked(gControlBits, bits);
	busy_wait_us(gConfig.pulseDelayUs);

	// Read data bus
	dataBits = gpio_get_all() & gDataBits;
	for (int i = 0; i < 8; i++) {
		if (dataBits & ((uint32_t)1 << gDataPins[i])) {
			data |= (1 << i);
		}
	}
	// NOTE: Could shift data bits into first 8 bits if always sequential.

	// Disable output
	gpio_put_masked(gControlBits, bits | (1 << gChipEnablePin) | (1 << gOutputEnablePin));
	busy_wait_us(gConfig.pulseDelayUs);

	// Return data bits to output operation
	gpio_set_dir_out_masked(gAllBits);

	return data;
}

size_t eeprom_verifyImage(const uint8_t* buffer, size_t size, size_t offset) {
	if (size > gConfig.size) size = gConfig.size;
	if (offset + size > gConfig.size) offset = gConfig.size - size;

	size_t error = 0;
	for (size_t address = offset; address < offset + size; ++address)
	{
		if (readByte(address) != (uint8_t)buffer[address-offset]) error++;

		gpio_put(gLedPin, (address & 0x100) != 0);

		if (((address + 1) & 0x7ff) == 0)
		{
			printf(" %dK", (address + 1) >> 10);
		}
	}
	return error;
}

size_t eeprom_readImage(uint8_t* buffer, size_t size, size_t offset) {
	if (size > gConfig.size) size = gConfig.size;
	if (offset + size > gConfig.size) offset = gConfig.size - size;

	size_t error = 0;
	for (size_t address = offset; address < offset + size; ++address)
	{
		buffer[address-offset] = readByte(address);

		gpio_put(gLedPin, (address & 0x100) != 0);

		if (((address + 1) & 0x7ff) == 0)
		{
			printf(" %dK", (address + 1) >> 10);
		}
	}

	return size;
}

#endif

bool eeprom_init()
{
	#if VERIFY_ROM
	gAllBits = packGpioBits(0xffff, 0xff, true, true, true);
	gDataBits = packGpioBits(0x0000, 0xff, false, false, false);
	gControlBits = packGpioBits(0xffff, 0x00, true, true, true);
	#else
	gAllBits = packGpioBits(0xffff, 0xff, true);
	#endif

	gpio_init(gLedPin);
	gpio_set_dir(gLedPin, GPIO_OUT);
	gpio_put(gLedPin, 1);

	gpio_init_mask(gAllBits);
	gpio_set_dir_out_masked(gAllBits);
	gpio_put_masked(gAllBits, gAllBits);

	gChipEnablePin = -1;
	#if VERIFY_ROM
	gOutputEnablePin = -1;
	gWriteEnablePin = -1;
	for (int i = 0; i < 8; i++) {
		gDataPins[i] = -1;
	}
	#endif
	for (int i = 0; i < sizeof gPinMapping / sizeof *gPinMapping; ++i)
	{
		switch (gPinMapping[i]) {
			case 15:
				gChipEnablePin = i;
				break;
			#if VERIFY_ROM
			case 24:
				gOutputEnablePin = i;
				break;
			case 25:
				gWriteEnablePin = i;
				break;
			#endif
		}
		#if VERIFY_ROM
		if (gPinMapping[i] >= 16 && gPinMapping[i] < 24) {
			gDataPins[gPinMapping[i]-16] = i;
		}
		#endif
	}

	#if VERIFY_ROM
	for (int i = 0; i < 8; i++) {
		if (gDataPins[i] < 0) return false;
	}
	return gChipEnablePin >= 0 && gOutputEnablePin >= 0 && gWriteEnablePin >= 0;
	#else
	return gChipEnablePin >= 0;
	#endif
}
