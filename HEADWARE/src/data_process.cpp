#include "data_process.h"

//WIRELESS_MACHINE machine_info[MACHINE_NUM_MAX];
MACHINE_INFO machine_info[MACHINE_NUM_MAX];

WIRELESS_DATA_TXRX si4463_txrx;
WIRELESS_DATA_PROCESS si4463_data_process;
Timer_Struct gTimer;
Command_handle command_handle;

unsigned short save_machine_num[1];
uint8_t key_value = 0;

void WIRELESS_DATA_PROCESS::systimer_loop(void) {
    gTimer.Status.byte &= 0xF0;

    if (bTemp10Msec) {
        bSystem10Msec = 1;
    }

    if (bTemp50Msec) {
        bSystem50Msec = 1;
    }

    if (bTemp100Msec) {
        bSystem100Msec = 1;
    }

    if (bTemp1Sec) {
        bSystem1Sec = 1;
    }

    gTimer.Status.byte &= 0x0F;
}

void WIRELESS_DATA_PROCESS::process_setup(uint8_t protocol_len) {
    uint8_t i;

    //在这个指针数组中，第0个元素作为本机器
    memset(machine_info, 0, sizeof(machine_info));
#if	MACHINE_HOST == 1
    machine_info[0].machine.name.machine_type = HOST;
    machine_info[0].machine.identify_SN = 0x00000001;
#else
    machine_info[0].machine.name.machine_type = DEVICE;
    machine_info[0].machine.identify_SN = 0x10000001;
#endif
    for (i = 0; i < 10; i++) {
        machine_info[0].machine.identify_KEY[i] = i;
    }
//		save_machine_num[0] = 0;
//		STMFLASH_Write(FLASH_SAVE_ADDR, (u16*)save_machine_num, 1);

    save_machine_num[0] = STMFLASH_ReadHalfWord(FLASH_SAVE_ADDR);

    if((save_machine_num[0] > MACHINE_NUM_MAX)||(save_machine_num[0] < 1)) {
        printf("no save machine\n");
        machine_info[0].machine.status = NORMAL;
        save_machine_num[0] = 1;
        STMFLASH_Write(FLASH_SAVE_ADDR, (u16*)save_machine_num, 1);
        for(i=0; i<MACHINE_NUM_MAX; i++) {
            STMFLASH_Write(FLASH_SAVE_ADDR+(i*18)+2,(u16*)&machine_info[i].machine,9);
        }
        printf("system data init\n");
    } else {
        for(i=0; i<MACHINE_NUM_MAX; i++) {
            STMFLASH_Read(FLASH_SAVE_ADDR+(i*18)+2,(u16*)&machine_info[i].machine,9);
        }
    }

    command_handle.op_fsm = SERIAL_IDLE;
    si4463_data_process.work_fail_counter = 0;
}
//接收无线数据处理的循环
void WIRELESS_DATA_PROCESS::process_loop(void) {
    si4463.read_IRQ();
    if(si4463_txrx.rxtx_status == 0) {
        si4463_txrx.rxtx_status = 0xFF;
//        EXTI->IMR|=EXTI_Line2;
        process_receive();
    }
}

unsigned short WIRELESS_DATA_PROCESS::crc16(const void *data, unsigned short sizeInByte) {

    unsigned short crc = 0;
    unsigned char *p = (unsigned char *) data;

    while (sizeInByte--) {
        unsigned char i;
        crc ^= *p++;
        for (i = 0; i < 8; i++) {
            crc = (crc >> 1) ^ ((crc & 0x01) ? 0xA001 : 0);
        }
    }
    return crc;
}

void WIRELESS_DATA_PROCESS::cryption_data_process(uint8_t* pending_data, uint8_t pending_len, uint8_t* return_data) {
    uint8_t i=0;
    while(pending_len--) {
        *return_data = machine_info[si4463_data_process.op_position].machine.identify_KEY[i]
                       ^ (*pending_data);
        pending_data++;
        return_data++;
        i++;
        if(i == 10) i=0;
    }
}

uint8_t WIRELESS_DATA_PROCESS::check_machine(void) {
    uint8_t i,empty_machine_min_position = 0,found_machine_position = 0;
    uint32_t receive_identify_SN = 0;
    si4463_data_process.is_exist_machine = 0;

    for(i=MACHINE_NUM_MAX-1; i>0; i--) {
        receive_identify_SN = (si4463_txrx.rx_fifo[4]<<24)
                              + (si4463_txrx.rx_fifo[5]<<16)
                              + (si4463_txrx.rx_fifo[6]<<8)
                              + (si4463_txrx.rx_fifo[7]);

        if(machine_info[i].machine.identify_SN == 0) {
            empty_machine_min_position = i;
            si4463_data_process.is_exist_machine = 0;
            continue;
        } else {
            if(machine_info[i].machine.identify_SN == receive_identify_SN) {
                found_machine_position = i;
                si4463_data_process.is_exist_machine = 1;
                printf("Mchine is exist,SN:%x position:%d\n",machine_info[found_machine_position].machine.identify_SN, found_machine_position);
                return found_machine_position;
            }
        }
    }
    printf("Mchine is not exist,min_empty_machine_position: %d\n",empty_machine_min_position);
    return empty_machine_min_position;
}

uint8_t WIRELESS_DATA_PROCESS::check_machine(uint32_t machine_SN) {
    uint8_t i,empty_machine_min_position = 0,found_machine_position = 0;
    uint32_t receive_identify_SN = 0;
    si4463_data_process.is_exist_machine = 0;

    for(i=MACHINE_NUM_MAX-1; i>0; i--) {
        receive_identify_SN = machine_SN;

        if(machine_info[i].machine.identify_SN == 0) {
            empty_machine_min_position = i;
            si4463_data_process.is_exist_machine = 0;
            continue;
        } else {
            if(machine_info[i].machine.identify_SN == receive_identify_SN) {
                found_machine_position = i;
                si4463_data_process.is_exist_machine = 1;
                printf("Mchine is exist,SN:%x position:%d\n",machine_info[found_machine_position].machine.identify_SN, found_machine_position);
                return found_machine_position;
            }
        }
    }
    printf("Mchine is not exist,min_empty_machine_position: %d\n",empty_machine_min_position);
    return empty_machine_min_position;
}

uint16_t WIRELESS_DATA_PROCESS::wait_wireless_ack(uint16_t wait_ms_out, uint32_t wait_SN) {
    uint32_t receive_identufy_SN = 0;
    while(1) {
        si4463.read_IRQ();
        if(si4463_txrx.rxtx_status == 0) {
            //目的地址校验
            receive_identufy_SN = (si4463_txrx.rx_fifo[0]<<24)
                                  +(si4463_txrx.rx_fifo[1]<<16)
                                  +(si4463_txrx.rx_fifo[2]<<8)
                                  +(si4463_txrx.rx_fifo[3]);
            if(receive_identufy_SN == machine_info[0].machine.identify_SN) {
                //来源地址校验
                receive_identufy_SN = (si4463_txrx.rx_fifo[4]<<24)
                                      +(si4463_txrx.rx_fifo[5]<<16)
                                      +(si4463_txrx.rx_fifo[6]<<8)
                                      +(si4463_txrx.rx_fifo[7]);
                if(receive_identufy_SN == wait_SN) {
                    printf("it is my wait from SN:%x ,ACK OK\n",wait_SN);
                    return wait_ms_out;
                }
            }
        }
        printf("wait");
        wait_ms_out--;
        if(wait_ms_out == 0)
            return wait_ms_out;
    }
}
//无地址等待响应
uint16_t WIRELESS_DATA_PROCESS::wait_wireless_ack(uint16_t wait_ms_out) {
    uint32_t receive_identufy_SN = 0;
    while(1) {
        si4463.read_IRQ();
        if(si4463_txrx.rxtx_status == 0) {
            //目的地址校验
            receive_identufy_SN = (si4463_txrx.rx_fifo[0]<<24)
                                  +(si4463_txrx.rx_fifo[1]<<16)
                                  +(si4463_txrx.rx_fifo[2]<<8)
                                  +(si4463_txrx.rx_fifo[3]);
            if(receive_identufy_SN == machine_info[0].machine.identify_SN) {
                printf("it is mine,ACK OK\n");
                return wait_ms_out;
            }
        }
        printf("wait");
        wait_ms_out--;
        if(wait_ms_out == 0)
            return wait_ms_out;
    }
}

void WIRELESS_DATA_PROCESS::login_save_machine_info(void) {
    memcpy(&machine_info[si4463_data_process.op_position].machine, &si4463_txrx.rx_fifo[9], 18);


}

//向本机器最小空位置注册设备
uint8_t WIRELESS_DATA_PROCESS::host_login(void) {
    uint8_t i;
    if (si4463_data_process.is_exist_machine == 0) {
        login_save_machine_info();
        srand(system_count);
        for (i = 0; i < 10; i++) {
            machine_info[si4463_data_process.op_position].machine.identify_KEY[i] = ((rand() % (253))+3);
            machine_info[0].machine.identify_KEY[i] = machine_info[si4463_data_process.op_position].machine.identify_KEY[i];
            printf("key:%x ", machine_info[si4463_data_process.op_position].machine.identify_KEY[i]);
        }
        printf("add device-position:%d,SN:%x\n", si4463_data_process.op_position,
               machine_info[si4463_data_process.op_position].machine.identify_SN);

        process_response(RESPONSE_SIGNIN);
        if(wait_wireless_ack(150, machine_info[si4463_data_process.op_position].machine.identify_SN) == 0) {
            machine_info[si4463_data_process.op_position].machine.identify_SN = 0;
            printf("add is fail! please try again!\n");
            return 1;
        }
        save_machine_num[0]++;
        STMFLASH_Write(FLASH_SAVE_ADDR, (u16*)save_machine_num, 1);
        STMFLASH_Write(FLASH_SAVE_ADDR+(si4463_data_process.op_position*18)+2,(u16*)&machine_info[si4463_data_process.op_position].machine,9);

        printf("add device is OK\n");
    } else {
//		process_response(RESPONSE_OK);
        printf("device is exist!\n");
    }
    return 0;
}

uint8_t WIRELESS_DATA_PROCESS::host_login(uint32_t device_SN) {
    uint8_t i;
    if (si4463_data_process.is_exist_machine == 0) {
//			login_save_machine_info();
        machine_info[si4463_data_process.op_position].machine.identify_SN = device_SN;
        srand(system_count);
        for (i = 0; i < 10; i++) {
            machine_info[si4463_data_process.op_position].machine.identify_KEY[i] = ((rand() % (253))+3);
            machine_info[0].machine.identify_KEY[i] = machine_info[si4463_data_process.op_position].machine.identify_KEY[i];
            printf("key:%x ", machine_info[si4463_data_process.op_position].machine.identify_KEY[i]);
        }
        printf("add device-position:%d,SN:%x\n", si4463_data_process.op_position,
               machine_info[si4463_data_process.op_position].machine.identify_SN);

        process_response(RESPONSE_SIGNIN);
        if(wait_wireless_ack(150, machine_info[si4463_data_process.op_position].machine.identify_SN) == 0) {
            machine_info[si4463_data_process.op_position].machine.identify_SN = 0;
            printf("add is fail! please try again!\n");
            return 1;
        }
        save_machine_num[0]++;
        STMFLASH_Write(FLASH_SAVE_ADDR, (u16*)save_machine_num, 1);
        STMFLASH_Write(FLASH_SAVE_ADDR+(si4463_data_process.op_position*18)+2,(u16*)&machine_info[si4463_data_process.op_position].machine,9);
        printf("add device is OK\n");
    } else {
//		process_response(RESPONSE_OK);
        printf("device is exist!\n");
    }
    return 0;
}

uint8_t WIRELESS_DATA_PROCESS::device_login() {
    uint8_t i;
    if(si4463_data_process.is_exist_machine == 0) {
        login_save_machine_info();
        for(i=0; i<10; i++) {
            printf("key-%x ",machine_info[si4463_data_process.op_position].machine.identify_KEY[i]);
        }
        save_machine_num[0]++;
        STMFLASH_Write(FLASH_SAVE_ADDR, (u16*)save_machine_num, 1);
        STMFLASH_Write(FLASH_SAVE_ADDR+(si4463_data_process.op_position*18)+2,(u16*)&machine_info[si4463_data_process.op_position].machine,9);
        printf("\n add host-position: %d,SN:%x\n",si4463_data_process.op_position,machine_info[si4463_data_process.op_position].machine.identify_SN);
        process_response(RESPONSE_OK);

    } else {
        login_save_machine_info();
        for(i=0; i<10; i++) {
            printf("key-%x ",machine_info[si4463_data_process.op_position].machine.identify_KEY[i]);
        }
        STMFLASH_Write(FLASH_SAVE_ADDR, (u16*)save_machine_num, 1);
        STMFLASH_Write(FLASH_SAVE_ADDR+(si4463_data_process.op_position*18)+2,(u16*)&machine_info[si4463_data_process.op_position].machine,9);
        printf("\n add host-position: %d,SN:%x\n",si4463_data_process.op_position,machine_info[si4463_data_process.op_position].machine.identify_SN);
        process_response(RESPONSE_OK);
        printf("this host is exist!!!-but i will add host again\n");

    }
    return 0;
}
//在主机上执行，如果存在则删除对应机器
uint8_t WIRELESS_DATA_PROCESS::host_logout() {

    if (si4463_data_process.is_exist_machine == 1) {
        printf("del device-position:%d,SN:%x\n", si4463_data_process.op_position,
               machine_info[si4463_data_process.op_position].machine.identify_SN);

        process_response(RESPONSE_SIGNOUT);

        if(wait_wireless_ack(100, machine_info[si4463_data_process.op_position].machine.identify_SN) == 0) {
            printf("del is fail! please try again!\n");
            return 1;
        }
        machine_info[si4463_data_process.op_position].machine.identify_SN = 0;

        save_machine_num[0]--;
        STMFLASH_Write(FLASH_SAVE_ADDR, (u16*)save_machine_num, 1);
        STMFLASH_Write(FLASH_SAVE_ADDR+(si4463_data_process.op_position*18)+2,(u16*)&machine_info[si4463_data_process.op_position].machine,9);

        printf("del device is OK\n");
    } else {
//		process_response(RESPONSE_OK);
        printf("this device is not exist!!!-not del\n");
    }
    return 0;
}
//在设备上执行，如果存在该主机则删除
uint8_t WIRELESS_DATA_PROCESS::device_logout() {

    if(si4463_data_process.is_exist_machine == 1) {
        printf("del host-position:%d,SN:%x\n",si4463_data_process.op_position,machine_info[si4463_data_process.op_position].machine.identify_SN);
        process_response(RESPONSE_OK);
        machine_info[si4463_data_process.op_position].machine.identify_SN = 0;

        save_machine_num[0]--;
        STMFLASH_Write(FLASH_SAVE_ADDR, (u16*)save_machine_num, 1);
        STMFLASH_Write(FLASH_SAVE_ADDR+(si4463_data_process.op_position*18)+2,(u16*)&machine_info[si4463_data_process.op_position].machine,9);
    } else {
        process_response(RESPONSE_OK);
        printf("this host is not exist!!!but i will del host again\n");
    }
    return 0;
}


uint8_t WIRELESS_DATA_PROCESS::test_device_process() {
//    uint8_t i;
    printf("\n------------------------数据解析-----------------------------\n");
    printf("设备ID:%d\n",((machine_info[si4463_data_process.op_position].data[3]&0x0F)
                            +machine_info[si4463_data_process.op_position].data[4]
                            +machine_info[si4463_data_process.op_position].data[5]
                            +machine_info[si4463_data_process.op_position].data[6])
          );
    printf("厂商ID:%x\n",(machine_info[si4463_data_process.op_position].data[7]
                           ));
    printf("设备类型:%x\n",(machine_info[si4463_data_process.op_position].data[8]
                               ));

    switch(machine_info[si4463_data_process.op_position].data[2]) {
    case 0x01:
        printf("数据上报\n");
        break;
    case 0x02:
        printf("入网操作\n");
        break;
    case 0x03:
        printf("LED操作\n");
        break;
    case 0x04:
        printf("警号控制设置\n");
        break;
    default:
        break;
    }

//    for(i=0; i<5; i++) {
//        while( RESET == USART_GetFlagStatus( USART3, USART_FLAG_TXE ));
//        USART3->DR = 0x55;
//    }
//    debug_tx_bytes(&machine_info[si4463_data_process.op_position].data[0], machine_info[si4463_data_process.op_position].data[1]+2);
    return 0;
}

void WIRELESS_DATA_PROCESS::test_smart_lock_process(void) {
    uint8_t i;
    printf("\n------------------------数据解析-----------------------------\n");
    switch(machine_info[si4463_data_process.op_position].data[1]) {
    case 0x10:
        printf("注册密码成功\n");
        break;
    case 0x51:
        printf("无需本地密码的远程开锁指令\n");
        for(i=0; i<10; i++) {
            while( RESET == USART_GetFlagStatus( USART1, USART_FLAG_TXE )) {}
            USART1->DR = 0x00;
        }
        drv_delay_ms(10);
        cmd_buf[0] = 0x55;
        cmd_buf[1] = 0x51;
        cmd_buf[2] = 0x01;
        cmd_buf[3] = 0xA7;
        cmd_buf[4] = 0xAA;
        for(i=0; i<3; i++) {
            drv_uart_tx_bytes(cmd_buf, 5);
        }
        break;
    case 0x20:
        printf("非法开锁\n");
        break;
    case 0x21:
        printf("机械钥匙开锁报警\n");
        break;
    default:
        printf("数据不被解析，详见文档\n");
        break;
    }
}

void WIRELESS_DATA_PROCESS::function_process() {
    //处理功能
    uint8_t i;
    for(i=0; i<si4463_txrx.receive_len-10; i++) {
        printf("data:%x ",machine_info[si4463_data_process.op_position].data[i]);
    }
    switch(machine_info[si4463_data_process.op_position].data[0]) {
    case 0x01:
        printf("no vaild op\n");
        //注册
        break;
    case 0x02:
        //注销
        break;
    case 0x03:
        printf("receive OK\n");
        break;
    case 0x04:
        //单向设备数据处理
        process_response(RESPONSE_OK);
        test_device_process();
        break;
    case 0x05:
        //双向设备数据处理
        process_response(RESPONSE_OK);
        test_smart_lock_process();
        break;
    default:
        break;
    }
}

// --pre --syc (-dst4 -src4 -fc1 -datax -crc2)--length
uint8_t WIRELESS_DATA_PROCESS::process_receive(void) {
    uint16_t calc_crc16 = 0,receive_crc16 = 0;
    uint32_t receive_identufy_SN = 0;

    //crc校验
    receive_crc16= (si4463_txrx.rx_fifo[si4463_txrx.receive_len-1])
                   +(si4463_txrx.rx_fifo[si4463_txrx.receive_len-2]<<8);
    calc_crc16 = crc16(si4463_txrx.rx_fifo,si4463_txrx.receive_len-2);

    if(calc_crc16 != receive_crc16) {
        printf("\nCRC ERROR!\n");
        si4463.si446x_recalibration();
        return 1;
    }
    //目的地址校验
    receive_identufy_SN = (si4463_txrx.rx_fifo[0]<<24)
                          +(si4463_txrx.rx_fifo[1]<<16)
                          +(si4463_txrx.rx_fifo[2]<<8)
                          +(si4463_txrx.rx_fifo[3]);
    if((receive_identufy_SN != machine_info[0].machine.identify_SN)
            &&(receive_identufy_SN != 0xFFFFFFFF)) {
        printf("not mine!\n");
        return 2;
    }
    //查找机器是否存在 如果存在则返回该机器位置 如果不存在找到空机器的最小位置。最大支持32个机器
    si4463_data_process.op_position = check_machine();
    if( si4463_data_process.op_position == 0) {
        printf("find machine error!\n");
        return 3;
    }

    if(machine_info[0].machine.name.machine_type == HOST) {
        if(machine_info[0].machine.status == SIGNIN) {
            host_login();
        } else if(machine_info[0].machine.status == SIGNOUT) {
            host_logout();
        } else if((machine_info[0].machine.status == NORMAL)
                  &&(si4463_data_process.is_exist_machine == 1)) {
            //解密
            cryption_data_process(&si4463_txrx.rx_fifo[8], si4463_txrx.receive_len-10, &machine_info[si4463_data_process.op_position].data[0]);
            //处理数据
            function_process();
        } else {
            printf("this machine is not be supported\n");
        }
    } else if(machine_info[0].machine.name.machine_type == DEVICE) {
        //设备是被动注册，即需要功能码判断主机是向我注册还是注销 还是做事
        if(si4463_data_process.is_exist_machine == 1) {
            cryption_data_process(&si4463_txrx.rx_fifo[8], si4463_txrx.receive_len-10, &machine_info[si4463_data_process.op_position].data[0]);
            function_process();
        }
        //解决主机丢失注册接收端ACK的问题
        if((si4463_txrx.rx_fifo[8] == 0x01)&&(si4463_txrx.receive_len == 21)) {
            device_login();
        } else if((si4463_txrx.rx_fifo[8] == 0x02)&&(si4463_txrx.receive_len == 11)) {
            device_logout();
        }
    }
#if MACHINE_HOST < 1
    si4463.SI446x_Change_Status(1);
#endif
    return 0;
}

uint8_t WIRELESS_DATA_PROCESS::calc_check_sum(uint8_t *data, uint8_t len) {
    uint8_t calc_sum=0;
    uint8_t i;
    for(i=0; i<len; i++) {
        calc_sum += *data++;
    }
    return (uint8_t)calc_sum;
}
//主板 命令处理函数
uint8_t last_lock_data[2]= {0};
void WIRELESS_DATA_PROCESS::process_command(void) {
    uint8_t calc_sum=0,i=0,j;
    uint8_t return_cmd_buf[32];
    serial_cmd_Loop();
    if(command_handle.op_fsm == SERIAL_RECEIVE) {
        //主板上传信息向无线模组发送数据
        if(cmd_buf[0] == 0xFA) {
            calc_sum = calc_check_sum(cmd_buf, cmd_buf[1]+1);
            printf("sumcalc:%x\n",calc_sum);
            if(calc_sum != cmd_buf[cmd_buf[1]+1]) {
                printf("sum check erro\n");
                return;
            }
            return_cmd_buf[i++] = 0xAA;
            return_cmd_buf[i++] = 0xAA;
            return_cmd_buf[i++] = 0xF5;
            for(j=1; j<cmd_buf[1]+1; j++) {
                return_cmd_buf[i++] = cmd_buf[j];
            }
            calc_sum = calc_check_sum(&return_cmd_buf[2], cmd_buf[1]+1);
            return_cmd_buf[cmd_buf[1]+3] = calc_sum;
            //响应主板 F5
            drv_uart_tx_bytes(return_cmd_buf, cmd_buf[1]+4);

            si4463_data_process.op_position = 1;
            process_transfer();
        }
//        //无线模组收到数据后向主板发送数据
//        else if(cmd_buf[0] == 0xF5) {
//            printf("wuxian to zhuban\n");
//        }
        //门锁设备上传到无线模组数据处理
        else if(cmd_buf[0] == 0x55) {
            calc_sum = calc_check_sum(cmd_buf, cmd_buf[2]+2);
            printf("%x",calc_sum);
            if(cmd_buf[cmd_buf[2]+2] != calc_sum) {
                printf("sum check erro\n");
            } else {
                switch(cmd_buf[1]) {
                case 0x80:
                    printf("repose OK\n");
                    process_response(RESPONSE_OK);
                    break;
                case 0x81:
                    printf("repose FAIL\n");

                    break;
                default:
                    printf("数据不被解析，详见文档\n");
                    si4463_data_process.op_position = 1;
                    process_transfer();
                    break;
                }
            }
        }

        command_handle.op_fsm = SERIAL_IDLE;
    }

#if DEBUG == 1
    if(command_handle.serial_debug == 1) {
        command_handle.serial_debug = 0;
        if(cmd_buf[0] == 't') {
            test_transfer();
        } else if(cmd_buf[0] == 's') {
            set_machine_info();
        } else if(!memcmp(cmd_buf,"add",3)) {
            printf("\nadd device\n");
            host_add_device();
        } else if(!memcmp(cmd_buf,"clear-save",10)) {
            printf("\nclear save machine data\n");
            clear_save_machine_data();
        } else if(!memcmp(cmd_buf,"check",5)) {
            printf("\ncheck machine info\n");
            check_machine_info();
        } else if(cmd_buf[0] == 0x55) {
            //主机对 smart lock 进行远程控制
#if MACHINE_HOST == 1
            for(i=1; i<MACHINE_NUM_MAX; i++) {
                //寻找门锁设备
                if((machine_info[i].machine.identify_SN&0xf0000000) == 0x20000000) {
                    si4463_data_process.op_position = i;
                    break;
                }
            }
            if(i == MACHINE_NUM_MAX) {
                printf("there is no smart lock,it is invalid op\n");
                return ;
            }
            printf("smart lock commander!\n");
            process_transfer();
#endif
        }
    }
#endif
}

void WIRELESS_DATA_PROCESS::check_machine_info(void) {
    uint8_t i;
    printf("save machine num:%d\r\n", save_machine_num[0]);

    for(i=0; i<MACHINE_NUM_MAX; i++) {
        if(machine_info[i].machine.identify_SN != 0) {
            printf("position:%d \n", i);
            printf("type:%x\r\n",machine_info[i].machine.name.machine_type);
            printf("sort:%x\r\n",machine_info[i].machine.name.machine_sort);
            printf("detail:%x\r\n",machine_info[i].machine.name.machine_detail);
            printf("status:%x\r\n",machine_info[i].machine.status);
            printf("SN:%x\r\n",machine_info[i].machine.identify_SN);
        }
    }
}

void WIRELESS_DATA_PROCESS::clear_save_machine_data(void) {
    uint8_t i;
    save_machine_num[0] = 0;
    STMFLASH_Write(FLASH_SAVE_ADDR, (u16*)save_machine_num, 1);
    memset(machine_info, 0, sizeof(machine_info));
    for(i=0; i<MACHINE_NUM_MAX; i++) {
        STMFLASH_Write(FLASH_SAVE_ADDR+(i*18)+2,(u16*)&machine_info[i].machine,9);
    }

    for(i=0; i<MACHINE_NUM_MAX; i++) {
        STMFLASH_Read(FLASH_SAVE_ADDR+(i*18)+2,(u16*)&machine_info[i].machine,9);
    }
    printf("clear save machine data OK\n");
}

void WIRELESS_DATA_PROCESS::host_add_device(void) {
    uint8_t i;
    uint32_t temp_device_SN = 0;
    //仅支持小写和数字，且8位
    for(i=3; i<11; i++) {
        cmd_buf[i] -= 0x30;
        if((cmd_buf[i]<=0x36)&&(cmd_buf[i]>=0x31)) {
            temp_device_SN += (cmd_buf[i]-0x27) << (4*(10-i));
        } else if((cmd_buf[i]<=9)&&(cmd_buf[i]>=0)) {
            temp_device_SN += (cmd_buf[i]) << (4*(10-i));
        }
    }
    if(temp_device_SN)
        printf("\nyou want to add SN:%x \n", temp_device_SN);
    //寻找位置
    si4463_data_process.op_position = check_machine(temp_device_SN);
    if(si4463_data_process.is_exist_machine == 1) {
        printf("this device is exist,not need to add\n");
        return;
    } else {
        host_login(temp_device_SN);
    }
}

void WIRELESS_DATA_PROCESS::set_machine_info(void) {

    if(!memcmp(cmd_buf, "set-host",8)) {
        machine_info[0].machine.name.machine_type = HOST;
        printf("HOST\n");
    } else if(!memcmp(cmd_buf,"set-device",10)) {
        machine_info[0].machine.name.machine_type = DEVICE;
        printf("DEVICE\n");
    } else if(!memcmp(cmd_buf,"set-signin",10)) {
        machine_info[0].machine.status = SIGNIN;
        printf("IN\n");
    } else if(!memcmp(cmd_buf,"set-signout",11)) {
        machine_info[0].machine.status = SIGNOUT;
        printf("OUT\n");
    } else if(!memcmp(cmd_buf,"set-normal",10)) {
        machine_info[0].machine.status = NORMAL;
        printf("NORMAL\n");
    }
    STMFLASH_Write(FLASH_SAVE_ADDR+2, (u16*)&machine_info[0].machine, 9);
    STMFLASH_Read(FLASH_SAVE_ADDR+2,(u16*)&machine_info[0].machine, 9);
    printf("status:%x\n",machine_info[0].machine.status);
}

void WIRELESS_DATA_PROCESS::test_transfer(void) {
    uint8_t sendtest[32] = {0x00};
    uint8_t i;
    uint16_t calc_crc16;

    sendtest[0] = 0xFF;
    sendtest[1] = 0xFF;
    sendtest[2] = 0xFF;
    sendtest[3] = 0xFF;

    for(i=MACHINE_NUM_MAX-1; i>0; i--) {
        if(machine_info[i].machine.identify_SN != 0x00) {
            printf("send-position-%d-SN%d \n",i,machine_info[i].machine.identify_SN);
            printf("this is means we find one machine as one DST\n");
            si4463_data_process.op_position = i;
            sendtest[0] = machine_info[i].machine.identify_SN>>24;
            sendtest[1] = machine_info[i].machine.identify_SN>>16;
            sendtest[2] = machine_info[i].machine.identify_SN>>8;
            sendtest[3] = machine_info[i].machine.identify_SN;
        }
    }

    sendtest[4] = machine_info[0].machine.identify_SN>>8;
    sendtest[5] = machine_info[0].machine.identify_SN;
    sendtest[6] = machine_info[0].machine.identify_SN>>8;
    sendtest[7] = machine_info[0].machine.identify_SN;

    sendtest[8] = 0x01;
    sendtest[9] = 0xff;
    sendtest[10] = 0xff;
    sendtest[11] = 0xff;
    sendtest[12] = 0xff;
    sendtest[13] = 0xff;
    if(si4463_data_process.op_position != 0)
        cryption_data_process(&sendtest[8],6,&sendtest[8]);

    calc_crc16 = crc16(sendtest, 14);
    sendtest[14] = (calc_crc16>>8)&0xff;
    sendtest[15] =  calc_crc16&0x0ff;

    debug_tx_bytes(sendtest,16);
    printf("test send\n");

    si4463.SI446x_Send_Packet(sendtest, 16, 0, 0x80 );
}
//查找本机储存的目的地址，没有则进行广播发送
//    for(i=1; i<MACHINE_NUM_MAX; i++) {
//        if(machine_info[i].machine.identify_SN != 0x00) {
//            si4463_data_process.op_position = i;
//            printf("this message means it have DST:%x\n", machine_info[i].machine.identify_SN);
//            sendtest[0] = machine_info[i].machine.identify_SN>>24;
//            sendtest[1] = machine_info[i].machine.identify_SN>>16;
//            sendtest[2] = machine_info[i].machine.identify_SN>>8;
//            sendtest[3] = machine_info[i].machine.identify_SN;
//            break;
//        }
//    }



void WIRELESS_DATA_PROCESS::process_transfer(void) {
    uint8_t sendtest[32] = {0x00};
    uint8_t temp_ms;
    uint8_t i,j;
    uint16_t calc_crc16;

    //写入本机地址
    sendtest[4] = machine_info[0].machine.identify_SN>>24;
    sendtest[5] = machine_info[0].machine.identify_SN>>16;
    sendtest[6] = machine_info[0].machine.identify_SN>>8;
    sendtest[7] = machine_info[0].machine.identify_SN;

    if(machine_info[si4463_data_process.op_position].machine.identify_SN == 0) {
        //没有目的地址
        sendtest[0] = 0xFF;
        sendtest[1] = 0xFF;
        sendtest[2] = 0xFF;
        sendtest[3] = 0xFF;
				sendtest[8] = 0x01;
			  memcpy(&sendtest[9], &machine_info[0].machine, 18);
				j = 27;
    } else {
			//有目的地址
        printf("this message means it have DST:%x\n", machine_info[si4463_data_process.op_position].machine.identify_SN);
        sendtest[0] = machine_info[si4463_data_process.op_position].machine.identify_SN>>24;
        sendtest[1] = machine_info[si4463_data_process.op_position].machine.identify_SN>>16;
        sendtest[2] = machine_info[si4463_data_process.op_position].machine.identify_SN>>8;
        sendtest[3] = machine_info[si4463_data_process.op_position].machine.identify_SN;

        if(((machine_info[si4463_data_process.op_position].machine.identify_SN&0xF0000000) != 0x20000000)
                &&((machine_info[0].machine.identify_SN&0xF0000000) != 0x20000000)) {
            //报警类设备
            sendtest[8] = 0x04;
            //写入其余数据
            for(i=1,j=9; i<cmd_buf[1]+2; i++,j++) {
                sendtest[j] = cmd_buf[i];
            }
        } else {
            sendtest[8] = 0x05;
            //门锁设备
            for(i=1,j=9; i<cmd_buf[2]+2; i++,j++) {
                sendtest[j] = cmd_buf[i];
            }
        }
        cryption_data_process(&sendtest[8],j-8,&sendtest[8]);
    }
		
    calc_crc16 = crc16(sendtest, j);
    sendtest[j++] = (calc_crc16>>8);
    sendtest[j++] = (calc_crc16);

    printf("test send data:");
    for(i=0; i<j; i++) {
        printf("%x ",sendtest[i]);
    }

    for(i=0; i<3; i++) {
        if(si4463_txrx.rxtx_status == 0) {
            //这代表此时有数据触发中断，信道有干扰，等待某个时间周期
//            si4463_txrx.rxtx_status = 0xff;
//            EXTI->IMR|=(1<<2);
            srand(system_count);
            //随机延时
            temp_ms = ((rand()%100)+10);
            printf("\nCCA_dedlay_ms:%d\n",temp_ms);
            drv_delay_ms(temp_ms);
        } else {
            si4463.SI446x_Send_Packet(sendtest, j, 0, 0x80 );
            if(machine_info[si4463_data_process.op_position].machine.identify_SN != 0) {
                //有目的地的发送需要等待 需要验证响应方序列号
                if(wait_wireless_ack(100, machine_info[si4463_data_process.op_position].machine.identify_SN)) {
                    break;
                } else {
                    si4463.si446x_recalibration();
                    printf("\nnot ACK\n");
                    si4463_data_process.work_fail_counter++;
                }
            } else {
                //广播发送，无需验证响应方序列号
                if(wait_wireless_ack(100)) {
                    break;
                } else {
                    si4463.si446x_recalibration();
                    si4463_data_process.work_fail_counter++;
                    printf("\nnot ACK\n");
                }
            }
        }
    }
    if(i==3) {
        if(si4463_data_process.work_fail_counter >= 10) {
            __set_FAULTMASK(1);      // 关闭所有中端
            NVIC_SystemReset();// 复位
        }
#if MACHINE_HOST < 1
        si4463.SI446x_Change_Status(1);
#endif
        printf("SEND fail\n");
    }

    if(key_value == 1) {
    }
}

void WIRELESS_DATA_PROCESS::process_response(uint8_t op) {
//    uint8_t i;
    uint16_t calc_crc16;
    uint8_t sendtest[32] = {0x00};

    switch(op) {
    case RESPONSE_SIGNIN:
        sendtest[0] = (uint8_t)(machine_info[si4463_data_process.op_position].machine.identify_SN>>24);
        sendtest[1] = (uint8_t)(machine_info[si4463_data_process.op_position].machine.identify_SN>>16);
        sendtest[2] = (uint8_t)(machine_info[si4463_data_process.op_position].machine.identify_SN>>8);
        sendtest[3] = (uint8_t)(machine_info[si4463_data_process.op_position].machine.identify_SN);
        sendtest[4] = (uint8_t)(machine_info[0].machine.identify_SN>>24);
        sendtest[5] = (uint8_t)(machine_info[0].machine.identify_SN>>16);
        sendtest[6] = (uint8_t)(machine_info[0].machine.identify_SN>>8);
        sendtest[7] = (uint8_t)(machine_info[0].machine.identify_SN);

        sendtest[8] = 0x01;
        memcpy(&sendtest[9], &machine_info[0].machine, 18);
        calc_crc16 = crc16(sendtest, 19);

        sendtest[27] = calc_crc16>>8;
        sendtest[28] =  calc_crc16;

        si4463.SI446x_Send_Packet(sendtest, 29, 0, 0x80 );
        printf("response SIGNIN,this is need ACK\n");

        break;
    case RESPONSE_OK:
        printf("op:%d \n",si4463_data_process.op_position);
        sendtest[0] = (uint8_t)(machine_info[si4463_data_process.op_position].machine.identify_SN>>24);
        sendtest[1] = (uint8_t)(machine_info[si4463_data_process.op_position].machine.identify_SN>>16);
        sendtest[2] = (uint8_t)(machine_info[si4463_data_process.op_position].machine.identify_SN>>8);
        sendtest[3] = (uint8_t)(machine_info[si4463_data_process.op_position].machine.identify_SN);
        sendtest[4] = (uint8_t)(machine_info[0].machine.identify_SN>>24);
        sendtest[5] = (uint8_t)(machine_info[0].machine.identify_SN>>16);
        sendtest[6] = (uint8_t)(machine_info[0].machine.identify_SN>>8);
        sendtest[7] = (uint8_t)(machine_info[0].machine.identify_SN);

        sendtest[8] = 0x03;
        calc_crc16 = crc16(sendtest, 9);

        sendtest[9] = (calc_crc16>>8);
        sendtest[10] =  calc_crc16;
        si4463.SI446x_Send_Packet(sendtest, 11, 0, 0x80 );
        printf("response OK,this is not need ACK\n");
#if MACHINE_HOST < 1
        si4463.SI446x_Change_Status(1);
#endif
        break;
    case RESPONSE_SIGNOUT:
        sendtest[0] = (uint8_t)(machine_info[si4463_data_process.op_position].machine.identify_SN>>24);
        sendtest[1] = (uint8_t)(machine_info[si4463_data_process.op_position].machine.identify_SN>>16);
        sendtest[2] = (uint8_t)(machine_info[si4463_data_process.op_position].machine.identify_SN>>8);
        sendtest[3] = (uint8_t)(machine_info[si4463_data_process.op_position].machine.identify_SN);
        sendtest[4] = (uint8_t)(machine_info[0].machine.identify_SN>>24);
        sendtest[5] = (uint8_t)(machine_info[0].machine.identify_SN>>16);
        sendtest[6] = (uint8_t)(machine_info[0].machine.identify_SN>>8);
        sendtest[7] = (uint8_t)(machine_info[0].machine.identify_SN);

        sendtest[8] = 0x02;
        calc_crc16 = crc16(sendtest, 9);

        sendtest[9] = (calc_crc16>>8);
        sendtest[10] =  calc_crc16;
        si4463.SI446x_Send_Packet(sendtest, 11, 0, 0x80 );
        printf("response SIGNOUT,this is need ACK\n");

        break;
    default:
        break;
    }
}
