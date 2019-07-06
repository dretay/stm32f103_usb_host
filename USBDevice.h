#pragma once

#include "usb_ch9.h"
#include "types_shortcuts.h"
#include "stdbool.h"
#include "usbcore.h"

struct USBDevice{
	u8(*poll)(void);	
	void(*parse)(void);			
	u8 address;
	USB_CONFIGURATION_DESCRIPTOR configuration_descriptor;
	USB_INTERFACE_DESCRIPTOR interface_descriptor;
	USB_HID_DESCRIPTOR hid_descriptor;
	USB_ENDPOINT_DESCRIPTOR endpoint_descriptor;
	USB_DEVICE_DESCRIPTOR device_descriptor;
	bool poll_enabled;
	u32 next_poll_time;	
};