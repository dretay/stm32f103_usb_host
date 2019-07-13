#include "usbcore.h"

extern UART_HandleTypeDef huart1;


// Global variables
static u8 usb_buffer[2000];        // Big array to handle max size descriptor data

static u16 VID, PID, nak_count, IN_nak_count, HS_nak_count;
static unsigned int last_transfer_size;	
unsigned volatile long timeval;  			// incremented by timer0 ISR
u16 inhibit_send;


static USBDevice* my_usb_device;
static bool running = false;





static void poll(void)
{
	HAL_Delay(10);
	my_usb_device->poll();
}
static void set_device_address(u8 address)
{
	printfUART("Setting address to %d\r\n", address);
	// Issue another USB bus reset
	printfUART("Issuing USB bus reset\r\n");
		
	// initiate the 50 msec bus reset
	MAX3421E.write_register(rHCTL, bmBUSRST);             
		
	// Wait for the bus reset to complete
	while(MAX3421E.read_register(rHCTL) & bmBUSRST);    
	wait_frames(200);
	if (!my_control_write_no_data(bmREQ_SET, USB_REQUEST_SET_ADDRESS, address, 0, 0, 0))
	{
		my_usb_device->address = 7;			
		// Device gets 2 msec recovery time
		wait_frames(30);              
		// now all transfers go to addr 7
		MAX3421E.write_register(rPERADDR, my_usb_device->address);         						
	}
}
u8 rcode;
static void parse_report_descriptor()
{
	// Get the 9-byte configuration descriptor
		printfUART("\r\n\r\nHID Configuration Descriptor ");
	if (!my_control_read_transfer(0x81, USB_REQUEST_GET_DESCRIPTOR, 0, HID_REPORT_DESCRIPTOR, 0, 141))
	{
	}
}
static void init(USBDevice* usbdevice) 
{	
	my_usb_device = usbdevice;	
	MAX3421E.init();
	int probe_result = detect_device();
	MAX3421E.clear_conn_detect_irq();
//	MAX3421E.enable_irq();

	if(probe_result == LSHOST || probe_result == FSHOST) 
	{		
		MAX3421E.reset_bus();	
	}	
	initialize_device();	
	//peek_device_descriptor();
	set_device_address(7);
	parse_device_descriptor();
	parse_config_descriptor();
	MAX3421E.write_register(rHCTL, bmRCVTOG0);
	//configuration = 1
	my_control_write_no_data(bmREQ_SET, USB_REQUEST_SET_CONFIGURATION, 1, 0, 0, 0);
	//duration=indefinite report=0
	my_control_write_no_data(0x21, USB_REQUEST_GET_INTERFACE, 0, 0, 0, 0);
	
	parse_report_descriptor();

	//MAX3421E.soft_reset();	
	

	my_usb_device->poll_enabled = true;
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	
//	int probe_result;
//	u8 HIRQ_sendback = 0;
//	u8 HIRQ = MAX3421E.read_register(rHIRQ);  //determine interrupt source
//	if(HIRQ & bmCONDETIRQ) {
//		probe_result = detect_device();
//		HIRQ_sendback = HIRQ_sendback | bmCONDETIRQ;
//	}
//            
//	// End HIRQ interrupts handling, clear serviced IRQs
//	MAX3421E.write_register(rHIRQ, HIRQ_sendback);

	// Call the registered callback if the state has changed
//	u8 state_change = (probe_result != -1);
//	bool connected = (probe_result != 0);
//	u8 lowspeed = (probe_result == LSHOST);
//	if (state_change) {
//		if (connected != running) {
//			MAX3421E.reset_bus();
//			//connect(connected, _config, lowspeed);
//		} 
//		if (!connected) running = false;
//	}	
}

static int detect_device(void)
{
	int busstate = -1;
	// Activate HOST mode & turn on the 15K pulldown resistors on D+ and D-
	// Note--initially set up as a FS host (LOWSPEED=0)
	MAX3421E.write_register(rMODE, (bmDPPULLDN | bmDMPULLDN | bmHOST));  

	// clear the connection detect IRQ
	MAX3421E.write_register(rHIRQ, bmCONDETIRQ);   

	// See if anything is plugged in. If not, hang until something plugs in
	do 		
	{
		// update the JSTATUS and KSTATUS bits
		MAX3421E.write_register(rHCTL, bmSAMPLEBUS); 	
		
		// read them
		busstate = MAX3421E.read_register(rHRSL); 			
		
		// check for either of them high	
		busstate &= (bmJSTATUS | bmKSTATUS); 	
	} 
	while (busstate == 0) ;
	// since we're set to FS, J-state means D+ high
	if(busstate == bmJSTATUS)    
	{
		// make the MAX3421E a full speed host
		MAX3421E.write_register(rMODE, (bmDPPULLDN | bmDMPULLDN | bmHOST | bmSOFKAENAB));   
		printfUART("Full-Speed Device Detected\r\n");
	}
	if (busstate == bmKSTATUS)  // K-state means D- high
	{
		// make the MAX3421E a low speed host        
		MAX3421E.write_register(rMODE, (bmDPPULLDN | bmDMPULLDN | bmHOST | bmLOWSPEED | bmSOFKAENAB));   
		printfUART("Low-Speed Device Detected\r\n");
	}
	return busstate;
}

static void get_descriptor_string(u8 index, char* prefix)
{
	if (index != 0)
	{
		if (!my_control_read_transfer(bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, index, USB_DESCRIPTOR_STRING, 0, 0x40))
		{
			printfUART("%s  \"", prefix);
			for (int i = 2; i < last_transfer_size; i += 2)
			{
				printfUART("%c", (u8*)usb_buffer[i]);		
			}
			printfUART("\"\r\n");					
		}
	}	
}
static void get_language_id(void)
{	
	if (!my_control_read_transfer(bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, 0, USB_DESCRIPTOR_STRING, 0, 0x40))				
	{
		printfUART("\nLanguage ID String Descriptor \"");
		for (int i = 0; i < last_transfer_size; i++)
		{
			printfUART("%02X ", (u8*)usb_buffer[i]);			
		}
		printfUART("\"\r\n");	
	}
}
static void peek_device_descriptor()
{
	printfUART("First 8 bytes of Device Descriptor \r\n");
	if (!my_control_read_transfer(bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, 0, USB_DESCRIPTOR_DEVICE, 0, 8))
	{
		USB_DEVICE_DESCRIPTOR *descriptor = (USB_DEVICE_DESCRIPTOR*)usb_buffer;
		memcpy(&my_usb_device->device_descriptor, usb_buffer, sizeof(USB_DEVICE_DESCRIPTOR));

		printfUART("(%u/%u NAKS)\r\n", IN_nak_count, HS_nak_count);   		
	
		for (int i = 0; i < last_transfer_size; i++)
		{
			printfUART("%02X ", (u8*)usb_buffer[i]);
		}
		printfUART("\r\n");
		printfUART("EP0 maxPacketSize is %02u bytes\r\n", descriptor->bMaxPacketSize0);

	}
}
static void parse_device_descriptor()
{
	USB_DEVICE_DESCRIPTOR *descriptor= (USB_DEVICE_DESCRIPTOR*)usb_buffer;	
	printfUART("First 8 bytes of Device Descriptor \r\n");
	if (!my_control_read_transfer(bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, 0, USB_DESCRIPTOR_DEVICE, 0, 8))
	{	
//		 Get the device descriptor at the assigned address. 
//		 fill in the real device descriptor length	
		printfUART("\r\nDevice Descriptor ");
		
		MAX3421E.write_register(rHCTL, bmRCVTOG1);     //set toggle value
	
		if(!my_control_read_transfer(bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, 0, USB_DESCRIPTOR_DEVICE, 0, descriptor->bLength))
		{			
					
			memcpy(&my_usb_device->device_descriptor, usb_buffer, sizeof(USB_DEVICE_DESCRIPTOR));
			
			for (int i = 0; i < last_transfer_size; i++)
			{
				printfUART("%02X ", (u8*)usb_buffer[i]);			
			}
			printfUART("\r\n");
			printfUART("This device has %u configuration\r\n", descriptor->bNumConfigurations);
			printfUART("Vendor  ID is 0x%04X\r\n", descriptor->idVendor);
			printfUART("Product ID is 0x%04X\r\n", descriptor->idProduct);
			
			get_language_id();
			get_descriptor_string(my_usb_device->device_descriptor.iManufacturer, "Manufacturer ");	
			get_descriptor_string(my_usb_device->device_descriptor.iProduct, "Product");
			get_descriptor_string(my_usb_device->device_descriptor.iSerialNumber, "S/N");		
		}	
	}
}

static void parse_config_descriptor()
{
	
	// Get the 9-byte configuration descriptor
	printfUART("\r\n\r\nConfiguration Descriptor\r\n ");
	printfUART("------------------------\r\n");

	if (!my_control_read_transfer(bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, 0, USB_DESCRIPTOR_CONFIGURATION, 0, 9))
	{
		USB_CONFIGURATION_DESCRIPTOR *prelim_config = (USB_CONFIGURATION_DESCRIPTOR*)usb_buffer;		
		int descriptor_offset = 0;
		if(!my_control_read_transfer(bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, 0, USB_DESCRIPTOR_CONFIGURATION, 0, prelim_config->wTotalLength))
		{
			do
			{				
				uint8_t wDescriptorLength = usb_buffer[descriptor_offset];
				uint8_t bDescrType = usb_buffer[descriptor_offset + 1];

				switch(bDescrType)
				{
				case USB_DESCRIPTOR_CONFIGURATION:
					{
						USB_CONFIGURATION_DESCRIPTOR *descriptor = (USB_CONFIGURATION_DESCRIPTOR*)&usb_buffer[descriptor_offset];
						memcpy(&my_usb_device->configuration_descriptor, &usb_buffer[descriptor_offset], sizeof(USB_CONFIGURATION_DESCRIPTOR));

						printfUART("\r\nConfiguration Descriptor\r\n");
						printfUART("bLength: %d\r\n", descriptor->bLength);
						printfUART("bDescriptorType: %d\r\n", descriptor->bDescriptorType);
						printfUART("wTotalLength: %d\r\n", descriptor->wTotalLength);
						printfUART("bNumInterfaces: %d\r\n", descriptor->bNumInterfaces);
						printfUART("bConfigurationValue: %d\r\n", descriptor->bConfigurationValue);
						printfUART("iConfiguration: %d\r\n", descriptor->iConfiguration);																								
						
						printfUART("This device is ");
						if (descriptor->bmAttributes & 0x40)
						{
							printfUART("self-powered\r\n");
						}
						else
						{
							printfUART("bus powered and uses %03u milliamps\r\n", descriptor->bMaxPower * 2);
						}			
					}
					case USB_DESCRIPTOR_INTERFACE: 
					{			
						USB_INTERFACE_DESCRIPTOR *descriptor = (USB_INTERFACE_DESCRIPTOR*)&usb_buffer[descriptor_offset];
						memcpy(&my_usb_device->interface_descriptor, &usb_buffer[descriptor_offset], sizeof(USB_INTERFACE_DESCRIPTOR));

						printfUART("\r\nInterface Descriptor\r\n");				
						printfUART("bLength: %d\r\n", descriptor->bLength);
						printfUART("bDescriptorType: %d\r\n", descriptor->bDescriptorType);
						printfUART("bInterfaceNumber: %d\r\n", descriptor->bInterfaceNumber);
						printfUART("bAlternateSetting: %d\r\n", descriptor->bAlternateSetting);
						printfUART("bNumEndpoints: %d\r\n", descriptor->bNumEndpoints);
						printfUART("bInterfaceClass: %d\r\n", descriptor->bInterfaceClass);
						printfUART("bInterfaceSubClass: %d\r\n", descriptor->bInterfaceSubClass);
						printfUART("bInterfaceProtocol: %d\r\n", descriptor->bInterfaceProtocol);
						printfUART("iInterface: %d\r\n", descriptor->iInterface);
						break;
					}			
					case HID_DESCRIPTOR_HID: 
					{
						USB_HID_DESCRIPTOR *descriptor = (USB_HID_DESCRIPTOR*)&usb_buffer[descriptor_offset];
						memcpy(&my_usb_device->hid_descriptor, &usb_buffer[descriptor_offset], sizeof(USB_HID_DESCRIPTOR));
						printfUART("\r\nHID Descriptor\r\n");				
						printfUART("bLength: %d\r\n", descriptor->bLength);
						printfUART("bDescriptorType: %d\r\n", descriptor->bDescriptorType);
						printfUART("bcdHID: %d\r\n", descriptor->bcdHID);
						printfUART("bCountryCode: %d\r\n", descriptor->bCountryCode);
						printfUART("bNumDescriptors: %d\r\n", descriptor->bNumDescriptors);
						printfUART("bDescriptorType: %d\r\n", descriptor->bDescriptorType);
						printfUART("wDescriptorLength: %d\r\n", descriptor->wDescriptorLength);
						break;			
					}
					case USB_DESCRIPTOR_ENDPOINT:
					{
						printfUART("\r\nEndpoint Descriptor\r\n");				
						// check for endpoint descriptor type				
						USB_ENDPOINT_DESCRIPTOR *descriptor = (USB_ENDPOINT_DESCRIPTOR*)&usb_buffer[descriptor_offset];
						memcpy(&my_usb_device->endpoint_descriptor, &usb_buffer[descriptor_offset], sizeof(USB_ENDPOINT_DESCRIPTOR));
						my_usb_device->endpoint_descriptor.bEndpointAddress = (descriptor->bEndpointAddress & 0x0F);
						printfUART("Endpoint %u", (descriptor->bEndpointAddress & 0x0F));
						if (descriptor->bEndpointAddress & 0x80)
						{
							printfUART("-IN  ");
						}
						else 
						{
							printfUART("-OUT ");
						}
						printfUART("(%02u) is type ", (u8)descriptor->wMaxPacketSize);

						switch (descriptor->bmAttributes & 0x03)
						{
						case USB_TRANSFER_TYPE_CONTROL:
							printfUART("CONTROL\r\n"); 
							break;
						case USB_TRANSFER_TYPE_ISOCHRONOUS:
							printfUART("ISOCHRONOUS\r\n"); 
							break;
						case USB_TRANSFER_TYPE_BULK:
							printfUART("BULK\r\n"); 
							break;
						case USB_TRANSFER_TYPE_INTERRUPT:
							printfUART("INTERRUPT with a polling interval of %u msec.\r\n", descriptor->bInterval);							
						}
						break;
					}
				}		
				descriptor_offset += wDescriptorLength;  				
			} while (descriptor_offset < last_transfer_size);
		}
	}	
}

static void initialize_device()
{
	// First request goes to address 0
	my_usb_device->address = 0;
	MAX3421E.write_register(rPERADDR, my_usb_device->address);    					
	
}



static u8 my_control_read_transfer(u8 bmRequestType, u8 bRequest, u8 wValueLo, u8 wValueHi, u16 wIndex, u16 wLength) 
{
	SETUP_PKT setup_pkt;
	setup_pkt.ReqType_u.bmRequestType = bmRequestType;
	setup_pkt.bRequest = bRequest;
	setup_pkt.wVal_u.wValueLo = wValueLo;
	setup_pkt.wVal_u.wValueHi = wValueHi;
	setup_pkt.wIndex = wIndex;
	setup_pkt.wLength = wLength;
	return control_read_transfer((uint8_t*)&setup_pkt);
}
static u8 control_read_transfer(u8 *pSUD)
{
	u8  resultcode;
	u16	bytes_to_read;
	bytes_to_read = pSUD[6] + 256*pSUD[7];

	// SETUP packet
	// Load the Setup data FIFO
	MAX3421E.write_bytes(rSUDFIFO, 8, pSUD);       		
	
	// SETUP packet to EP0
	resultcode = send_packet(tokSETUP, 0);    	
	
	// should be 0, indicating ACK. Else return error code.
	if(resultcode)
	{
		return (resultcode); 
	}
	
	// One or more IN packets (may be a multi-packet transfer)
	// FIRST Data packet in a CTL transfer uses DATA1 toggle.
	MAX3421E.write_register(rHCTL, bmRCVTOG1);             			
	resultcode = in_transfer(0, bytes_to_read); 
	if (resultcode) 
	{
		return (resultcode);
	}

	IN_nak_count = nak_count;
	// The OUT status stage
	resultcode = send_packet(tokOUTHS, 0);
	if (resultcode) 
	{
		print_error(resultcode);
		// should be 0, indicating ACK. Else return error code. 
		return (resultcode);  
	}
	return (0);     
}

static u8 in_transfer(u8 endpoint, u16 INbytes)
{
	u8 resultcode, j, pktsize;
	u32 xfrlen, xfrsize;

	xfrsize = INbytes;
	xfrlen = 0;

	while (1) 
	{
		// IN packet to EP-'endpoint'. Function takes care of NAKS.
		resultcode = send_packet(tokIN, endpoint);      	
		if (resultcode)
		{
			// should be 0, indicating ACK. Else return error code.  
			return (resultcode);   		
		}
		// number of received bytes
		pktsize = MAX3421E.read_register(rRCVBC);                         
		
		// add this packet's data to XfrData array
		for(j = 0 ; j < pktsize ; j++) 
		{
			usb_buffer[j + xfrlen] = MAX3421E.read_register(rRCVFIFO);         
		}

		// Clear the IRQ & free the buffer
		MAX3421E.write_register(rHIRQ, bmRCVDAVIRQ);                      
		
		// add this packet's byte count to total transfer length
		xfrlen += pktsize;            
		
		// The transfer is complete when something smaller than max_packet_size is sent or 'INbytes' have been transfered		
		if((pktsize < my_usb_device->device_descriptor.bMaxPacketSize0) || (xfrlen >= xfrsize))
		{
			last_transfer_size = xfrlen;
			return (resultcode);
		}
	}
}
static u8 my_control_write_no_data(u8 bmRequestType, u8 bRequest, u8 wValueLo, u8 wValueHi, u16 wIndex, u16 wLength) 
{
	SETUP_PKT setup_pkt;
	setup_pkt.ReqType_u.bmRequestType = bmRequestType;
	setup_pkt.bRequest = bRequest;
	setup_pkt.wVal_u.wValueLo = wValueLo;
	setup_pkt.wVal_u.wValueHi = wValueHi;
	setup_pkt.wIndex = wIndex;
	setup_pkt.wLength = wLength;
	return control_write_no_data((uint8_t*)&setup_pkt);
}
static u8 control_write_no_data(u8 *pSUD)
{
	u8 resultcode;
	MAX3421E.write_bytes(rSUDFIFO, 8, pSUD);
	//Send the SETUP token and 8 setup bytes. Device should immediately ACK.
	resultcode = send_packet(tokSETUP, 0);     
	if (resultcode)
	{
		// should be 0, indicating ACK.
		return (resultcode);    
	}

	//No data stage, so the last operation is to send an IN token to the peripheral
	//as the STATUS (handshake) stage of this control transfer. We should get NAK or the
	//DATA1 PID. When we get the DATA1 PID the 3421 automatically sends the closing ACK.
	resultcode = send_packet(tokINHS, 0);    
	if (resultcode)
	{
		// should be 0, indicating ACK.
		return (resultcode);   
	}
	else
	{
		return (0);
	}
}



static void wait_frames(u8 num)
{
	u8 k = 0;
	// clear any pending 
	MAX3421E.write_register(rHIRQ, bmFRAMEIRQ);      	
	
	while (k != num)               
	{		
		while (!(MAX3421E.read_register(rHIRQ)& bmFRAMEIRQ));
		
		// clear the IRQ
		MAX3421E.write_register(rHIRQ, bmFRAMEIRQ);  
		k++;
	}
}


static u8 send_packet(u8 token, u8 endpoint)
{
	u8 resultcode, retry_count = 0;	
	nak_count = 0;
	
	// If the response is NAK or timeout, keep sending until either NAK_LIMIT or RETRY_LIMIT is reached.
	while(1) 					
	{                                     
		// launch the transfer
		MAX3421E.write_register(rHXFR, (token | endpoint));          
		// wait for the completion IRQ
		while(!(MAX3421E.read_register(rHIRQ) & bmHXFRDNIRQ));   
		
		// clear the IRQ
		MAX3421E.write_register(rHIRQ, bmHXFRDNIRQ);               
		
		// get the result
		resultcode = (MAX3421E.read_register(rHRSL) & 0x0F);     
		if (resultcode == hrNAK) 
		{
			nak_count++;
			if (nak_count == NAK_LIMIT) 
			{
				break;
			}
			else
			{	
				continue;
			}
		}

		if (resultcode == hrTIMEOUT)
		{
			retry_count++;
			if (retry_count == RETRY_LIMIT)
			{
				// hit the max allowed retries. Exit and return result code
				break;    
			}
			else
			{
				continue;
			}
		}
		else
		{
			// all other cases, just return the success or error code
			break;                           	
		}
	}
	return (resultcode);
}

static u8 print_error(u8 err)
{
	if (err)
	{
		printfUART(">>>>> Error >>>>> \r\n");
		switch (err)
		{
		case hrBUSY: 
			printfUART("MAX3421E SIE is busy "); 
			break;
		case hrBADREQ: 
			printfUART("Bad value in HXFR register "); 
			break;
		case hrNAK: 
			printfUART("Exceeded NAK limit"); 
			break;
		case hrKERR: 
			printfUART("LS Timeout "); 
			break;
		case hrJERR: 
			printfUART("FS Timeout "); 
			break;
		case hrTIMEOUT: 
			printfUART("Device did not respond in time "); 
			break;
		case hrBABBLE: 
			printfUART("Device babbled (sent too long) "); 
			break;
		default:   
			printfUART("Programming error %01X,", err);
		}
	}
	return (err);
}
static u8* get_usb_buffer(void)
{
	return usb_buffer;
}
const struct usbcore USBCORE= { 
	.init = init,		
	.poll = poll,
	.in_transfer = in_transfer,
	.get_usb_buffer = get_usb_buffer,	
	.send_packet = send_packet,
};

