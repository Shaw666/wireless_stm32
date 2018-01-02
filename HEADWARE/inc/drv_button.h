#ifndef __DRV_BUTTON_H__
#define __DRV_BUTTON_H__

#include "main.h"

//按键硬件定义 KEY0
#define BUTOTN_GPIO_PORT			GPIOE									
#define BUTTON_GPIO_CLK				RCC_APB2Periph_GPIOE
#define BUTTON_GPIO_PIN				GPIO_Pin_4

extern uint8_t key_value;
/** 按键状态定义 */
enum
{
	BUTOTN_UP = 0,		//按键未按下
	BUTOTN_PRESS_DOWN	//按键按下
};

#ifdef __cplusplus
extern "C" {
#endif

void drv_button_init( void );
void drv_button_check(uint8_t mode);

#ifdef __cplusplus
}
#endif



#endif

