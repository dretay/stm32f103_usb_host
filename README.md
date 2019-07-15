## STM32F103-based library to interface with a MAX3421e USB Host ##

### Overview ###

Example code and supporting structure that illustrates a simple HID enumeration with a device connected to a MAX3421e. 
### Main Features ###
- Structs emulate an "interface" in C, so you can swap the HID device with other implementations
- Multilevel logging helps with debugging an issue. You can then disable lower level logging when issue is resolved.


### Summary ###
In your main method you simply need to initialize the USB module like this and then call poll in a loop to process bus events.
```c
static void init(void)
{	
	USBCORE.init(HIDUniversal.new());
}
static void process(void)
{	
	USBCORE.poll();
}
```
