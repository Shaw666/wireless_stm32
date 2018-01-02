#include "main.h"
#include "drv_SI446x.h"
#include "drv_button.h"
#include "drv_delay.h"
#include "drv_led.h"
#include "drv_uart.h"
#include "data_process.h"

SI446X si4463;
uint32_t system_count = 0;

void setup() {
    uint8_t i;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    drv_uart_setup( 9600 );
    drv_delay_setup( );
    drv_led_setup( );
    si4463.SI446x_setup( );
    si4463_data_process.process_setup(0);
    printf("init ok\n");
    printf("save machine num:%d\r\n", save_machine_num[0]);

    for(i=0; i<MACHINE_NUM_MAX; i++) {
        if((machine_info[i].machine.identify_SN != 0)&&(machine_info[i].machine.status < 3)) {
            printf("position:%d \n", i);
            printf("type:%x\r\n",machine_info[i].machine.name.machine_type);
					  printf("sort:%x\r\n",machine_info[i].machine.name.machine_sort);
					  printf("detail:%x\r\n",machine_info[i].machine.name.machine_detail);
            printf("status:%x\r\n",machine_info[i].machine.status);
            printf("SN:%x\r\n",machine_info[i].machine.identify_SN);
        }
    }
}

void loop() {
//    drv_button_check(0);
    si4463_data_process.process_loop();
    si4463_data_process.process_command();
}

int main( void ) {
    setup();
    while(1) {
        loop();
    }
}
