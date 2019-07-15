#pragma once

#include <stdio.h>

#include "log.h"
#include "USBCore.h"

struct application {
	void(*init)(void);			
	void(*process)(void);			
};

extern const struct application Application;