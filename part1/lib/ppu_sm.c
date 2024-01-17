#include<ppu_sm.h>
#include<ppu.h>
#include<cpu.h>
#include<interrupts.h>
#include<lcd.h>
#include<common.h>
#include<string.h>
#include<cart.h>
//extern ppu_context ctx;
static ppu_context* ctx(){return ppu_get_context();}
static lcd_context* lctx(){return lcd_get_context();}
extern bool window_visible();
static void increment_ly(){
    if(window_visible()
        &&lctx()->ly>=lctx()->win_y
        &&lctx()->ly<lctx()->win_y+YRES){
        ctx()->window_line++;
    }
    lctx()->ly++;
    if(lctx()->ly==lctx()->ly_compare){
        LCDS_LYC_SET(1);
        if(LCDS_STAT_INT(SS_LYC)){
            cpu_request_interrupt(IT_LCD_STAT);
        }
    }else{
        LCDS_LYC_SET(0);
    }
}

static void load_line_sprites(){
    int cur_y = lctx()->ly;
    u8 sprite_height = LCDC_OBJ_HEIGHT;
    memset(ctx()->line_entry_array,0
        ,sizeof(ctx()->line_entry_array));
    for(int i=0;i<40;i++){
        oam_entry e = ctx()->oam_ram[i];
        if(!e.x)continue;//x=0 means not visible
        if(ctx()->line_sprite_count>=10)break;//max 10 sprites per line
        if(e.y<=cur_y+16 && e.y+sprite_height>cur_y+16){
            //this sprite is on the current line.
            oam_line_entry* entry = 
                &ctx()->line_entry_array
                    [ctx()->line_sprite_count++];
            entry->entry = e;
            entry->next = NULL;
            if(!ctx()->line_sprites
                ||ctx()->line_sprites->entry.x>e.x){
                entry->next = ctx()->line_sprites;
                ctx()->line_sprites = entry;
                continue;
            }
            //do some sorting...
            oam_line_entry *le = ctx()->line_sprites;
            oam_line_entry *prev = le;
            while(le){
                if(le->entry.x>e.x){
                    prev->next = entry;
                    entry->next = le;
                    break;
                }
                if(!le->next){
                    le->next = entry;
                    break;
                }
                prev = le;
                le = le->next;
            }
        }
    }
}
void ppu_mode_oam(){
    if(ctx()->line_ticks>=80){
        LCDS_MODE_SET(MODE_XFER);
        ctx()->pfc.cur_fetch_state = FS_TILE;
        ctx()->pfc.line_x = 0;
        ctx()->pfc.fetch_x = 0;
        ctx()->pfc.pushed_x = 0;
        ctx()->pfc.fifo_x = 0;
    }

    if(1== ctx()->line_ticks){
        //read oam on the first tick only
        ctx()->line_sprites = 0;
        ctx()->line_sprite_count = 0;
        load_line_sprites();
    }
}
extern void pipeline_process();
extern void pipeline_fifo_reset();
void ppu_mode_xfer(){
    //if(ctx()->line_ticks>=80+172)LCDS_MODE_SET(MODE_HBLANK);
    pipeline_process();
    if(ctx()->pfc.pushed_x>=XRES){
        pipeline_fifo_reset();
        LCDS_MODE_SET(MODE_HBLANK);
        if(LCDS_STAT_INT(SS_HBLANK)){
            cpu_request_interrupt(IT_LCD_STAT);
        }
    }
}
void ppu_mode_vblank(){
    if(ctx()->line_ticks>=TICKS_PER_LINE){
        increment_ly();
        if(lctx()->ly>=LINES_PER_FRAME){
            LCDS_MODE_SET(MODE_OAM);
            lctx()->ly = 0;
            ctx()->window_line = 0;
        }
        ctx()->line_ticks = 0;
    }

}
static u32 target_frame_time = 1000/60;
static long prev_frame_time = 0;
static long start_timer = 0;
static long frame_count = 0;
static void fps(){
    //calc fps...
    u32 end = get_ticks();
    u32 frame_time = end - prev_frame_time;
    if(frame_time<target_frame_time){
        delay(target_frame_time-frame_time);
    }
    if(end-start_timer>=1000){
        u32 fps = frame_count;
        start_timer = end;
        frame_count = 0;
        //printf("FPS: %d\n",fps);
        if(cart_need_save()){
            cart_battery_save();
        }
    }
    frame_count++;
    prev_frame_time = get_ticks();
}
void ppu_mode_hblank(){
    if(ctx()->line_ticks>=TICKS_PER_LINE){
        increment_ly();
        if(lcd_get_context()->ly>=YRES){
            LCDS_MODE_SET(MODE_VBLANK);
            cpu_request_interrupt(IT_VBLANK);
            if(LCDS_STAT_INT(SS_VBLANK)){
                cpu_request_interrupt(IT_LCD_STAT);
            }
            ctx()->current_frame++;
            fps();
        }else{
            LCDS_MODE_SET(MODE_OAM);
        }
        ctx()->line_ticks = 0;
    }
}