#include "drv_led.h"



/**
  * @brief :LED初始化
  * @param :无
  * @note  :无
  * @retval:无
  */ 
void drv_led_setup( void )
{
	GPIO_InitTypeDef	GpioInitStructer;
	
	//使能口线时钟
	RCC_APB2PeriphClockCmd( LED_RED_GPIO_CLK | LED_BLUE_GPIO_CLK, ENABLE );	//打开端口时钟
	
	GpioInitStructer.GPIO_Mode = GPIO_Mode_Out_PP;						
	GpioInitStructer.GPIO_Speed = GPIO_Speed_2MHz;		
	
	GpioInitStructer.GPIO_Pin = LED_RED_GPIO_PIN;		
	GPIO_Init( LED_RED_GPIO_PORT, &GpioInitStructer );			//初始化红色LED引脚
	GPIO_SetBits( LED_RED_GPIO_PORT, LED_RED_GPIO_PIN );		//初始状态置低，红色LED初始化状态默认为灭
	
	GpioInitStructer.GPIO_Pin = LED_BLUE_GPIO_PIN;		
	GPIO_Init( LED_BLUE_GPIO_PORT, &GpioInitStructer );			//初始化蓝色LED引脚
	GPIO_SetBits( LED_BLUE_GPIO_PORT, LED_BLUE_GPIO_PIN );	//初始状态置低，蓝色LED初始化状态默认为灭
	
}

/**
  * @brief :LED亮
  * @param :
  *			@LedPort:LED选择，红色或蓝色
  * @note  :无
  * @retval:无
  */
void drv_led_off( LedPortType LedPort )
{
	if( LED_RED == LedPort )	//LED_RED
	{
		GPIO_ResetBits( LED_RED_GPIO_PORT, LED_RED_GPIO_PIN );		//红色LED引脚置低，红色LED亮
	}
	else						//LED_BLUE
	{
		GPIO_ResetBits( LED_BLUE_GPIO_PORT, LED_BLUE_GPIO_PIN );	//蓝色LED引脚置低，蓝色LED亮
	}
	
}

/**
  * @brief :LED灭
  * @param :
  *			@LedPort:LED选择，红色或蓝色
  * @note  :无
  * @retval:无
  */
void drv_led_on( LedPortType LedPort )
{
	if( LED_RED == LedPort )	//LED_RED
	{
		GPIO_SetBits( LED_RED_GPIO_PORT, LED_RED_GPIO_PIN );		//红色LED引脚置高，红色LED灭
	}
	else						//LED_BLUE
	{
		GPIO_SetBits( LED_BLUE_GPIO_PORT, LED_BLUE_GPIO_PIN );		//蓝色LED引脚置高，蓝色LED灭
	}
	
}

/**
  * @brief :LED闪烁
  * @param :
  *			@LedPort:LED选择，红色或蓝色
  * @note  :无
  * @retval:无
  */
void drv_led_flashing( LedPortType LedPort )
{
	
	if( LED_RED == LedPort )
	{
		LED_RED_GPIO_PORT->ODR ^= ( uint32_t)LED_RED_GPIO_PIN;
	}
	else
	{
		LED_BLUE_GPIO_PORT->ODR ^= ( uint32_t)LED_BLUE_GPIO_PIN;
	}
}



void EXTIX_Init(void)
{
 
 	EXTI_InitTypeDef EXTI_InitStructure;
 	NVIC_InitTypeDef NVIC_InitStructure;


  	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);	//使能复用功能时钟

    //GPIOE.2 中断线以及中断初始化配置   下降沿触发
  	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource2);

  	EXTI_InitStructure.EXTI_Line=EXTI_Line2;	//KEY2
  	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
  	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);	 	//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器



    NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;			//使能按键KEY2所在的外部中断通道
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;	//抢占优先级2， 
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;					//子优先级2
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;								//使能外部中断通道
  	NVIC_Init(&NVIC_InitStructure);
		
		EXTI->IMR|=1<<2;
}
