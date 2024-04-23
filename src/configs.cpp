#include "configs.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

picoprom_config_t gConfig;

static picoprom_config_t gConfigs[] = {
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

void init_device() {
	gConfigIndex = 0;
	memcpy(&gConfig, &gConfigs[gConfigIndex], sizeof gConfig);
}

void print_device() {
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
}

void change_device() {
	++gConfigIndex;
	if (!gConfigs[gConfigIndex].name)
		gConfigIndex = 0;

	memcpy(&gConfig, &gConfigs[gConfigIndex], sizeof gConfig);

	printf("\r\n\r\nChanged device to %s\r\n", gConfig.name);
}
