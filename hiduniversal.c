#include "hiduniversal.h"

static USBDevice device;
static uint8_t prevBuf[constBuffLen];

static void zero_memory(uint8_t len, uint8_t *buf) {
	for (uint8_t i = 0; i < len; i++)
		buf[i] = 0;
}
static bool buffers_identical(uint8_t len, uint8_t *buf1, uint8_t *buf2) {
	for (uint8_t i = 0; i < len; i++)
		if (buf1[i] != buf2[i])
			return false;
	return true;
}
static void save_buffer(uint8_t len, uint8_t *src, uint8_t *dest) {
	for (uint8_t i = 0; i < len; i++)
		dest[i] = src[i];
}
static void parse()
{
}

typedef struct {

	struct {
		uint8_t bmLeftButton : 1;
		uint8_t bmRightButton : 1;
		uint8_t bmMiddleButton : 1;
		uint8_t bmDummy : 5;
	};
	int8_t dX;
	int8_t dY;
} MOUSEINFO;

static u8 poll()
{
	uint8_t rcode = 0;
	if (!device.poll_enabled)
	{
		return rcode;		
	}

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
	if ((int32_t)(HAL_GetTick() - device.next_poll_time) >= 0L) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
		device.next_poll_time = HAL_GetTick() + device.endpoint_descriptor.bInterval;		
		uint8_t buf[constBuffLen];
		uint16_t read = device.endpoint_descriptor.wMaxPacketSize;
		zero_memory(constBuffLen, buf);	
			
		rcode = USBCORE.in_transfer(device.endpoint_descriptor.bEndpointAddress, read);						

		if (rcode) {
			if (rcode != hrNAK)
				//USBTRACE3("(hiduniversal.h) Poll:", rcode, 0x81);
			return rcode;
		}
		memcpy(buf, USBCORE.get_usb_buffer(), constBuffLen);

		if (read > constBuffLen)
			read = constBuffLen;

		bool identical = buffers_identical(read, buf, prevBuf);

		if (!identical)
		{
			MOUSEINFO *mouse_info = (MOUSEINFO*)buf;
			MOUSEINFO *prev_mouse_info = (MOUSEINFO*)prevBuf;

			if (prev_mouse_info->bmLeftButton == 0 && mouse_info->bmLeftButton == 1)
			{
				_INFO("Left Mouse Down", 0);
			}
			if (prev_mouse_info->bmLeftButton == 1 && mouse_info->bmLeftButton == 0)
			{
				_INFO("Left Mouse Up", 0);
			}			
			if (prev_mouse_info->bmRightButton == 0 && mouse_info->bmRightButton == 1)
			{
				_INFO("Right Mouse Down", 0);
			}			
			if (prev_mouse_info->bmRightButton == 1 && mouse_info->bmRightButton == 0)
			{
				_INFO("Right Mouse Up", 0);
			}
			if (prev_mouse_info->bmMiddleButton == 0 && mouse_info->bmMiddleButton == 1)
			{
				_INFO("Middle Mouse Down", 0);
			}
			if (prev_mouse_info->bmMiddleButton == 1 && mouse_info->bmMiddleButton == 0)
			{
				_INFO("Middle Mouse Up", 0);
			}			
			if (prev_mouse_info->dX != mouse_info->dX || prev_mouse_info->dY != mouse_info->dY)
			{
				_INFO("dX=%d dY=%d", mouse_info->dX, mouse_info->dY, 0);
			}			
		}
		save_buffer(read, buf, prevBuf);
	}	
	return rcode;
}
static void parse_report_descriptor()
{
	// Get the 9-byte configuration descriptor
	_DEBUG("", 0);
	_DEBUG("HID Configuration Descriptor ",0);
	if (!USBCORE.control_read_transfer(0x81, USB_REQUEST_GET_DESCRIPTOR, 0, HID_REPORT_DESCRIPTOR, 0, 141))
	{
	}
}
static void configure(void)
{
	MAX3421E.write_register(rHCTL, bmRCVTOG0);
	//configuration = 1
	USBCORE.control_write_no_data(bmREQ_SET, USB_REQUEST_SET_CONFIGURATION, 1, 0, 0, 0);
	
	//duration=indefinite report=0
	USBCORE.control_write_no_data(0x21, USB_REQUEST_GET_INTERFACE, 0, 0, 0, 0);

	parse_report_descriptor();
	
	device.poll_enabled = true;
}
static USBDevice* new(void)
{
	device.poll_enabled = false;
	device.next_poll_time = 0;
	device.poll= poll;
	device.parse = parse;	
	device.configure = configure;
	return &device;
}
const struct hiduniversal HIDUniversal = { 
	.new = new,			
};

