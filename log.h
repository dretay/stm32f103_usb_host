#pragma once

//simple debug logging
#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdarg.h>

void printfUART(const char *fmt, ...);

#define LOG(...) printfUART(__VA_ARGS__)

#define _ERROR(fmt, args...) LOG("\r\n[ERROR] %s%s:%s:%d: "fmt, __FILE__, __FUNCTION__, __LINE__, args)
#define __ERROR(...) printfUART(__VA_ARGS__)

#define _WARN(fmt, args...) LOG("\r\n[WARN] %s%s:%s:%d: "fmt, __FILE__, __FUNCTION__, __LINE__, args)
#define __WARN(...) printfUART(__VA_ARGS__)


#define _INFO(fmt, args...) LOG("\r\ns[INFO] %s:%s:%d: "fmt, __FILE__, __FUNCTION__, __LINE__, args)
#define __INFO(...) printfUART(__VA_ARGS__)


#define _DEBUG(fmt, args...) LOG("\r\n[DEBUG] %s:%s:%d: "fmt, __FILE__, __FUNCTION__, __LINE__, args)
#define __DEBUG(...) printfUART(__VA_ARGS__)



