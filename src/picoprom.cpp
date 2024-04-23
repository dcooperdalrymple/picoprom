#include "picoprom.hpp"

#include <stdio.h>
#include <tusb.h>

#include "pico/binary_info.h"
#include "pico/stdlib.h"

#include "xmodem.hpp"

#include "configs.hpp"
#include "eeprom.hpp"
#include "command.hpp"

static uint8_t buffer[MAXSIZE];
static size_t image_size;
static XMODEM xmodem;
static Command * command;

// Image Actions

static Command transfer_options[] = {
	{ 'x', "XMODEM" },
	{ 's', "Flash Storage" },
	{ 0 }
};

static bool receive_image() {
	command = command_prompt(transfer_options, "Select how you would like to transfer the image", true);
	if (!command) return false;

	image_size = 0;
	switch (command->key) {
		case 'x':
			printf("Ready to receive image. Begin XMODEM transfer... ");
			image_size = xmodem.receive(buffer, MAXSIZE);
			if (!image_size) printf("\r\nXMODEM transfer failed\r\n");
			break;
		case 's':
			printf("Not yet implemented.\r\n");
			break;
	}
	printf("\r\n");
	return !!image_size;
}

static bool send_image() {
	if (!image_size) return false;
	command = command_prompt(transfer_options, "Select how you would like to receive the ROM image", true);
	if (!command) return false;
	bool result = false;
	switch (command->key) {
		case 'x':
			printf("Ready to send ROM image. Begin XMODEM transfer... ");
			if (xmodem.send(buffer, image_size)) {
				result = true;
				printf("\r\nSend transfer complete - delivered %d bytes\r\n", image_size);
			} else {
				printf("\r\nXMODEM send transfer failed\r\n");
			}
			break;
		case 's':
			printf("Not yet implemented.\r\n");
			break;
	}
	printf("\r\n");
	return result;
}

static bool verify_buffer() {
	if (!image_size) return false;
	printf("Verifying ROM contents... ");
	sleep_ms(gConfig.pageDelayMs);
	size_t error = eeprom_verifyImage(buffer, image_size, 0);
	printf("\r\n");
	if (error > 0) {
		printf("ROM verification failed: %d incorrect bytes out of %d\r\n", error, image_size);
	} else {
		printf("ROM verification succeeded\r\n");
	}
	printf("\r\n");
	return !error;
}

static void write_image() {
	if (!receive_image()) return;

	printf("\r\n");
	printf("Transfer complete - received %d bytes\r\n", image_size);
	printf("\r\n");

	if (image_size > gConfig.size) {
		printf("Truncating image to %d bytes\r\n", gConfig.size);
		printf("\r\n");
		image_size = gConfig.size;
	}

	printf("Writing to EEPROM... ");
	eeprom_writeImage(buffer, image_size);
	printf("\r\n");

	verify_buffer();
}

static void read_image() {
	printf("Reading EEPROM contents...");
	image_size = eeprom_readImage(buffer, MAXSIZE, 0);
	printf("\r\n\r\n");
	send_image();
}

static void read_page() {
	int i, j, k, mul, page = 0, len = 0;
	printf("Enter the number of the page you would like to view then hit enter: ");

	// Get number value
	do {
		buffer[len] = getchar();
		putchar(buffer[len]);
		if (buffer[len] == 13) break; // Carriage return
	} while (len++ < 5);

	// Convert ascii characters to number
	for (i = len-1; i >= 0; i--) {
		j = buffer[i] - 48;
		if (j > 9 || j < 0) break;
		mul = 1;
		for (k = 0; k < len-i-1; k++) {
			mul *= 10;
		}
		page += j * mul;
	}

	int pageSize = gConfig.pageSize;
	if (pageSize <= 0) pageSize = 64;

	printf("\r\nReading EEPROM page %d contents...", page);
	image_size = eeprom_readImage(buffer, pageSize, page * pageSize);
	if (image_size) {
		printf("\r\nSuccessfully read %d bytes.\r\n", image_size);
		for (int i = 0; i < image_size; i++) {
			if (i % 16 == 0) printf("\r\n");
			printf("%02X ", buffer[i]);
		}
	} else {
		printf("\r\nUnable to read page.");
	}
	printf("\r\n\r\n");
}

static void verify_image() {
	if (!receive_image()) return;
	verify_buffer();
}

// Tools

static void erase() {
	printf("Not yet implemented.\r\n");
}

static void write_zeroes() {
	printf("\r\n");
	printf("Writing zeroes to EEPROM... ");
	eeprom_writeValue(0x00);
	printf("\r\n");

	// TODO: Implement verification

	printf("\r\n");
}

static void write_ones() {
	printf("\r\n");
	printf("Writing zeroes to EEPROM... ");
	eeprom_writeValue(0xff);
	printf("\r\n");

	// TODO: Implement verification

	printf("\r\n");
}

static void write_random() {
	printf("\r\n");
	printf("Writing random values to EEPROM... ");
	eeprom_writeRandom();
	printf("\r\n");
	printf("\r\n");
}

static void write_index() {
	printf("\r\n");
	printf("Writing address index values to EEPROM... ");
	eeprom_writeIndex();
	printf("\r\n");

	// TODO: Implement verification

	printf("\r\n");
}

static Command tools_commands[] = {
	{ 'e', "Erase", erase },
	{ '0', "write all 0 values", write_zeroes },
	{ '1', "write all 1 values", write_ones },
	{ '2', "write random values", write_random },
	{ '3', "write address index", write_index },
	{ 0 }
};

static void tools_menu() {
	while (true) {
		command = command_prompt(tools_commands, "Select the action you would like to take", true);
		if (!command) break;
		if (command->action) command->action();
	}
}

// Settings

static Command log_level_items[XLOGLEVEL_COUNT];

static void change_log_level() {
	if (!log_level_items[0].key) {
		for (uint8_t i = 0; i < XLOGLEVEL_COUNT; i++) {
			log_level_items[i].key = (char)(0x31+i);
			log_level_items[i].name = XLogLevelNames[i];
		}
	}

	command = command_prompt(log_level_items, "Select your desired log level", true);
	if (!command) return;

	xmodem.set_log_level(static_cast<XLogLevel>(command->key-0x31));
}

static Command settings_commands[] = {
	{ 'd', "Change device", change_device },
	//{ 'c', "Change category", change_device_category },
	{ 'l', "Change log level", change_log_level },
	{ 0 }
};

static void show_settings() {
	print_device();
	xmodem.print_config();
}

static void settings_menu() {
	while (true) {
		show_settings();
		command = command_prompt(settings_commands, "Select the setting you would like to change", true);
		if (!command) break;
		if (command->action) command->action();
	}
}

// Main Menu

static Command menu_commands[] = {
	{ 'w', "Write image", write_image },
	{ 'r', "Read image", read_image },
	{ 'p', "Read page", read_page },
	{ 'v', "Verify image", verify_image },
	{ 't', "Tools", tools_menu },
	{ 's', "Settings", settings_menu },
	{ 0 }
};

int main() {
	bi_decl(bi_program_description("PicoPROM - EEPROM programming tool"));

	bool eepromOk = eeprom_init();

	init_device();

	while (true) {

		while (!tud_cdc_connected()) sleep_ms(100);
		printf("\r\nUSB Serial connected\r\n");

		if (!eepromOk) {
			printf("ERROR: no pin was mapped to Write Enable (ID 15)\r\n");
			return -1;
		}

		// Print banner
		printf("\r\n\r\n");
		printf("PicoPROM v0.23   Raspberry Pi Pico DIP-EEPROM programmer\r\n");
		printf("                 by George Foot, February 2021 & Cooper Dalrymple, April 2024\r\n");
		printf("                 https://github.com/dcooperdalrymple/picoprom\r\n");

		// Print current settings
		printf("\r\n\r\n");
		show_settings();

		while (true) {
			command = command_prompt(menu_commands, "Main Menu");
			if (command && command->action) command->action();
		}

	}

	return 0;
}
