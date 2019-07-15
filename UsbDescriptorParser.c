#include "UsbDescriptorParser.h"

static void get_descriptor_string(u8 index, char* prefix)
{
	u32 *buffer_size = USBCORE.get_last_transfer_size();
	u8 *usb_buffer = USBCORE.get_usb_buffer();
	if (index != 0)
	{
		if (!USBCORE.control_read_transfer(bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, index, USB_DESCRIPTOR_STRING, 0, 0x40))
		{
			_DEBUG("\t%s  ", prefix);
			for (int i = 2; i < *buffer_size; i += 2)
			{
				__DEBUG("%c", usb_buffer[i]);		
			}							
		}
	}	
}

static void peek_device_descriptor(USBDevice *my_usb_device)
{	
	if (!USBCORE.control_read_transfer(bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, 0, USB_DESCRIPTOR_DEVICE, 0, 8))
	{
		u32 *buffer_size = USBCORE.get_last_transfer_size();
		u8 *usb_buffer = USBCORE.get_usb_buffer();
		USB_DEVICE_DESCRIPTOR *descriptor = (USB_DEVICE_DESCRIPTOR*)usb_buffer;
		memcpy(&my_usb_device->device_descriptor, usb_buffer, sizeof(USB_DEVICE_DESCRIPTOR));
				
		_DEBUG("EP0 maxPacketSize is %02u bytes", descriptor->bMaxPacketSize0);

	}
}
static void parse_device_descriptor(USBDevice *my_usb_device)
{
	u32 *buffer_size = USBCORE.get_last_transfer_size();
	u8 *usb_buffer = USBCORE.get_usb_buffer();
	USB_DEVICE_DESCRIPTOR *descriptor = (USB_DEVICE_DESCRIPTOR*)usb_buffer;		
	_DEBUG("", 0);
	_DEBUG("Device Descriptor ",0);
			
	if(!USBCORE.control_read_transfer(bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, 0, USB_DESCRIPTOR_DEVICE, 0, descriptor->bLength))
	{			
					
		memcpy(&my_usb_device->device_descriptor, usb_buffer, sizeof(USB_DEVICE_DESCRIPTOR));
		
		_DEBUG("\tThis device has %u configuration", descriptor->bNumConfigurations);
		_DEBUG("\tVendor  ID: 0x%04X", descriptor->idVendor);
		_DEBUG("\tProduct ID: 0x%04X", descriptor->idProduct);
			
		get_descriptor_string(my_usb_device->device_descriptor.iManufacturer, "Manufacturer: ");	
		get_descriptor_string(my_usb_device->device_descriptor.iProduct, "Product: ");
		get_descriptor_string(my_usb_device->device_descriptor.iSerialNumber, "S/N: ");		
	}		
}

static void parse_config_descriptor(USBDevice *my_usb_device)
{
	
	// Get the 9-byte configuration descriptor
	u32 *buffer_size = USBCORE.get_last_transfer_size();
	u8 *usb_buffer = USBCORE.get_usb_buffer();

	if (!USBCORE.control_read_transfer(bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, 0, USB_DESCRIPTOR_CONFIGURATION, 0, 9))
	{
		USB_CONFIGURATION_DESCRIPTOR *prelim_config = (USB_CONFIGURATION_DESCRIPTOR*)usb_buffer;		
		int descriptor_offset = 0;
		if (!USBCORE.control_read_transfer(bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, 0, USB_DESCRIPTOR_CONFIGURATION, 0, prelim_config->wTotalLength))
		{
			do
			{				
				uint8_t wDescriptorLength = usb_buffer[descriptor_offset];
				uint8_t bDescrType = usb_buffer[descriptor_offset + 1];

				switch (bDescrType)
				{
				case USB_DESCRIPTOR_CONFIGURATION:
					{
						USB_CONFIGURATION_DESCRIPTOR *descriptor = (USB_CONFIGURATION_DESCRIPTOR*)&usb_buffer[descriptor_offset];
						memcpy(&my_usb_device->configuration_descriptor, &usb_buffer[descriptor_offset], sizeof(USB_CONFIGURATION_DESCRIPTOR));
						_DEBUG("", 0);
						_DEBUG("Configuration Descriptor",0);
						_DEBUG("\tbLength: %d", descriptor->bLength);
						_DEBUG("\tbDescriptorType: %d", descriptor->bDescriptorType);
						_DEBUG("\twTotalLength: %d", descriptor->wTotalLength);
						_DEBUG("\tbNumInterfaces: %d", descriptor->bNumInterfaces);
						_DEBUG("\tbConfigurationValue: %d", descriptor->bConfigurationValue);
						_DEBUG("\tiConfiguration: %d", descriptor->iConfiguration);																								
						
						_DEBUG("\tThis device is ",0);
						if (descriptor->bmAttributes & 0x40)
						{
							__DEBUG("self-powered");
						}
						else
						{
							__DEBUG("bus powered and uses %03u milliamps", descriptor->bMaxPower * 2);
						}			
					}
				case USB_DESCRIPTOR_INTERFACE: 
					{			
						USB_INTERFACE_DESCRIPTOR *descriptor = (USB_INTERFACE_DESCRIPTOR*)&usb_buffer[descriptor_offset];
						memcpy(&my_usb_device->interface_descriptor, &usb_buffer[descriptor_offset], sizeof(USB_INTERFACE_DESCRIPTOR));
						_DEBUG("", 0);
						_DEBUG("Interface Descriptor",0);				
						_DEBUG("\tbLength: %d", descriptor->bLength);
						_DEBUG("\tbDescriptorType: %d", descriptor->bDescriptorType);
						_DEBUG("\tbInterfaceNumber: %d", descriptor->bInterfaceNumber);
						_DEBUG("\tbAlternateSetting: %d", descriptor->bAlternateSetting);
						_DEBUG("\tbNumEndpoints: %d", descriptor->bNumEndpoints);
						_DEBUG("\tbInterfaceClass: %d", descriptor->bInterfaceClass);
						_DEBUG("\tbInterfaceSubClass: %d", descriptor->bInterfaceSubClass);
						_DEBUG("\tbInterfaceProtocol: %d", descriptor->bInterfaceProtocol);
						_DEBUG("\tiInterface: %d", descriptor->iInterface);
						break;
					}			
				case HID_DESCRIPTOR_HID: 
					{
						USB_HID_DESCRIPTOR *descriptor = (USB_HID_DESCRIPTOR*)&usb_buffer[descriptor_offset];
						memcpy(&my_usb_device->hid_descriptor, &usb_buffer[descriptor_offset], sizeof(USB_HID_DESCRIPTOR));
						_DEBUG("", 0);
						_DEBUG("HID Descriptor",0);				
						_DEBUG("\tbLength: %d", descriptor->bLength);
						_DEBUG("\tbDescriptorType: %d", descriptor->bDescriptorType);
						_DEBUG("\tbcdHID: %d", descriptor->bcdHID);
						_DEBUG("\tbCountryCode: %d", descriptor->bCountryCode);
						_DEBUG("\tbNumDescriptors: %d", descriptor->bNumDescriptors);
						_DEBUG("\tbDescriptorType: %d", descriptor->bDescriptorType);
						_DEBUG("\twDescriptorLength: %d", descriptor->wDescriptorLength);
						break;			
					}
				case USB_DESCRIPTOR_ENDPOINT:
					{
						_DEBUG("", 0);
						// check for endpoint descriptor type				
						USB_ENDPOINT_DESCRIPTOR *descriptor = (USB_ENDPOINT_DESCRIPTOR*)&usb_buffer[descriptor_offset];
						memcpy(&my_usb_device->endpoint_descriptor, &usb_buffer[descriptor_offset], sizeof(USB_ENDPOINT_DESCRIPTOR));
						my_usb_device->endpoint_descriptor.bEndpointAddress = (descriptor->bEndpointAddress & 0x0F);
						_DEBUG("Endpoint %u", (descriptor->bEndpointAddress & 0x0F));
						if (descriptor->bEndpointAddress & 0x80)
						{
							__DEBUG("-IN ",0);
						}
						else 
						{
							__DEBUG("-OUT ",0);
						}
						__DEBUG("(%02u) is type ", (u8)descriptor->wMaxPacketSize);

						switch (descriptor->bmAttributes & 0x03)
						{
						case USB_TRANSFER_TYPE_CONTROL:
							__DEBUG("CONTROL"); 
							break;
						case USB_TRANSFER_TYPE_ISOCHRONOUS:
							__DEBUG("ISOCHRONOUS"); 
							break;
						case USB_TRANSFER_TYPE_BULK:
							__DEBUG("BULK"); 
							break;
						case USB_TRANSFER_TYPE_INTERRUPT:
							__DEBUG("INTERRUPT with a polling interval of %u msec.", descriptor->bInterval);							
						}
						break;
					}
				}		
				descriptor_offset += wDescriptorLength;  				
			} while (descriptor_offset < *buffer_size);
		}
	}	
}
const struct usbdescriptorparser UsbDescriptorParser = { 	
	.peek_device_descriptor = peek_device_descriptor,
	.parse_device_descriptor = parse_device_descriptor,
	.parse_config_descriptor = parse_config_descriptor,
};
