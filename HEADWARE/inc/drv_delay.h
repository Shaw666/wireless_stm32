#ifndef __DRV_DELAY_H__
#define __DRV_DELAY_H__

#include "main.h"


//延时硬件定义
#define DELAY_TIME_BASE					TIM2
#define DELAY_TIME_BASE_CLK				RCC_APB1Periph_TIM2

#define DELAY_UNBLOCK_TIME_BASE						TIM3
#define DELAY_UNBLOCK_TIME_BASE_CLK				RCC_APB1Periph_TIM3


typedef struct
{
    unsigned char bit0: 1;
    unsigned char bit1: 1;
    unsigned char bit2: 1;
    unsigned char bit3: 1;
    unsigned char bit4: 1;
    unsigned char bit5: 1;
    unsigned char bit6: 1;
    unsigned char bit7: 1;
} Timer_Bit;

typedef union
{
    unsigned char byte;
    Timer_Bit field;
} Char_Field;

typedef struct
{
    uint8_t Tick10Msec;
    Char_Field Status;
} Timer_Struct;

extern Timer_Struct gTimer;

#define bSystem10Msec        gTimer.Status.field.bit0
#define bSystem50Msec        gTimer.Status.field.bit1
#define bSystem100Msec       gTimer.Status.field.bit2
#define bSystem1Sec          gTimer.Status.field.bit3
#define bTemp10Msec          gTimer.Status.field.bit4
#define bTemp50Msec          gTimer.Status.field.bit5
#define bTemp100Msec         gTimer.Status.field.bit6
#define bTemp1Sec            gTimer.Status.field.bit7




#ifdef __cplusplus
extern "C" {
#endif
void drv_delay_setup( void );
void drv_delay_unblock_setup( void );
void drv_delay_us( uint16_t Us );
void drv_delay_ms( uint8_t Ms );
void drv_delay_500Ms( uint8_t Ms_500 );
void drv_delay_free( uint32_t Delay_Time );


#ifdef __cplusplus
}
#endif


#endif

