#pragma once

//simple debug logging
#include "SEGGER_RTT.h"
#include "stm32f1xx_hal.h"
#include <string.h>



void printfUART(const char *fmt, ...);

#define LOG(...) printfUART(__VA_ARGS__)

#define _ERROR(fmt, args...) LOG("\r\n%s%s[ERROR] %s%s:%s:%d: "fmt, RTT_CTRL_RESET, RTT_CTRL_BG_BRIGHT_RED, RTT_CTRL_RESET, __FILE__, __FUNCTION__, __LINE__, args)
#define __ERROR(...) printfUART(__VA_ARGS__)

#define _WARN(fmt, args...) LOG("\r\n%s%s[WARN] %s%s:%s:%d: "fmt, RTT_CTRL_RESET, RTT_CTRL_BG_BRIGHT_YELLOW, RTT_CTRL_RESET, __FILE__, __FUNCTION__, __LINE__, args)
#define __WARN(...) printfUART(__VA_ARGS__)


#define _INFO(fmt, args...) LOG("\r\n%s[INFO] %s:%s:%d: "fmt, RTT_CTRL_RESET, __FILE__, __FUNCTION__, __LINE__, args)
#define __INFO(...) printfUART(__VA_ARGS__)


#define _DEBUG(fmt, args...) LOG("\r\n%s[DEBUG] %s:%s:%d: "fmt, RTT_CTRL_RESET, __FILE__, __FUNCTION__, __LINE__, args)
#define __DEBUG(...) printfUART(__VA_ARGS__)



