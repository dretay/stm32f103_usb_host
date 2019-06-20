#pragma once

#include "types_shortcuts.h"
#include "MAX3421E_registers.h"

#define MAX_CS_PIN		GPIO_PIN_4
#define MAX_CS_PORT		GPIOA
#define MAX_RST_PIN		GPIO_PIN_3
#define MAX_RST_PORT	GPIOA

#define START_SPI_TRANSACTION HAL_GPIO_WritePin(MAX_CS_PORT, MAX_CS_PIN, GPIO_PIN_RESET);
#define END_SPI_TRANSACTION HAL_GPIO_WritePin(MAX_CS_PORT, MAX_CS_PIN, GPIO_PIN_SET);
#define VBUS_ON write_register(rIOPINS2,(read_register(rIOPINS2)|0x08));				// H-GPO7
#define VBUS_OFF Hwreg(rIOPINS2,(Hrreg(rIOPINS2)& ~0x08));

struct max3421e {
	void(*init)(void);	
	void(*write_register)(u8 reg, u8 val);
	uint8_t(*read_register)(u8 reg);
	void(*write_bytes)(u8 reg, u8 N, u8 *p);
	void(*soft_reset)(void);
	void(*hard_reset)(void);
};

extern const struct max3421e MAX3421E;