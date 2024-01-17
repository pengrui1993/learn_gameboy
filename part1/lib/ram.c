#include<ram.h>
#define RAM_W_SIZE 0x2000
#define RAM_H_SIZE 0x80
//see memory map
#define WRAM_ADDR_START 0xC000
#define HRAM_ADDR_START 0xFF80


typedef struct{
    u8 wram[RAM_W_SIZE];
    u8 hram[RAM_H_SIZE];
}ram_context;
static ram_context ctx;
void hram_write(u16 address,u8 value){
      address -= HRAM_ADDR_START;
    if(address >=RAM_H_SIZE){
        printf("INVALID (H)RAM READ,ADDR %08X\n",address+HRAM_ADDR_START);
        exit(-9);
    }
    ctx.hram[address] = value;
}
u8 hram_read(u16 address){
    address -= HRAM_ADDR_START;
    if(address >=RAM_H_SIZE){
        printf("INVALID (H)RAM READ,ADDR %08X\n",address+HRAM_ADDR_START);
        exit(-9);
    }
    return ctx.hram[address];
}
u8 wram_read(u16 address){
    address -= WRAM_ADDR_START;
    if(address >=RAM_W_SIZE){
        printf("INVALID WRAM READ,ADDR %08X\n",address+WRAM_ADDR_START);
        exit(-9);
    }
    return ctx.wram[address];
}
void wram_write(u16 address,u8 value){
    address -= WRAM_ADDR_START;
    if(address >=RAM_W_SIZE){
        printf("INVALID WRAM WRITE,ADDR %08X\n",address+WRAM_ADDR_START);
        exit(-9);
    }
    ctx.wram[address] = value;
}