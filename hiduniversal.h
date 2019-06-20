#pragma once

#include "USBDevice.h"
#include "stm32f1xx_hal.h"
#include "usbcore.h"
#include "MAX3421E.h"

#define constBuffLen 64  // event buffer length

struct hiduniversal {
	USBDevice*(*configure)(void);			
};

extern const struct hiduniversal HIDUniversal;