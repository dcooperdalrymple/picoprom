#include "picoprom.hpp"

#include <stdio.h>
#include <tusb.h>
#include <typeinfo>
#include <cstdlib>

#include "pico/binary_info.h"
#include "pico/stdlib.h"

#include "xmodem.hpp"
#include <lfs.h>

#include "config.hpp"
#include "rom.hpp"
#include "storage.hpp"
#include "command.hpp"

static uint8_t buffer[MAXSIZE];
static char input_buffer[LFS_NAME_MAX+1];
static size_t image_size;
static XMODEM xmodem;
static ROM * rom = NULL;
static Command * command;
static char * selected_file;

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
			// TODO: quit during timeout?
			printf("Ready to receive image. Begin XMODEM transfer... ");
			if (image_size = xmodem.receive(buffer, MAXSIZE)) {
				printf("\r\nTransfer complete!\r\n");
			} else {
				printf("\r\nXMODEM transfer failed\r\n");
			}
			break;
		case 's':
			if ((selected_file = get_file_selection()) != NULL) {
				printf("Reading \"%s\"...\r\n", selected_file);
				if (!(image_size = read_file(selected_file, buffer, MAXSIZE))) {
					printf("Failed to read data from \"%s\".\r\n", selected_file);
				}
			}
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
			uint i, j, len = 0;
			do {
				printf("Enter a valid filename (%d characters max, leave empty to quit): ", LFS_NAME_MAX);
				do {
					input_buffer[len] = getchar();
					if (input_buffer[len] == 13) break; // Carriage return
					putchar(input_buffer[len]);
				} while (len++ < LFS_NAME_MAX);
				printf("\r\n");
				if (len == 0) break;
				input_buffer[len] = 0;
				if (!valid_filename(input_buffer)) len = 0;
				if (file_exists(input_buffer)) {
					printf("File already exists.\r\n");
					len = 0;
				}
			} while (!len);
			if (len) {
				printf("\r\nWriting data to \"%s\"...\r\n", input_buffer);
				if (write_file(input_buffer, buffer, image_size)) {
					printf("Successfully written data to file.\r\n", input_buffer);
					result = true;
				} else {
					printf("Failed to write to flash storage.\r\n");
				}
			}
			break;
	}
	printf("\r\n");
	return result;
}

static bool verify_buffer() {
	if (!image_size) return false;
	printf("Verifying ROM contents... ");
	size_t error = rom->verify_image(buffer, image_size);
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

	printf("Preparing image with a size of %d bytes\r\n", image_size);
	if (image_size > rom->get_size()) {
		printf("Truncating image to %d bytes\r\n", rom->get_size());
		image_size = rom->get_size();
	}
	printf("\r\n");

	printf("Writing to device... ");
	rom->write_image(buffer, image_size);
	printf("\r\n");

	verify_buffer();
}

static void read_image() {
	printf("Reading device contents... ");
	if (rom->read(buffer)) {
		image_size = rom->get_size();
	} else {
		printf("\r\nFailed to read image.");
	}
	printf("\r\n\r\n");
	send_image();
}

static void read_page() {
	// TODO: optimize data types
	int i, j, k, mul, page = 0, len = 0;

	image_size = rom->get_page_size();
	if (image_size <= 0) image_size = 64;

	printf("Provide the number of the page (%d bytes) you would like to view then hit enter (0-%d): ", image_size, rom->get_size() / image_size - 1);

	// Get number value
	do {
		input_buffer[len] = getchar();
		putchar(input_buffer[len]);
		if (input_buffer[len] == 13) break; // Carriage return
	} while (len++ < 5);

	// Convert ascii characters to number
	for (i = len-1; i >= 0; i--) {
		j = input_buffer[i] - 48;
		if (j > 9 || j < 0) break;
		mul = 1;
		for (k = 0; k < len-i-1; k++) {
			mul *= 10;
		}
		page += j * mul;
	}

	printf("\r\nReading page %d contents... ", page);
	if (rom->read(buffer, image_size, page * image_size)) {
		printf("\r\nSuccessfully read %d bytes.\r\n", image_size);
		for (int i = 0; i < image_size; i++) {
			if (i % 16 == 0) printf("\r\n");
			printf("%02X ", buffer[i]);
		}
	} else {
		printf("\r\nFailed to read page.");
	}
	printf("\r\n\r\n");
}

static void verify_image() {
	if (!receive_image()) return;
	verify_buffer();
}

// Tools

static void erase() {
	printf("Not yet implemented.\r\n\r\n");
}

static void write_zeroes() {
	printf("Writing zeroes to device... ");
	if (!rom->write_value(0x00)) {
		printf("\r\nFailed to write to device.\r\n\r\n");
		return;
	}
	printf("\r\n");

	printf("Verifying ROM contents... ");
	size_t error = rom->verify_value(0x00);
	printf("\r\n");
	if (error > 0) {
		printf("ROM verification failed: %d incorrect bytes out of %d\r\n", error, rom->get_size());
	} else {
		printf("ROM verification succeeded\r\n");
	}
	printf("\r\n");
}

static void write_ones() {
	printf("Writing zeroes to device... ");
	if (!rom->write_value(0xFF)) {
		printf("\r\nFailed to write to device.\r\n\r\n");
		return;
	}
	printf("\r\n");

	printf("Verifying ROM contents... ");
	size_t error = rom->verify_value(0xFF);
	printf("\r\n");
	if (error > 0) {
		printf("ROM verification failed: %d incorrect bytes out of %d\r\n", error, rom->get_size());
	} else {
		printf("ROM verification succeeded\r\n");
	}
	printf("\r\n");
}

static void write_random() {
	printf("Writing random values to device... ");
	if (!rom->write_random()) printf("\r\nFailed to write to device.");
	printf("\r\n\r\n");
}

static void write_index() {
	printf("Writing address index values to device... ");
	if (!rom->write_index()) {
		printf("\r\nFailed to write to device.\r\n\r\n");
		return;
	}
	printf("\r\n");

	printf("Verifying ROM contents... ");
	size_t error = rom->verify_index();
	printf("\r\n");
	if (error > 0) {
		printf("ROM verification failed: %d incorrect bytes out of %d\r\n", error, rom->get_size());
	} else {
		printf("ROM verification succeeded\r\n");
	}
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
	{ 'd', "Change device", next_config },
	{ 'c', "Change category", next_config_category },
	{ 'l', "Change log level", change_log_level },
	{ 0 }
};

static void show_settings() {
	print_config();
	printf("\r\n");
	xmodem.print_config();
}

static void init_rom() {
	if (rom) delete rom;
	rom = new ROM(get_config());
}

static void settings_menu() {
	while (true) {
		show_settings();
		command = command_prompt(settings_commands, "Select the setting you would like to change", true);
		if (!command) break;
		if (command->action) command->action();
		if (command->key != 'l') init_rom();
	}
}

// Filesystem

static void filesystem_delete() {
	if ((selected_file = get_file_selection("Select the file you would like to delete")) != NULL) {
		printf("Deleting \"%s\"...\r\n", selected_file);
		if (delete_file(selected_file)) {
			printf("Successfully deleted file.\r\n");
		} else {
			printf("Unable to delete file.\r\n");
		}
	}
	printf("\r\n");
}

static void filesystem_reformat() {
	printf("Reformatting flash storage...\r\n");
	if (!reformat_filesystem()) {
		printf("Unable to perform operation.\r\n");
	} else {
		printf("Operation successfully completed!\r\n");
	}
	printf("\r\n");
};

static Command filesystem_commands[] = {
	{ 'd', "Delete file", filesystem_delete },
	{ 'f', "Reformat file system", filesystem_reformat },
	// TODO: Rename
	{ 0 }
};

static void filesystem_menu() {
	while (true) {
		if (dir_count()) {
			printf("Available Files:\r\n");
			print_dir_items();
			printf("\r\n");
		}
		command = command_prompt(filesystem_commands, "Select the file operation you would like to perform", true);
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
	{ 'f', "Manage files", filesystem_menu },
	{ 0 }
};

int main() {
	bi_decl(bi_program_description("PicoPROM - ROM programming tool"));

	init_rom();
	init_filesystem();

	while (true) {

		while (!tud_cdc_connected()) sleep_ms(100);
		printf("\r\n\r\nUSB Serial connected\r\n\r\n");

		// Print banner
		printf("PicoPROM v0.23   Raspberry Pi Pico ROM programmer\r\n");
		printf("                 by George Foot, February 2021 & Cooper Dalrymple, April 2024\r\n");
		printf("                 https://github.com/dcooperdalrymple/picoprom\r\n");

		// Print current settings
		printf("\r\n");
		show_settings();

		while (true) {
			command = command_prompt(menu_commands, "Main Menu");
			if (command && command->action) command->action();
		}

	}

	return 0;
}
