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

	while (!tud_cdc_connected()) sleep_ms(100);
	printf("\nUSB Serial connected\n");


	if (!eepromOk)
	{
		printf("ERROR: no pin was mapped to Write Enable (ID 15)\n");
		return false;
	}


	init_settings();

	return true;
}


static void banner()
{
	printf("\n\n");
	printf("\n\n");
	printf("\n\n");

	printf("PicoPROM v0.22   Raspberry Pi Pico DIP-EEPROM programmer\n");
	printf("                 by George Foot, February 2021 & Cooper Dalrymple, March 2024\n");
	printf("                 https://github.com/dcooperdalrymple/picoprom\n");
}

static void read_image()
{
	printf("\nReading EEPROM contents...");
	int sizeReceived = eeprom_readImage(buffer, sizeof buffer, 0);
	printf("\nReady to send ROM image...\n");
	if (xmodem_send(buffer, sizeReceived))
	{
		printf("\nSend transfer complete - delivered %d bytes\n", sizeReceived);
	}
	else
	{
		printf("\nXMODEM send transfer failed\n");
	}
	printf("\n");
	xmodem_dumplog();
	printf("\n");
}

static void read_page()
{
	int i, j, k, mul, page = 0, len = 0;
	printf("\nEnter the number of the page you would like to view then hit enter: ");

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

	printf("\nReading EEPROM page %d contents...", page);
	int sizeReceived = eeprom_readImage(buffer, gConfig.pageSize, page * gConfig.pageSize);
	for (int i = 0; i < sizeReceived; i++) {
		if (i % 16 == 0) printf("\n");
		printf("%02X ", buffer[i]);
	}
	printf("\n");
}

static command_t mCommands[] =
{
	{ 'r', "receive eeprom image", read_image },
	{ 'p', "read eeprom page", read_page },
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
	printf("\n\n");
	printf("\n\n");
	printf("\n\n");

	banner();

	printf("\n\n");
	
	show_settings();
	
	printf("\n\n");
	printf("Ready to program - send data now\n");
	printf("Other options:\n");
	for (int i = 0; mCommands[i].key; ++i)
	{
		if (mCommands[i].key == 13)
		{
			printf("    enter = %s\n", mCommands[i].commandName);
		}
		else
		{
			printf("    %c = %s\n", mCommands[i].key, mCommands[i].commandName);
		}
	}
	printf("\n");

	int sizeReceived = xmodem_receive(buffer, sizeof buffer, NULL, input_handler);

	if (sizeReceived < 0)
	{
		xmodem_dumplog();
		printf("XMODEM transfer failed\n");
	}

	if (sizeReceived <= 0)
	{
		return;
	}

	printf("\n");
	printf("Transfer complete - received %d bytes\n", sizeReceived);
	printf("\n");

	if (sizeReceived > gConfig.size)
	{
		printf("Truncating image to %d bytes\n", gConfig.size);
		printf("\n");
		sizeReceived = gConfig.size;
	}

	printf("Writing to EEPROM...\n");
	eeprom_writeImage(buffer, sizeReceived);
	printf("\n");

	#if VERIFY_ROM
	printf("Verifying EEPROM contents...\n");
	size_t error = eeprom_verifyImage(buffer, sizeReceived);
	printf("\n");
	if (error > 0) {
		printf("ROM verification failed: %d incorrect bytes out of %d\n", error, sizeReceived);
	} else {
		printf("ROM verification succeeded\n");
	}
	printf("\n");
	#endif

	printf("Done\n");
}


int main()
{
	if (!setup())
		return -1;

	while (true)
		loop();
	
	return 0;
}
