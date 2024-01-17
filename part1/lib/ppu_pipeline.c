#include <ppu.h>
#include <lcd.h>
#include <bus.h>
#define ctx() ppu_get_context()
#define lctx() lcd_get_context()
//static ppu_context* ctx(){ return ppu_get_context();}
//static lcd_context* lctx(){ return lcd_get_context();}
bool window_visible();

static void pixel_fifo_push(u32 value){
    fifo_entry* next = malloc(sizeof(fifo_entry));
    next->next = NULL;
    next->value = value;
    if(!ctx()->pfc.pixel_fifo.head){
        ctx()->pfc.pixel_fifo.head = 
            ctx()->pfc.pixel_fifo.tail = next;
    }else{
        ctx()->pfc.pixel_fifo.tail->next = next;
        ctx()->pfc.pixel_fifo.tail = next;
    }
    ctx()->pfc.pixel_fifo.size++;
}
static u32 pixel_fifo_pop(){
     if(ctx()->pfc.pixel_fifo.size<=0)NO_IMPL

     fifo_entry* popped = ctx()->pfc.pixel_fifo.head;
     ctx()->pfc.pixel_fifo.head = popped->next;
     ctx()->pfc.pixel_fifo.size--;
     u32 val = popped->value;
     free(popped);
     return val;
}
static u32 fetch_sprite_pixels(int bit,u32 color,u8 bg_color){
    for(int i=0;i<ctx()->fetched_entry_count;i++){
        int sp_x = (ctx()->fetched_entries[i].x-8)
            +((lctx()->scroll_x % 8));
        //past pixel point already...
        if(sp_x +8 < ctx()->pfc.fifo_x)continue;
        int offset = ctx()->pfc.fifo_x-sp_x;
        if(offset<0 || offset >7)continue;//out of bounds    
        bit = (7-offset);
        if(ctx()->fetched_entries[i].f_x_flip)bit = offset;
        u8 hi = !!(ctx()->pfc.fetch_entry_data[i*2]&(1<<bit));
        u8 lo = !!(ctx()->pfc.fetch_entry_data[(i*2)+1]&(1<<bit)) << 1;
        bool bg_priority = ctx()->fetched_entries[i].f_bgp;
        if(!(hi|lo))continue;//transparent
        if(!bg_priority || 0 == bg_color) {
            color = ctx()->fetched_entries[i].f_pn
                ?lctx()->sp2_colors[hi|lo]
                :lctx()->sp1_colors[hi|lo]
                ;
            if(hi|lo)break;
        }
            
                
    }
    return color;
}
static bool pipeline_fifo_add(){
    if(ctx()->pfc.pixel_fifo.size>8)return false;//fifo is full
    int x = ctx()->pfc.fetch_x - (8-(lctx()->scroll_x%8));
    for(int i=0;i<8;i++){
        int bit = 7-i;
        u8 hi = !!(ctx()->pfc.bgw_fetch_data[1]&(1<<bit));
        u8 lo = !!(ctx()->pfc.bgw_fetch_data[2]&(1<<bit))<<1;
        u32 color = lctx()->bg_colors[hi|lo];

        if(!LCDC_BGW_ENABLE)color = lctx()->bg_colors[0];
        if(LCDC_OBJ_ENABLE)color = fetch_sprite_pixels(bit,color,hi|lo);

        if(x>=0){
            pixel_fifo_push(color);
            ctx()->pfc.fifo_x++;
        }
    }
    return true;
}
static void pipeline_load_sprite_tile(){
    oam_line_entry* le = ctx()->line_sprites;
    while(le){
        int sp_x = (le->entry.x - 8) + (lctx()->scroll_x % 8);
        if((sp_x>= ctx()->pfc.fetch_x 
            && sp_x < ctx()->pfc.fetch_x + 8)
            ||
            ((sp_x +8) >= ctx()->pfc.fetch_x 
                && (sp_x+8)< ctx()->pfc.fetch_x + 8)
            ){//need to add entry
                ctx()->fetched_entries
                    [ctx()->fetched_entry_count++] = le->entry;
        }
        le = le->next;
        if(!le || ctx()->fetched_entry_count>=3){
            break;//max checking 3 sprites on pixels
        }
    }
}
static void pipeline_load_sprite_data(u8 offset){
    int cur_y = lctx()->ly;
    u8 sprite_height = LCDC_OBJ_HEIGHT;
    for(int i=0;i<ctx()->fetched_entry_count;i++){
        u8 ty = ((cur_y+16)-ctx()->fetched_entries[i].y)*2;
        if(ctx()->fetched_entries[i].f_y_flip){
            //flipped upside down...
            ty = ((sprite_height*2)-2)-ty;
        }
        u8 tile_index = ctx()->fetched_entries[i].tile;
        if(16 == sprite_height){
            tile_index&=~(1);//remove last bit...
        }
        ctx()->pfc.fetch_entry_data[(i*2)+offset] 
            = bus_read(0x8000+(tile_index*16)+ty+offset);
    }
}
static void pipeline_load_window_tile(){
    if(!window_visible())return;
    u8 window_y = lctx()->win_y;
    if(ctx()->pfc.fetch_x+7>=lctx()->win_x
        &&ctx()->pfc.fetch_x+7<lctx()->win_x+YRES+14){
        if(lctx()->ly>=window_y 
            &&lctx()->ly<window_y + XRES){
            u8 w_tile_y = ctx()->window_line/8;
            ctx()->pfc.bgw_fetch_data[0]
                =bus_read(LCDC_WIN_MAP_AREA
                    +((ctx()->pfc.fetch_x+7-lctx()->win_x)/8)
                    +(w_tile_y*32));
            if(0x8800==LCDC_BGW_DATA_AREA){
                ctx()->pfc.bgw_fetch_data[0]+=128;
            }
        }
    }
}
static void pipeline_fetch(){
    switch(ctx()->pfc.cur_fetch_state){
        case FS_TILE:{
            ctx()->fetched_entry_count = 0;
            if(LCDC_BGW_ENABLE){
                const u16 addr = LCDC_BG_MAP_AREA
                    +(ctx()->pfc.map_x/8)
                    +(32*(ctx()->pfc.map_y/8));
                ctx()->pfc.bgw_fetch_data[0] = bus_read(addr);
                if(0x8800 == LCDC_BGW_DATA_AREA){
                    ctx()->pfc.bgw_fetch_data[0]+=128;
                }
                pipeline_load_window_tile();
            }
            if(LCDC_OBJ_ENABLE && ctx()->line_sprites){
                pipeline_load_sprite_tile();
            }
            ctx()->pfc.cur_fetch_state = FS_DATA0;
            ctx()->pfc.fetch_x += 8;
        }break;
        case FS_DATA0:{
            const u16 addr = LCDC_BGW_DATA_AREA
                +(ctx()->pfc.bgw_fetch_data[0]*16)
                +ctx()->pfc.tile_y;
            ctx()->pfc.bgw_fetch_data[1] = bus_read(addr);
            pipeline_load_sprite_data(0);
            ctx()->pfc.cur_fetch_state = FS_DATA1;
        }break;
        case FS_DATA1:{
            const u16 addr = LCDC_BGW_DATA_AREA
                +(ctx()->pfc.bgw_fetch_data[0]*16)
                +ctx()->pfc.tile_y+1;
            ctx()->pfc.bgw_fetch_data[2] = bus_read(addr);
            pipeline_load_sprite_data(1);
            ctx()->pfc.cur_fetch_state = FS_IDLE;
        }break;
        case FS_IDLE:{ctx()->pfc.cur_fetch_state = FS_PUSH;}break;
        case FS_PUSH:{
            ctx()->pfc.cur_fetch_state = 
                pipeline_fifo_add()?FS_TILE:FS_PUSH;
        }break;
        default:NO_IMPL
    }
}
static void pipeline_push_pixel(){
    if(ctx()->pfc.pixel_fifo.size>8){
        u32 pixel_data = pixel_fifo_pop();
        if(ctx()->pfc.line_x>=(lctx()->scroll_x %8)){
            const int index = ctx()->pfc.pushed_x
            +(lctx()->ly*XRES);
            ctx()->video_buffer[index] = pixel_data;
            ctx()->pfc.pushed_x++;
        }
        ctx()->pfc.line_x++;
    }
}
void pipeline_process(){
    ctx()->pfc.map_y = (lctx()->ly+lctx()->scroll_y);
    ctx()->pfc.map_x = (ctx()->pfc.fetch_x+lctx()->scroll_x);
    ctx()->pfc.tile_y = ((lctx()->ly+lctx()->scroll_y)%8)*2;
    if(!(ctx()->line_ticks&1))pipeline_fetch();
    pipeline_push_pixel();
}
void pipeline_fifo_reset(){
    while(ctx()->pfc.pixel_fifo.size)pixel_fifo_pop();
    ctx()->pfc.pixel_fifo.head = 0;
}
bool window_visible(){
    return LCDC_WIN_ENABLE
        &&lctx()->win_x>=0 
        &&lctx()->win_x<=166 
        &&lctx()->win_y>=0
        &&lctx()->win_y<YRES;
}
#undef ctx
#undef lctx