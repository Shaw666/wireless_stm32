#include "drv_delay.h"

/**
  * @brief :延时初始化
  * @param :无
  * @note  :定时器默认初始化成us级计数
  * @retval:无
  */
void drv_delay_setup( void )
{
	TIM_TimeBaseInitTypeDef	 TimerInitStructer;
	RCC_ClocksTypeDef RCC_ClocksStatus;
	
	//获取系统时钟
	RCC_GetClocksFreq(&RCC_ClocksStatus);		//用于计算定时器分频系数
	
	//使能用作延时的TIMER时钟
	RCC_APB1PeriphClockCmd( DELAY_TIME_BASE_CLK, ENABLE );
	
	TimerInitStructer.TIM_ClockDivision = TIM_CKD_DIV1;
	TimerInitStructer.TIM_CounterMode = TIM_CounterMode_Up;
	TimerInitStructer.TIM_Period = 0x00FF;
	TimerInitStructer.TIM_RepetitionCounter = DISABLE;
	TimerInitStructer.TIM_Prescaler = ( RCC_ClocksStatus.PCLK2_Frequency / 1000000 ) - 1;	//初始化默认配置为1us精度
	TIM_TimeBaseInit( DELAY_TIME_BASE, &TimerInitStructer );
	
	TIM_ClearFlag( DELAY_TIME_BASE, TIM_FLAG_Update ); 
	TIM_SetCounter( DELAY_TIME_BASE, 0 );
	TIM_Cmd( DELAY_TIME_BASE, ENABLE );
	while( RESET == TIM_GetFlagStatus( DELAY_TIME_BASE, TIM_FLAG_Update ));
	TIM_Cmd( DELAY_TIME_BASE, DISABLE );
	TIM_SetCounter( DELAY_TIME_BASE, 0 );
	TIM_ClearFlag( DELAY_TIME_BASE, TIM_FLAG_Update ); 
}

void drv_delay_unblock_setup( void )
{
	TIM_TimeBaseInitTypeDef	 TimerInitStructer;
	RCC_ClocksTypeDef RCC_ClocksStatus;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	//获取系统时钟
	RCC_GetClocksFreq(&RCC_ClocksStatus);		//用于计算定时器分频系数
	
	//使能用作延时的TIMER时钟
	RCC_APB1PeriphClockCmd( DELAY_UNBLOCK_TIME_BASE_CLK, ENABLE );
	
	TimerInitStructer.TIM_ClockDivision = TIM_CKD_DIV1;
	TimerInitStructer.TIM_CounterMode = TIM_CounterMode_Up;
	TimerInitStructer.TIM_Period = 1000;
	TimerInitStructer.TIM_RepetitionCounter = ENABLE;
	TimerInitStructer.TIM_Prescaler = ( RCC_ClocksStatus.PCLK2_Frequency / 1000000 ) - 1;	//初始化默认配置为1us精度
	TIM_TimeBaseInit( DELAY_UNBLOCK_TIME_BASE, &TimerInitStructer );
	
	TIM_ITConfig(DELAY_UNBLOCK_TIME_BASE,TIM_IT_Update,ENABLE );
	
  NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;			//使能按键KEY2所在的外部中断通道
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x03;	//抢占优先级2， 
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x03;					//子优先级2
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;								//使能外部中断通道
  NVIC_Init(&NVIC_InitStructure);
}

void TIM3_IRQHandler(void)   //TIM3ÖÐ¶Ï
{
	if((TIM3->SR&TIM_IT_Update) != RESET){
		TIM3->SR = (uint16_t)~TIM_IT_Update;
         
        bTemp10Msec = 1;
		
        ++gTimer.Tick10Msec;
        if (0 == (gTimer.Tick10Msec % 5))
        {
            bTemp50Msec = 1;
        }
        
        if (0 == (gTimer.Tick10Msec % 10))
        {
            bTemp100Msec = 1;
        }
        
        if (100 == gTimer.Tick10Msec)
        {
            gTimer.Tick10Msec = 0;
            bTemp1Sec = 1;
        }
		}
}

/**
  * @brief :延时(us)
  * @param :
*			@Us:延时的us数
  * @note  :不超过65535
  * @retval:无
  */
void drv_delay_us( uint16_t Us )
{
	DELAY_TIME_BASE->ARR = Us;		//写如周期值 us
	DELAY_TIME_BASE->CNT = 0;		//计数器清零
	DELAY_TIME_BASE->CR1 |= (uint32_t)0x01;	//使能计数
	while( RESET == ( DELAY_TIME_BASE->SR & TIM_FLAG_Update ));		//等待溢出，溢出说明时间到
	DELAY_TIME_BASE->SR &= (uint32_t)( ~(uint32_t)TIM_FLAG_Update );//清溢出标志
	DELAY_TIME_BASE->CR1 &= (uint32_t)( ~(uint32_t)0x01 );			//停止计数
}

/**
  * @brief :延时(ms)
  * @param :
  *			@Ms:延时的Ms数
  * @note  :不超过65
  * @retval:无
  */
void drv_delay_ms( uint8_t Ms )
{
	DELAY_TIME_BASE->ARR = Ms * 1000;	//写如周期值 us * 1000 = Ms
	DELAY_TIME_BASE->CNT = 0;			//计数器清零
	DELAY_TIME_BASE->CR1 |= (uint32_t)0x01;	//使能计数
	while( RESET == ( DELAY_TIME_BASE->SR & TIM_FLAG_Update ));	//等待溢出，溢出说明时间到
	DELAY_TIME_BASE->SR &= (uint32_t)( ~(uint32_t)TIM_FLAG_Update );	//清溢出标志
	DELAY_TIME_BASE->CR1 &= (uint32_t)( ~(uint32_t)0x01 );	//停止计数
}

/**
  * @brief :延时(500Ms)
  * @param :
*			@Ms:延时的500Ms倍数
  * @note  :不超过255
  * @retval:无
  */
void drv_delay_500Ms( uint8_t Ms_500 )
{
	while( Ms_500 -- )
	{
		drv_delay_ms( 50 );		//1 * 50ms
		drv_delay_ms( 50 );		//2 * 50ms
		drv_delay_ms( 50 );		//3 * 50ms
		drv_delay_ms( 50 );		//4 * 50ms
		drv_delay_ms( 50 );		//5 * 50ms
		drv_delay_ms( 50 );		//6 * 50ms
		drv_delay_ms( 50 );		//7 * 50ms
		drv_delay_ms( 50 );		//8 * 50ms
		drv_delay_ms( 50 );		//9 * 50ms
		drv_delay_ms( 50 );		//10 * 50ms = 500ms
	}
}

/**
  * @brief :自由延时
  * @param :无
  * @note  :无
  * @retval:无
  */
void drv_delay_free( uint32_t Delay_Time )
{
	while( Delay_Time-- )
	{
			//
	}
}
