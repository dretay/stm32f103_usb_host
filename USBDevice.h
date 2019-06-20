#pragma once

#include "usb_ch9.h"
#include "types_shortcuts.h"
#include "stdbool.h"
#include "usbcore.h"

struct USBDevice{
	u8(*poll)(void);	
	void(*parse)(void);		
	u8 max_packet_size;
	u8 address;
	USB_DEVICE_DESCRIPTOR device_descriptor;
	USB_CONFIGURATION_DESCRIPTOR config_descriptor;
	USB_ENDPOINT_DESCRIPTOR endpoint_descriptor;
	bool poll_enabled;
	u32 next_poll_time;	
};