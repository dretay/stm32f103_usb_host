#include "MAX3421E.h"

extern SPI_HandleTypeDef hspi1;

static void write_register(uint8_t reg, uint8_t val) 
{	
	uint8_t out[2];
	out[0] = reg | 0x02;
	out[1] = val;
	START_SPI_TRANSACTION;
	HAL_SPI_Transmit(&hspi1, out, 2, HAL_MAX_DELAY);
	END_SPI_TRANSACTION;		
}

static uint8_t read_register(uint8_t reg) 
{	
	uint8_t out[2], in[2];
	out[0] = reg;
	out[1] = 0;
	START_SPI_TRANSACTION;
	HAL_SPI_TransmitReceive(&hspi1, out, in, 2, HAL_MAX_DELAY);
	END_SPI_TRANSACTION;	
	return in[1];
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

static void init(void)
{
	hard_reset();

	uint8_t out[2];
	out[0] = (17 << 3) | 2;
	out[1] = 0x10;
	START_SPI_TRANSACTION;
	HAL_SPI_Transmit(&hspi1, out, 2, HAL_MAX_DELAY);
	END_SPI_TRANSACTION;			
	
	write_register(rUSBIEN, bmOSCOKIE);
	read_register(rUSBIEN);
	read_register(18 << 3);
	soft_reset();
	write_register(rIOPINS1, 0x00);   		// seven-segs off
	write_register(rIOPINS2, 0x00);   		// and Vbus OFF (in case something already plugged in)
	HAL_Delay(200);
	VBUS_ON;
}

const struct max3421e MAX3421E = { 
	.init = init,		
	.write_register = write_register,
	.read_register = read_register,
	.write_bytes = write_bytes,
	.soft_reset = soft_reset,
	.hard_reset = hard_reset
};

