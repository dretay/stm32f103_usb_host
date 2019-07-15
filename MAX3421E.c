#include "MAX3421E.h"

extern SPI_HandleTypeDef hspi1;

static void write_register(u8 reg, u8 val) 
{	
	uint8_t out[2];
	out[0] = reg | 0x02;
	out[1] = val;
	START_SPI_TRANSACTION;
	HAL_SPI_Transmit(&hspi1, out, 2, HAL_MAX_DELAY);
	END_SPI_TRANSACTION;		
}

static u8 read_register(u8 reg) 
{	
	uint8_t out[2], in[2];
	out[0] = reg;
	out[1] = 0;
	START_SPI_TRANSACTION;
	HAL_SPI_TransmitReceive(&hspi1, out, in, 2, HAL_MAX_DELAY);
	END_SPI_TRANSACTION;	
	return in[1];
}
static void read_bytes(u8 reg, u8 bytes, u8 *p) 
{	
	uint8_t out[2];
	out[0] = reg;
	out[1] = 0;
	START_SPI_TRANSACTION;
	HAL_SPI_TransmitReceive(&hspi1, out, p, bytes, HAL_MAX_DELAY);
	END_SPI_TRANSACTION;		
}

static void write_bytes(u8 reg, u8 N, u8 *p)
{	
	uint8_t out[9];
	out[0] = reg + 0x02;   			// command byte into the FIFO. 0x0002 is the write bit
	for(int j = 0 ; j < N ; j++)
	{
		out[j + 1] = *p++;   				// send the next data byte
	}
	START_SPI_TRANSACTION;
	HAL_SPI_Transmit(&hspi1, out, N + 1, HAL_MAX_DELAY);
	END_SPI_TRANSACTION;			
}

static void soft_reset(void)
{
	// chip reset This stops the oscillator
	write_register(rUSBCTL, bmCHIPRES);   

	// remove the reset
	write_register(rUSBCTL, 0x00);

	// hang until the PLL stabilizes
	while(!(read_register(rUSBIRQ) & bmOSCOKIRQ));   
}
static void hard_reset(void)
{
	HAL_GPIO_WritePin(MAX_CS_PORT, MAX_RST_PIN, GPIO_PIN_RESET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(MAX_CS_PORT, MAX_RST_PIN, GPIO_PIN_SET);   
}
static void clear_conn_detect_irq(void)
{
	write_register(rHIRQ, bmCONDETIRQ);   //clear connection detect interrupt
}
static void enable_irq(void)
{
	write_register(rCPUCTL, 0x01);   //enable interrupt pin
}
static void reset_bus(void)
{
	// Let the bus settle
    HAL_Delay(200);

	//issue bus reset
	write_register(rHCTL, bmBUSRST); 
        
	// Wait for a response
	while((read_register(rHCTL) & bmBUSRST) != 0); 
	u8 tmpdata = read_register(rMODE) | bmSOFKAENAB;  //start SOF generation
	write_register(rMODE, tmpdata);

	// when first SOF received _and_ 20ms has passed we can continue
	while((read_register(rHIRQ) & bmFRAMEIRQ) == 0);
	HAL_Delay(3);
}

static void init(void)
{
	hard_reset();

	write_register(rPINCTL, bmFDUPSPI | bmPOSINT);   // Full duplex SPI, edge-active, rising edges	
	soft_reset();
	write_register(rMODE, bmDPPULLDN | bmDMPULLDN | bmHOST); // set pull-downs, Host
	write_register(rHIEN, bmCONDETIE | bmFRAMEIE); //connection detection
	write_register(rHCTL, bmSAMPLEBUS);  // sample USB bus
    while(!(read_register(rHCTL) & bmSAMPLEBUS));  //wait for sample operation to finish
	VBUS_OFF; 		// and Vbus OFF (in case something already plugged in)
	HAL_Delay(200);
	VBUS_ON;
	
}

const struct max3421e MAX3421E = { 
	.init = init,		
	.write_register = write_register,
	.read_register = read_register,
	.write_bytes = write_bytes,
	.read_bytes = read_bytes,
	.soft_reset = soft_reset,
	.hard_reset = hard_reset,
	.clear_conn_detect_irq = clear_conn_detect_irq,
	.enable_irq = enable_irq,
	.reset_bus = reset_bus,
};

