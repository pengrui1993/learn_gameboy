#include <bus.h>
#include <common.h>
#include <cart.h>
#include <ram.h>
#include <cpu.h>
#include <io.h>
#include <ppu.h>
#include <dma.h>
/*
https://gbdev.io/pandocs/Memory_Map.html

Start	End	Description	Notes
0000	3FFF	16 KiB ROM bank 00	From cartridge, usually a fixed bank
4000	7FFF	16 KiB ROM Bank 01~NN	From cartridge, switchable bank via mapper (if any)
8000	9FFF	8 KiB Video RAM (VRAM)	In CGB mode, switchable bank 0/1
A000	BFFF	8 KiB External RAM	From cartridge, switchable bank if any
C000	CFFF	4 KiB Work RAM (WRAM)
D000	DFFF	4 KiB Work RAM (WRAM)	In CGB mode, switchable bank 1~7
E000	FDFF	Mirror of C000~DDFF (ECHO RAM)	Nintendo says use of this area is prohibited.
FE00	FE9F	Object attribute memory (OAM)
FEA0	FEFF	Not Usable	Nintendo says use of this area is prohibited
FF00	FF7F	I/O Registers
FF80	FFFE	High RAM (HRAM)
FFFF	FFFF	Interrupt Enable register (IE)
*/

static const bool show_reserved = false;
u8 bus_read(u16 address){
    if (address < 0x8000){//ROM Data
        return cart_read(address);
    }else if(address < 0XA000){
        // printf("Char/Map data\n");
        return ppu_vram_read(address);
    }else if(address < 0XC000){  //Cartridge RAM
        return cart_read(address);
    }else if(address < 0XE000){
        //WRAM(Working RAM)
        return wram_read(address);
    }else if(address < 0XFE00){
        if(show_reserved)
            printf("reserved echo ram read,addr:%04X ...\n",address);
        return 0;
    }else if(address < 0XFEA0){
        //printf("OAM (Object attribute memory)\n");
        if(dma_transferring())return 0xFF;
        //While an OAM DMA is in progress, the PPU cannot read OAM properly either. 
        return ppu_oam_read(address);
    }else if(address < 0XFF00){
        if(show_reserved)
            printf("reserved unusable,addr:%04X ...\n",address);
        return 0;
    }else if(address < 0XFF80){
        return io_read(address);
    }else if(address == 0xFFFF){//printf("CPU ENABLE REGISTER...\n");
        return cpu_get_ie_register();
    }else{
        return hram_read(address);
    }
// printf("UNSOPPURTED bus_read(%04X)\n",address);
//    NO_IMPL
}
void bus_write(u16 address, u8 value){
    if (address < 0x8000){ //ROM Data
        cart_write(address, value);
        return;
    }else if(address < 0XA000){
        //printf("Char/Map data\n");return;
        ppu_vram_write(address,value);
        return;
    }else if(address < 0XC000){ //Cartridge RAM  EXT-RAM   
        cart_write(address,value);
        return;
    }else if(address < 0XE000){
        wram_write(address,value);//WRAM(Working RAM)
        return ;
    }else if(address < 0XFE00){
        if(show_reserved)
            printf("reserved echo ram write,addr:%04X,value:%02X...\n"
            ,address,value);
    }else if(address < 0XFEA0){
        //printf("OAM (Object attribute memory)\n");
        if(dma_transferring())return;

        ppu_oam_write(address,value);
        return;
    }else if(address < 0XFF00){
        if(show_reserved)
            printf("reserved unusable,addr:%04X,value:%02X...\n"
            ,address,value);
        return;
    }else if(address < 0XFF80){//CGB Registers(Color GameBoy)
        io_write(address,value);
        return;
    }else if(address == 0xFFFF){
        //printf("CPU ENABLE REGISTER...\n");
        cpu_set_ie_register(value);
        return;
    }else{
        hram_write(address,value);
        return ;
    }
//    printf("UNSOPPURTED bus_write(%04X)\n",address);
//    NO_IMPL
}
u16 bus_read16(u16 address){
    u16 lo = bus_read(address);
    u16 hi = bus_read(address+1);
    return lo|(hi<<8);
}
void bus_write16(u16 address, u16 value){
    bus_write(address+1,(value>>8)&0xff);
    bus_write(address,value&0xff);
}