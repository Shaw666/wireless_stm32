#ifndef __DATA_PROCESS_H
#define __DATA_PROCESS_H

#include "main.h"
#include "drv_SI446x.h"
#include "drv_uart.h"
#include "stm32_flash.h"

class WIRELESS_DATA_PROCESS{
	private:
		uint8_t work_fail_counter;
		uint8_t is_exist_machine;
		uint8_t op_position;
		uint8_t current_CCA_rssi;
		uint16_t delay_ms_counter;
		unsigned short crc16(const void *data, unsigned short sizeInByte);
		uint8_t calc_check_sum(uint8_t* data, uint8_t len);
		uint16_t wait_wireless_ack(uint16_t wait_ms_out, uint32_t wait_SN);
		uint16_t wait_wireless_ack(uint16_t wait_ms_out);
		void login_save_machine_info(void);
	
	public:
		void process_setup(uint8_t protocol_len);
		void process_loop(void);
		uint8_t process_receive(void);
		void process_transfer(void);
		uint8_t check_machine(void);
		uint8_t check_machine(uint32_t machine_SN);
		uint8_t host_login(void);
		uint8_t host_login(uint32_t device_SN);
		uint8_t host_logout(void);
		uint8_t device_login(void);
		uint8_t device_logout(void);
		uint8_t test_device_process(void);
		void function_process(void);
		void cryption_data_process(uint8_t* pending_data, uint8_t pending_len, uint8_t* return_data);
		void process_response(uint8_t op);
		void systimer_loop(void);
		void process_command(void);
		void set_machine_info(void);
		void test_transfer(void);
	
		void host_add_device(void);
		void test_smart_lock_process(void);
		uint16_t wait_serial_ack(uint16_t wait_ms_out);
		void clear_save_machine_data(void);
		void check_machine_info(void);

};

extern WIRELESS_DATA_PROCESS si4463_data_process;

#define MACHINE_NUM_MAX	32

typedef enum{
	HOST = 0,
	DEVICE = 1,
}Machine_type;

typedef enum{
	NORMAL = 0,
	SIGNIN,
	SIGNOUT,
	
}Machine_status;

typedef enum{
	RESPONSE_SIGNIN = 0,
	RESPONSE_SIGNOUT,
	RESPONSE_OK,
	
}Machine_op_status;

//#pragma pack(1)
//typedef struct {
//	Machine_type type;	//0 host 1 device					1
//	Machine_status status; //host -> 0 normal 1 sign up  	1
//	uint32_t identify_SN;	//								4
//	uint8_t identify_KEY[10];	//							10
//	uint8_t data[txrx_max];		//						
//	
//}WIRELESS_MACHINE;

//extern WIRELESS_MACHINE si4463_machine[MACHINE_NUM_MAX];
extern unsigned short save_machine_num[1];


typedef struct{
	unsigned machine_type		:1;
	unsigned machine_sort		:3;
	unsigned machine_detail	:12;
}Machine_name;

typedef struct{
	Machine_name name; //2
	uint8_t vendor_id; //1
	Machine_status status; //1
	uint32_t identify_SN; //4
	uint8_t identify_KEY[10]; //10
}Machine;

typedef struct {
	Machine machine;
	uint8_t data[txrx_max];
}MACHINE_INFO;

extern MACHINE_INFO machine_info[MACHINE_NUM_MAX];

//WIRELESS_DATA_PROCESS si4463_data_process;































#endif
