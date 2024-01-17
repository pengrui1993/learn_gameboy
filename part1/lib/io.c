#include<stdlib.h>
#include<io.h>
#include<timer.h>
#include<cpu.h>
#include<dma.h>
#include<lcd.h>
#include<gamepad.h>
#include <apu.h>
//ff01:serial transfer data(r/w)
//ff02:serial transfer control(r/w)
//https://gbdev.io/pandocs/Serial_Data_Transfer_(Link_Cable).html
static char serial_data[2];
u8 io_read(u16 address){
    switch (address){
    case 0xFF00://printf("ignore Controller IO Registers read\n");
        return gamepad_get_output();
    case 0xFF01:
        return serial_data[0];
    case 0xFF02:
        return serial_data[1];
    case 0xFF0F:
        return cpu_get_int_flags();
    default:
        if(BETWEEN(address,0xFF04,0xff07)){
            return timer_read(address);
        }
        if(BETWEEN(address,0xFF10,0xff3f)){
            return apu_read(address);
        }
        if(BETWEEN(address,0xFF40,0xff4B)){
            return lcd_read(address);
        }
        //printf("UNSUPPORTED IO Registers read,addr:%02X...\n",address);
        // NO_IMPL
        return 0x0;//for test
    }
}
void io_write(u16 address,u8 value){
    switch (address){
        case 0xFF00:// printf("ignore Controller IO Registers (write)\n");
            gamepad_set_sel(value);
            return;    
        case 0xFF01:
            serial_data[0] = value;return;
        case 0xFF02:
            serial_data[1] = value;return;
            // printf("serial control 0xff02 write\n");
        case 0xFF0F:
            cpu_set_int_flags(value);return;
        case 0xFF46:
            dma_start(value);
            //printf("DMA START\n");
            return;
        case 0xFF77:
            printf("ignore 0xFF77 IO Registers (write)\n");
            break;
        default:{
            if(BETWEEN(address,0xFF04,0xff07)){
                timer_write(address,value);return;
            }
            if(BETWEEN(address,0xFF10,0xff3f)){
               apu_write(address,value);return;
            }
            if(BETWEEN(address,0xFF40,0xff4B)){
                lcd_write(address,value);return;
            }
            //printf("UNSUPPORTED IO Registers write,addr:%02X...\n",address);
            // NO_IMPL
        }
    }
}