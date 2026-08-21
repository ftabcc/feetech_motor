#ifndef _ESP_SYS_H
#define _ESP_SYS_H


#include <stdint.h>

typedef int8_t 
typedef uint8_t
typedef int16_t
typedef uint16_t
typedef int32_t
typedef uint32_t

typedef	char s8;
typedef	unsigned char u8;	
typedef	unsigned short u16;	
typedef	short s16;
typedef	unsigned long u32;	
typedef	long s32;

class ESP{
public:
	
protected:

private:
    TaskHandle_t native_usb_rx_task_handle = nullptr;
};
#endif
