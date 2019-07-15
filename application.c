#include "application.h"
#include "hiduniversal.h"

static void init(void)
{	
	USBCORE.init(HIDUniversal.new());
}
static void process(void)
{	
	USBCORE.poll();
}

const struct application Application = { 
	.init = init,		
	.process = process,		
};

