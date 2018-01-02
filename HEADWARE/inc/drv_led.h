#ifndef	__DRV_LED_H__
#define __DRV_LED_H__

#include "main.h"

//LED硬件定义 
#define LED_RED_GPIO_PORT			GPIOB								
#define LED_RED_GPIO_CLK			RCC_APB2Periph_GPIOB
#define LED_RED_GPIO_PIN			GPIO_Pin_12

#define LED_BLUE_GPIO_PORT		GPIOC					
#define LED_BLUE_GPIO_CLK			RCC_APB2Periph_GPIOC
#define LED_BLUE_GPIO_PIN			GPIO_Pin_13


/** LED定义 */
typedef enum LedPort
{
	LED_RED = 0,		//红色LED
	LED_GREEN			//绿色LED
}LedPortType;
	
//红色LED操作函数
#define led_red_on( )				drv_led_on( LED_RED )
#define led_red_off( )				drv_led_off( LED_RED )
#define led_red_flashing( )			drv_led_flashing( LED_RED )
//蓝色LED操作函数
#define led_green_on( )				drv_led_on( LED_GREEN )
#define led_green_off( )			drv_led_off( LED_GREEN )
#define led_green_flashing( )		drv_led_flashing( LED_GREEN )

#ifdef __cplusplus
extern "C" {
#endif
void drv_led_setup( void );
void drv_led_on( LedPortType LedPort );
void drv_led_off( LedPortType LedPort );
void drv_led_flashing( LedPortType LedPort );
void EXTIX_Init(void);

#ifdef __cplusplus
}
#endif

#endif

