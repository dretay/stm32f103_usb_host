#include "usbcore.h"

extern UART_HandleTypeDef huart1;


// Global variables
static u8 usb_buffer[2000];        // Big array to handle max size descriptor data

static u16 VID, PID, nak_count, IN_nak_count, HS_nak_count;
static unsigned int last_transfer_size;	
unsigned volatile long timeval;  			// incremented by timer0 ISR
u16 inhibit_send;


static USBDevice* my_usb_device;



//from: https://electronics.stackexchange.com/questions/206113/how-do-i-use-the-printf-function-on-stm32
void vprint(const char *fmt, va_list argp)
{
	char string[200];
	if (0 < vsprintf(string, fmt, argp)) // build string
		{
			HAL_UART_Transmit(&huart1, (uint8_t*)string, strlen(string), 0xffffff);   // send message via UART
		}
}

void printfUART(const char *fmt, ...) // custom printf() function
{
	va_list argp;
	va_start(argp, fmt);
	vprint(fmt, argp);
	va_end(argp);
}


static void poll(void)
{
	my_usb_device->poll();
}
static void init(USBDevice* usbdevice) 
{	
	my_usb_device = usbdevice;	
	MAX3421E.init();
	probe_bus();
	
}
void probe_bus(void) {
	detect_device();
	// Some devices require this
	wait_frames(200);  			
	enumerate_device();
	my_usb_device->poll_enabled = true;
}


static void detect_device(void)
{
	int busstate;
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
static void get_device_descriptor()
{
	
	// Get the device descriptor at the assigned address. 
	// fill in the real device descriptor length	
	printfUART("\r\nDevice Descriptor ");
	
	if (!my_control_read_transfer(bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, 0, USB_DESCRIPTOR_DEVICE, 0, 0x12))
	{
		memcpy(&my_usb_device->device_descriptor, usb_buffer, sizeof(USB_DEVICE_DESCRIPTOR));
		printfUART("(%u/%u NAKS)\r\n", IN_nak_count, HS_nak_count);
		printf("-----------------\r\n");

		for (int i = 0; i < last_transfer_size; i++)
		{
			printfUART("%02X ", (u8*)usb_buffer[i]);			
		}
		printfUART("\r\n");
		printfUART("This device has %u configuration\r\n", my_usb_device->device_descriptor.bNumConfigurations);
		printfUART("Vendor  ID is 0x%04X\r\n", my_usb_device->device_descriptor.idVendor);
		printfUART("Product ID is 0x%04X\r\n", my_usb_device->device_descriptor.idProduct);
	
		get_language_id();
		get_descriptor_string(my_usb_device->device_descriptor.iManufacturer, "Manufacturer ");	
		get_descriptor_string(my_usb_device->device_descriptor.iProduct, "Product");
		get_descriptor_string(my_usb_device->device_descriptor.iSerialNumber, "S/N");		
	}	
}

static void initialize_device()
{
	// First request goes to address 0
	my_usb_device->address = 0;
	MAX3421E.write_register(rPERADDR, my_usb_device->address);    			
	printfUART("First 8 bytes of Device Descriptor \r\n");

	if (!my_control_read_transfer(bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, 0, USB_DESCRIPTOR_DEVICE, 0, 8))
	{
		USB_DEVICE_DESCRIPTOR *my_descriptor = (USB_DEVICE_DESCRIPTOR*)&usb_buffer;	

		printfUART("(%u/%u NAKS)\r\n", IN_nak_count, HS_nak_count);   		
	
		for (int i = 0; i < last_transfer_size; i++)
		{
			printfUART("%02X ", (u8*)usb_buffer[i]);
		}
		printfUART("\r\n");
		printfUART("EP0 maxPacketSize is %02u bytes\r\n", my_descriptor->bMaxPacketSize0);

		// Issue another USB bus reset
		printfUART("Issuing USB bus reset\r\n");
		
		// initiate the 50 msec bus reset
		MAX3421E.write_register(rHCTL, bmBUSRST);             
		
		// Wait for the bus reset to complete
		while(MAX3421E.read_register(rHCTL) & bmBUSRST);    
		wait_frames(200);

		// Set_Address to 7 (Note: this request goes to address 0, already set in PERADDR register).				
		printfUART("Setting address to 0x07\r\n");
		if (!my_control_write_no_data(bmREQ_SET, USB_REQUEST_SET_ADDRESS, 7, 0, 0, 0))
		{
			my_usb_device->address = 7;			
			// Device gets 2 msec recovery time
			wait_frames(30);              
			// now all transfers go to addr 7
			MAX3421E.write_register(rPERADDR, my_usb_device->address);         			
			my_usb_device->max_packet_size  = my_descriptor->bMaxPacketSize0;
		}
	}	
}
static void get_config_descriptor()
{
	
	// Get the 9-byte configuration descriptor
	printfUART("\r\n\r\nConfiguration Descriptor ");
	if (!my_control_read_transfer(bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, 0, USB_DESCRIPTOR_CONFIGURATION, 0, 9))
	{
		printfUART("(%u/%u NAKS)\r\n", IN_nak_count, HS_nak_count);
		printfUART("------------------------\r\n");
		for (int i = 0; i < last_transfer_size; i++)
		{
			printfUART("%02X ", (u8*)usb_buffer[i]);		
		}
		printfUART("\r\n");		

		// Now that the full length of all descriptors (Config, Interface, Endpoint, maybe Class)
		// is known we can fill in the correct length and ask for the full boat.
		if(!my_control_read_transfer(bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, 0, USB_DESCRIPTOR_CONFIGURATION, 0, (usb_buffer[2] + 256*usb_buffer[3])))
		{
			memcpy(&my_usb_device->config_descriptor, usb_buffer, sizeof(USB_CONFIGURATION_DESCRIPTOR));
			printfUART("\r\nFull Configuration Data");
			for (int i = 0; i < last_transfer_size; i++)
			{				
				if ((i & 0x0F) == 0) 
				{	
					printfUART("\r\n");   		
				}
				printfUART("%02X ", (u8*)usb_buffer[i]);
			}
			printfUART("\r\nConfiguration %01X has %01X interface", my_usb_device->config_descriptor.bConfigurationValue, my_usb_device->config_descriptor.bNumInterfaces);
			if (my_usb_device->config_descriptor.bNumInterfaces > 1) 
			{
				printfUART("s");
			}
			printfUART("\r\nThis device is ");
			if (my_usb_device->config_descriptor.bmAttributes & 0x40)
			{
				printfUART("self-powered\r\n");
			}
			else
			{
				printfUART("bus powered and uses %03u milliamps\r\n", my_usb_device->config_descriptor.bMaxPower * 2);
			}			
		}
	}
}
static void get_config_string(void)
{
	if (my_usb_device->config_descriptor.iConfiguration > 0)
	{
		if (!my_control_read_transfer(bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, my_usb_device->config_descriptor.iConfiguration, USB_DESCRIPTOR_STRING, 0, 4))
		{
			printfUART("\r\nConfig string is \"");
			for (int i = 2; i < last_transfer_size; i += 2)
			{
				printfUART("%c", (u8*)usb_buffer[i]);				
			}
			printfUART("\"\r\n");			
		}
	}
	else 
	{
		printfUART("There is no Config string\r\n");
	}
}
static void get_endpoint_descriptor(void)
{
	int i = 0;
	
	do
	{		
		memcpy(&my_usb_device->endpoint_descriptor, &usb_buffer[i], sizeof(USB_ENDPOINT_DESCRIPTOR));

		if (my_usb_device->endpoint_descriptor.bDescriptorType == USB_DESCRIPTOR_INTERFACE)
		{	
			printfUART("Interface %u, Alternate Setting %u has:\r\n", my_usb_device->endpoint_descriptor.bEndpointAddress, my_usb_device->endpoint_descriptor.bmAttributes);
		}
		else if (my_usb_device->endpoint_descriptor.bDescriptorType == USB_DESCRIPTOR_ENDPOINT)		
		{
			// check for endpoint descriptor type
			printfUART("--Endpoint %u", (my_usb_device->endpoint_descriptor.bEndpointAddress & 0x0F));
			if (my_usb_device->endpoint_descriptor.bEndpointAddress & 0x80)
			{
				printfUART("-IN  ");
			}
			else 
			{
				printfUART("-OUT ");
			}
			printfUART("(%02u) is type ", (u8)my_usb_device->endpoint_descriptor.wMaxPacketSize);

			switch (my_usb_device->endpoint_descriptor.bmAttributes & 0x03)
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
				printfUART("INTERRUPT with a polling interval of %u msec.\r\n", my_usb_device->endpoint_descriptor.bInterval);
			}
		}
		i += my_usb_device->endpoint_descriptor.bLength;  				
	} while (i < last_transfer_size);
}
static void enumerate_device(void)
{		
	// Issue a USB bus reset and wait for it to complete
	printfUART("Issuing USB bus reset\r\n");
	MAX3421E.write_register(rHCTL, bmBUSRST);            		
	while (MAX3421E.read_register(rHCTL) & bmBUSRST) ;   

	// Wait some frames to let device revover before programming any transfers. 
	wait_frames(200);  
	
	//interogate the connected device
	initialize_device();
	get_device_descriptor();
	get_config_descriptor();
	get_endpoint_descriptor();
	get_config_string();

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
		if((pktsize < my_usb_device->max_packet_size) || (xfrlen >= xfrsize))
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
};

