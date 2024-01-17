#include<dbg.h>
#include<bus.h>
#include<common.h>
#include<emu.h>
#include<cpu.h>
#define MSG_BUF_SIZE  1024

extern cpu_context ctx;
static char dbg_msg[MSG_BUF_SIZE] = {0};
static int msg_size = 0;

void dbg_update(){//https://gbdev.io/pandocs/Serial_Data_Transfer_(Link_Cable).html#ff02--sc-serial-transfer-control
    if(0x81==bus_read(0xFF02)){//0x81 see upon link
        if(MSG_BUF_SIZE==msg_size){
            printf("out of memory\n");
            NO_IMPL
        }else{
            char c = bus_read(0xFF01);
            dbg_msg[msg_size++] = c;
            bus_write(0xFF02,0);
        }
    }
}
void dbg_print(){
    if(dbg_msg[0])printf("DBG: %s\n",dbg_msg);
}