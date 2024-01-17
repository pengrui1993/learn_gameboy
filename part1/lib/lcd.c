#include <lcd.h>
#include <ppu.h>
#include <dma.h>
#define ARR_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))
static lcd_context ctx;
static uint32_t colors_default[4] = {
    0xffffffff,0xffaaaaaa,0xff555555,0xff000000
};// light lightgray darkgray dark
lcd_context* lcd_get_context(){return &ctx;}
void lcd_init(){
    ctx.lcdc = 0x91;
    ctx.scroll_x = 0;
    ctx.scroll_y = 0;
    ctx.ly = 0;
    ctx.ly_compare = 0;
    ctx.bg_palette = 0xFC;
    ctx.obj_palette[0] = 0xff;
    ctx.obj_palette[1] = 0xff;
    ctx.win_y = 0;
    ctx.win_x = 0;
    for(int i=0;i<ARR_SIZE(colors_default);i++){
        ctx.bg_colors[i] = colors_default[i];
        ctx.sp1_colors[i] = colors_default[i];
        ctx.sp2_colors[i] = colors_default[i];
    }
}
u8 lcd_read(u16 address){ 
    u8 offset = (address - 0xFF40);
    u8 *p = (u8*)&ctx;
    return p[offset];
}
static void update_palette(u8 palette_data,u8 pal){
    u32* p_colors = ctx.bg_colors;
    switch(pal){
        case 1: p_colors = ctx.sp1_colors;break;
        case 2: p_colors = ctx.sp2_colors;break;
    }
    p_colors[0] = colors_default[palette_data&0b11];
    p_colors[1] = colors_default[(palette_data>>2)&0b11];
    p_colors[2] = colors_default[(palette_data>>4)&0b11];
    p_colors[3] = colors_default[(palette_data>>6)&0b11];
}
void lcd_write(u16 address,u8 value){
    u8 offset = (address - 0xFF40);
    u8 *p = (u8*)&ctx;
    p[offset] = value;
    if(6 == offset){//0xFF46 = DMA
        dma_start(value);
    }
    switch(address){
        case 0xFF47: update_palette(value,0);return;
        case 0xFF48: update_palette(value & 0b11111100,1);return;
        case 0xFF49: update_palette(value & 0b11111100,2);return;
    }
  
}