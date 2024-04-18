#include "picoprom.h"

#include <stdio.h>
#include <tusb.h>

#include "pico/binary_info.h"
#include "pico/stdlib.h"

#include "configs.h"
#include "xmodem.h"
#include "eeprom.h"

static char buffer[65536];

static bool setup()
{
	bi_decl(bi_program_description("PicoPROM - EEPROM programming tool"));

	bool eepromOk = eeprom_init();

	stdio_init_all();
	stdio_set_translate_crlf(&stdio_usb, false); // Disable automatic carriage return

	while (!tud_cdc_connected()) sleep_ms(100);
	printf("\r\nUSB Serial connected\r\n");


	if (!eepromOk)
	{
		printf("ERROR: no pin was mapped to Write Enable (ID 15)\r\n");
		return false;
	}


	init_settings();

	return true;
}


static void banner()
{
	printf("\r\n\r\n");
	printf("\r\n\r\n");
	printf("\r\n\r\n");

	printf("PicoPROM v0.22   Raspberry Pi Pico DIP-EEPROM programmer\r\n");
	printf("                 by George Foot, February 2021 & Cooper Dalrymple, March 2024\r\n");
	printf("                 https://github.com/dcooperdalrymple/picoprom\r\n");
}

#if VERIFY_ROM
static void read_image()
{
	printf("\r\nReading EEPROM contents...");
	int sizeReceived = eeprom_readImage(buffer, sizeof buffer, 0);
	printf("\r\nReady to send ROM image...\r\n");
	if (xmodem_send(buffer, sizeReceived))
	{
		printf("\r\nSend transfer complete - delivered %d bytes\r\n", sizeReceived);
	}
	else
	{
		printf("\r\nXMODEM send transfer failed\r\n");
	}
	printf("\r\n");
	xmodem_dumplog();
	printf("\r\n");
}

static void read_page()
{
	int i, j, k, mul, page = 0, len = 0;
	printf("\r\nEnter the number of the page you would like to view then hit enter: ");

	// Get number value
	do
	{
		buffer[len] = getchar();
		putchar(buffer[len]);
		if (buffer[len] == 13) break; // Carriage return
	}
	while (len++ < 5);

	// Convert ascii characters to number
	for (i = len-1; i >= 0; i--)
	{
		j = buffer[i] - 48;
		if (j > 9 || j < 0) break;
		mul = 1;
		for (k = 0; k < len-i-1; k++)
		{
			mul *= 10;
		}
		page += j * mul;
	}

	printf("\r\nReading EEPROM page %d contents...", page);
	int sizeReceived = eeprom_readImage(buffer, gConfig.pageSize, page * gConfig.pageSize);
	for (int i = 0; i < sizeReceived; i++) {
		if (i % 16 == 0) printf("\r\n");
		printf("%02X ", buffer[i]);
	}
	printf("\r\n");
}

static void verify_image() {
	// TODO: receive image over xmodem and verify with rom
}
#endif

static void write_zeroes() {
	printf("\r\n");
	printf("Writing zeroes to EEPROM...\r\n");
	eeprom_writeValue(0x00);
	printf("\r\n");

	// TODO: Implement verification

	printf("Done\r\n");
}

static void write_ones() {
	printf("\r\n");
	printf("Writing zeroes to EEPROM...\r\n");
	eeprom_writeValue(0xff);
	printf("\r\n");

	// TODO: Implement verification

	printf("Done\r\n");
}

static void write_random() {
	printf("\r\n");
	printf("Writing random values to EEPROM...\r\n");
	eeprom_writeRandom();
	printf("\r\n");

	// TODO: Implement verification

	printf("Done\r\n");
}

static void write_index() {
	printf("\r\n");
	printf("Writing address index values to EEPROM...\r\n");
	eeprom_writeIndex();
	printf("\r\n");

	// TODO: Implement verification

	printf("Done\r\n");
}

static command_t mCommands[] =
{
	#if VERIFY_ROM
	{ 'r', "receive eeprom image", read_image },
	{ 'p', "read eeprom page", read_page },
	{ 'v', "verify eeprom image", verify_image },
	#endif
	{ '0', "write all 0 values to eeprom", write_zeroes },
	{ '1', "write all 1 values to eeprom", write_ones },
	{ '2', "write random values to eeprom", write_random },
	{ '3', "write address index to eeprom", write_index },
	{ 13, "change settings", change_settings },
	{ 0 }
};

static bool input_handler(int c)
{
	for (int i = 0; mCommands[i].key; ++i)
	{
		if (c == mCommands[i].key && mCommands[i].action)
		{
			mCommands[i].action();
		return true;
		}
	}
	return false;
}


void loop()
{
	printf("\r\n\r\n");
	printf("\r\n\r\n");
	printf("\r\n\r\n");

	banner();

	printf("\r\n\r\n");
	
	show_settings();
	
	printf("\r\n\r\n");
	printf("Ready to program - send data now\r\n");
	printf("Other options:\r\n");
	for (int i = 0; mCommands[i].key; ++i)
	{
		if (mCommands[i].key == 13)
		{
			printf("    enter = %s\r\n", mCommands[i].commandName);
		}
		else
		{
			printf("    %c = %s\r\n", mCommands[i].key, mCommands[i].commandName);
		}
	}
	printf("\r\n");

	int sizeReceived = xmodem_receive(buffer, sizeof buffer, NULL, input_handler);

	if (sizeReceived < 0)
	{
		xmodem_dumplog();
		printf("XMODEM transfer failed\r\n");
	}

	if (sizeReceived <= 0)
	{
		return;
	}

	printf("\r\n");
	printf("Transfer complete - received %d bytes\r\n", sizeReceived);
	printf("\r\n");

	if (sizeReceived > gConfig.size)
	{
		printf("Truncating image to %d bytes\r\n", gConfig.size);
		printf("\r\n");
		sizeReceived = gConfig.size;
	}

	printf("Writing to EEPROM...\r\n");
	eeprom_writeImage(buffer, sizeReceived);
	printf("\r\n");

	#if VERIFY_ROM
	printf("Verifying EEPROM contents...\r\n");
	sleep_ms(gConfig.pageDelayMs);
	size_t error = eeprom_verifyImage(buffer, sizeReceived, 0);
	printf("\r\n");
	if (error > 0) {
		printf("ROM verification failed: %d incorrect bytes out of %d\r\n", error, sizeReceived);
	} else {
		printf("ROM verification succeeded\r\n");
	}
	printf("\r\n");
	#endif

	printf("Done\r\n");
}


int main()
{
	if (!setup())
		return -1;

	while (true)
		loop();
	
	return 0;
}
