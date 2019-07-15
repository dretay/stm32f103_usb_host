#include "application.h"
#include "hiduniversal.h"

static void init(void)
{	
	USBCore.init(HIDUniversal.new());
}
static void process(void)
{	
	USBCore.poll();
}

const struct application Application = { 
	.init = init,		
	.process = process,		
};

