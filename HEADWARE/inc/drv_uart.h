#ifndef __DRV_UART_H__
#define __DRV_UART_H__
#include "main.h"
#include "drv_delay.h"

#define MACHINE_HOST 0
#define DEBUG 1
#define LOCK 0

extern unsigned char cmd_buf[32];

typedef enum{
	SERIAL_IDLE = 0,
	SERIAL_READY,
	SERIAL_RECEIVE,
	
}Serial_port_op_status;

#pragma pack(1)
typedef struct{
	Serial_port_op_status op_fsm;
	uint8_t serial_debug;
	
}Command_handle;

extern Command_handle command_handle;


#ifdef __cplusplus
extern "C" {
#endif

void drv_uart_setup( uint32_t UartBaudRate );
void drv_uart_tx_bytes( uint8_t* TxBuffer, uint8_t Length );
uint8_t drv_uart_rx_bytes( uint8_t* RxBuffer );

void debug_tx_bytes( uint8_t* TxBuffer, uint8_t Length );
void debug_tx_bytes_printf( uint8_t* TxBuffer, uint8_t Length );
	
void serial_cmd_Loop(void);
void serial_cmd_process(void) ;

#ifdef __cplusplus
}
#endif


#endif



