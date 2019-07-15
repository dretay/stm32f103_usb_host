#include "log.h"

extern UART_HandleTypeDef huart1;

//from: https://electronics.stackexchange.com/questions/206113/how-do-i-use-the-printf-function-on-stm32
static void vprint(const char *fmt, va_list argp)
{
	char string[200];
	if (0 < vsprintf(string, fmt, argp)) // build string
		{
			HAL_UART_Transmit(&huart1, (uint8_t*)string, strlen(string), 0xffffff);     // send message via UART
		}
}

void printfUART(const char *fmt, ...) // custom printf() function
{
	va_list argp;
	va_start(argp, fmt);
	vprint(fmt, argp);
	va_end(argp);
}