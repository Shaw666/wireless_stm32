#include "drv_uart.h"


#if 1
#pragma import(__use_no_semihosting)
//标准库需要的支持函数
struct __FILE {
    int handle;

};

FILE __stdout;
//定义_sys_exit()以避免使用半主机模式
void _sys_exit(int x) {
    x = x;
}
//重定义fputc函数
int fputc(int ch, FILE *f) {
    while((USART3->SR&0X40)==0) {;} //循环发送,直到发送完毕
    USART3->DR = (u8) ch;
    return ch;
}
#endif

void drv_uart_setup( uint32_t UartBaudRate ) {
    GPIO_InitTypeDef	UartGpioInitStructer;
    USART_InitTypeDef	UartinitStructer;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2,DISABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_AFIO
                           |RCC_APB2Periph_USART1, ENABLE );	//打开TX RX 端口时钟

    UartGpioInitStructer.GPIO_Mode = GPIO_Mode_AF_PP;
    UartGpioInitStructer.GPIO_Speed = GPIO_Speed_2MHz;
    //TX
    UartGpioInitStructer.GPIO_Pin = GPIO_Pin_9;
    GPIO_Init( GPIOA, &UartGpioInitStructer );		//初始化TX引脚  配置为复用功能
    UartGpioInitStructer.GPIO_Pin = GPIO_Pin_10;
    GPIO_Init( GPIOB, &UartGpioInitStructer );		//初始化TX引脚  配置为复用功能
    //RX
    UartGpioInitStructer.GPIO_Mode = GPIO_Mode_IN_FLOATING;//GPIO_Mode_AF_PP;//GPIO_Mode_AF_OD;//GPIO_Mode_AIN;//GPIO_Mode_IN_FLOATING;
    UartGpioInitStructer.GPIO_Pin = GPIO_Pin_10;
    GPIO_Init( GPIOA, &UartGpioInitStructer );		//初始化RX引脚  配置为输入

    UartGpioInitStructer.GPIO_Pin = GPIO_Pin_11;
    GPIO_Init( GPIOB, &UartGpioInitStructer );		//初始化RX引脚  配置为输入

//	GPIO_PinRemapConfig(GPIO_FullRemap_USART3, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);
    //配置USART外设
    USART_DeInit( USART1 );		//外设复位
    USART_DeInit(USART3);

    UartinitStructer.USART_BaudRate = UartBaudRate;						//设置波特率
    UartinitStructer.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	//不使用流控制
    UartinitStructer.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;		//发送和接收
    UartinitStructer.USART_Parity = USART_Parity_No;					//不带校验
    UartinitStructer.USART_StopBits = USART_StopBits_1;					//一个停止位
    UartinitStructer.USART_WordLength = USART_WordLength_8b;			//8个数据位

    USART_Cmd( USART1, DISABLE );									//失能外设
    USART_Init( USART1, &UartinitStructer );							//初始化外设
    USART_Cmd( USART1, ENABLE );										//使能外设

    USART_Cmd( USART3, DISABLE );									//失能外设
    USART_Init( USART3, &UartinitStructer );							//初始化外设
    USART_Cmd( USART3, ENABLE );										//使能外设

#if DEBUG == 1
    command_handle.serial_debug = 0;
#endif
}

void debug_tx_bytes( uint8_t* TxBuffer, uint8_t Length ) {
    while( Length-- ) {
//		printf("%x ",*TxBuffer);
        while( RESET == USART_GetFlagStatus( USART3, USART_FLAG_TXE ));
        USART3->DR = *TxBuffer;
        TxBuffer++;
    }
    printf("\n");
}

void debug_tx_bytes_printf( uint8_t* TxBuffer, uint8_t Length ) {
    while( Length-- ) {
        printf("%x ",*TxBuffer);
        TxBuffer++;
    }
    printf("\n");
}

void drv_uart_tx_bytes( uint8_t* TxBuffer, uint8_t Length ) {
    while( Length-- ) {
        while( RESET == USART_GetFlagStatus( USART1, USART_FLAG_TXE ));
        USART1->DR = *TxBuffer;
        TxBuffer++;
    }
}


/**
  * @brief :串口接收数据
  * @param :
  *			@RxBuffer:发送数据首地址
  * @note  :无
  * @retval:接收到的字节个数
  */
uint8_t drv_uart_rx_bytes( uint8_t* RxBuffer ) {
    uint8_t l_RxLength = 0;
    uint16_t l_UartRxTimOut = 0x7FFF;

    while( l_UartRxTimOut-- ) {		//等待查询串口数据
        if( RESET != USART_GetFlagStatus( USART1, USART_FLAG_RXNE )) {
            *RxBuffer = (uint8_t)USART1->DR;
            RxBuffer++;
            l_RxLength++;
            l_UartRxTimOut = 0x7FFF;	//接收到一个字符，回复等待时间
        }
        if( 64 == l_RxLength ) {
            break;		//不能超过64个字节
        }
    }

    return l_RxLength;					//等待超时，数据接收完成
}

uint8_t rec_ptr=0;
unsigned char cmd_buf[32] = {0x00};

void serial_cmd_Loop(void) {
    uint8_t rec_data;

#if DEBUG == 1
    if( RESET != USART_GetFlagStatus( USART3, USART_FLAG_RXNE )) {
        rec_data = (uint8_t)USART3->DR;
        cmd_buf[rec_ptr] = rec_data;
        while( RESET == USART_GetFlagStatus( USART3, USART_FLAG_TXE ));
        USART3->DR = rec_data;
        rec_ptr++;

        if (rec_data == 0x0A) {
            rec_ptr = 0;
            command_handle.serial_debug = 1;
        }
    }
#endif

#if LOCK == 1
    if( RESET != USART_GetFlagStatus( USART1, USART_FLAG_RXNE )) {
            rec_data = (uint8_t)USART1->DR;
            cmd_buf[rec_ptr] = rec_data;
            while( RESET == USART_GetFlagStatus( USART3, USART_FLAG_TXE ));
            USART3->DR = rec_data;
            rec_ptr++;
            if((command_handle.op_fsm == SERIAL_READY)
                    ||((rec_data == 0x00))) {
//            command_handle.op_fsm = SERIAL_READY;
                rec_ptr = 0;
            }
            if((rec_data == 0xAA)&&((rec_ptr/3) == (cmd_buf[2]+4))) {

                rec_ptr = 0;
                command_handle.op_fsm = SERIAL_RECEIVE;
            }
        }
#else
        if( RESET != USART_GetFlagStatus( USART1, USART_FLAG_RXNE )) {
            rec_data = (uint8_t)USART1->DR;
            cmd_buf[rec_ptr] = rec_data;
            while( RESET == USART_GetFlagStatus( USART3, USART_FLAG_TXE ));
            USART3->DR = rec_data;
            rec_ptr++;
            if((command_handle.op_fsm != SERIAL_READY)
                    ||(rec_data == 0xAA)) {
                cmd_buf[1] = 0x00;
                command_handle.op_fsm = SERIAL_READY;
                rec_ptr = 0;
            }
            if(cmd_buf[1] == rec_ptr-2) {
                command_handle.op_fsm = SERIAL_RECEIVE;
            }
        }

#endif
}
