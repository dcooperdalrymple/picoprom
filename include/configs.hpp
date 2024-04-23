#pragma once

#include <stdbool.h>

typedef struct {
	const char *name;
	int size;
	int pageSize;
	int pageDelayMs;
	int pulseDelayUs;
	int byteDelayUs;
	bool writeProtect;
	bool writeProtectDisable;
	bool invertClock;
	bool readonly;
} picoprom_config_t;

extern picoprom_config_t gConfig;

void init_device();
void print_device();
void change_device();
