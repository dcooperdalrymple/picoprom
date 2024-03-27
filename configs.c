#include "configs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xmodem.h"


picoprom_config_t gConfig;


static picoprom_config_t gConfigs[] =
{
	{
		"AT28C256",
		32768,
		64, 10, 1, 0,
		true, false
	},
	{
		"AT28C256F",
		32768,
		64, 3, 1, 0,
		true, false
	},
	{
		"AT28C64",
		8192,
		0, 10, 1, 1000,
		false, false
	},
	{
		"AT28C64B",
		8192,
		64, 10, 1, 0,
		true, false
	},
	{
		"AT28C64E",
		8192,
		0, 10, 1, 200,
		false, false
	},
	{
		"AT28C16",
		2048,
		0, 10, 1, 1000,
		false, false
	},
	{
		"AT28C16E",
		2048,
		0, 10, 1, 200,
		false, false
	},
	{
		"M28C16",
		2048,
		64, 3, 1, 0,
		true, false
	},
	{
		NULL
	}
};


static int gConfigIndex = 0;


void init_settings()
{
	gConfigIndex = 0;
	memcpy(&gConfig, &gConfigs[gConfigIndex], sizeof gConfig);

	xmodem_config.logLevel = 1;
}


void show_settings()
{
	printf("EEPROM Device: %s\r\n", gConfig.name);
	printf("        Capacity: %dK bytes\r\n", gConfig.size / 1024);
	printf("        Page mode: %s\r\n", gConfig.pageSize ? "on" : "off");
	if (gConfig.pageSize)
	{
		printf("        Page size: %d bytes\r\n", gConfig.pageSize);
		printf("        Page delay: %dms\r\n", gConfig.pageDelayMs);
	}
	printf("        Pulse delay: %dus\r\n", gConfig.pulseDelayUs);
	printf("        Byte delay: %dus\r\n", gConfig.byteDelayUs);
	printf("        Write protect: %s\r\n", gConfig.writeProtect ? "enable" : gConfig.writeProtectDisable ? "disable" : "no action / not supported");
	printf("\r\n");
	printf("Serial protocol: XMODEM+CRC\r\n");
	printf("        Block size: 128 bytes\r\n");
	printf("        CRC: on\r\n");
	printf("        Escaping: off\r\n");
	printf("        Log level: %d\r\n", xmodem_config.logLevel);
}


static void change_device()
{
	++gConfigIndex;
	if (!gConfigs[gConfigIndex].name)
		gConfigIndex = 0;

	memcpy(&gConfig, &gConfigs[gConfigIndex], sizeof gConfig);

	printf("\r\n\r\nChanged device to %s\r\n", gConfig.name);
}

static void change_log_level()
{
	xmodem_config.logLevel = (xmodem_config.logLevel + 1) % 4;

	printf("\r\n\r\nChanged log level to %d\r\n", xmodem_config.logLevel);
}


static command_t gCommands[] =
{
	{ 'd', "change device", change_device },
	{ 'l', "change log level", change_log_level },
	{ 'p', "return to programming mode", NULL },
	{ 0 }
};


void change_settings()
{
	while (true)
	{
		printf("\r\n\r\n");
		printf("\r\n\r\n");
		printf("\r\n\r\n");

		show_settings();
		
		printf("\r\n\r\n");
		
		printf("Changing settings:\r\n");
		printf("\r\n");
		
		for (int i = 0; gCommands[i].key; ++i)
		{
			printf("    %c = %s\r\n", gCommands[i].key, gCommands[i].commandName);
		}
		printf("\r\n");
		printf("?");

		int c = getchar();
		for (int i = 0; gCommands[i].key; ++i)
		{
			if (c == gCommands[i].key)
			{
				if (!gCommands[i].action)
					return;

				gCommands[i].action();
			}
		}
	}
}
