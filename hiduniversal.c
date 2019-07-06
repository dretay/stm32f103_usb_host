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
static u8 poll()
{
	uint8_t rcode = 0;
	if (!device.poll_enabled)
	{
		return rcode;		
	}

	if ((int32_t)(HAL_GetTick() - device.next_poll_time) >= 0L) {
		device.next_poll_time = HAL_GetTick() + device.endpoint_descriptor.bInterval;
		//MAX3421E.write_register(rHCTL, bmRCVTOG0 );  //set toggle value
		//u8 mode = MAX3421E.read_register(rMODE);
		//u8 bmHubPre = 0;
		//MAX3421E.write_register(rMODE, (true) ? mode | bmLOWSPEED | bmHubPre : mode & ~(bmHUBPRE | bmLOWSPEED));
		//rcode = USBCORE.send_packet(tokIN, 0x01);
		uint8_t buf[constBuffLen];

		
//		for (uint8_t i = 0; i < device.config_descriptor.bNumInterfaces; i++) {
			//uint8_t index = hidInterfaces[i].epIndex[epInterruptInIndex];
			uint16_t read = 8;//device.endpoint_descriptor.wMaxPacketSize;

			zero_memory(constBuffLen, buf);	
			//USBCORE.set_address(7, 1);
			
			
			rcode = USBCORE.in_transfer(1, 8);			
			

			if (rcode) {
				if (rcode != hrNAK)
					//USBTRACE3("(hiduniversal.h) Poll:", rcode, 0x81);
				return rcode;
			}
			memcpy(buf, USBCORE.get_usb_buffer(), constBuffLen);

			if (read > constBuffLen)
				read = constBuffLen;

			bool identical = buffers_identical(read, buf, prevBuf);

			save_buffer(read, buf, prevBuf);

			if (identical)
			{
				return 0;				
			}
#if 0
			Notify(PSTR("\r\nBuf: "), 0x80);

			for (uint8_t i = 0; i < read; i++) {
				D_PrintHex<uint8_t > (buf[i], 0x80);
				Notify(PSTR(" "), 0x80);
			}

			Notify(PSTR("\r\n"), 0x80);
#endif
//			ParseHIDData(this, bHasReportId, (uint8_t)read, buf);
//
//			HIDReportParser *prs = GetReportParser(((bHasReportId) ? *buf : 0));
//
//			if (prs)
//				prs->Parse(this, bHasReportId, (uint8_t)read, buf);
//		}
	}
	return rcode;
}

static USBDevice* configure(void)
{
	device.poll_enabled = false;
	device.next_poll_time = 0;
	device.poll= poll;
	device.parse = parse;	
	return &device;
}
const struct hiduniversal HIDUniversal = { 
	.configure = configure,			
};

