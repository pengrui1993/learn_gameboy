#pragma once

#include <common.h>

static const int LINES_PER_FRAME = 154;
static const int TICKS_PER_LINE = 456;
static const int YRES = 144;
static const int XRES = 160;

typedef enum{
    FS_TILE,FS_DATA0,FS_DATA1,FS_IDLE,FS_PUSH
}fetch_state;
typedef struct _fifo_entry{
    struct _fifo_entry *next;
    u32 value;
} fifo_entry;
typedef struct{
    fifo_entry *head;
    fifo_entry *tail;
    u32 size;
}fifo;

typedef struct{
    fetch_state cur_fetch_state;
    fifo pixel_fifo;
    u8 line_x;
    u8 pushed_x;
    u8 fetch_x;

    u8 bgw_fetch_data[3];
    u8 fetch_entry_data[6];//oam data...
    u8 map_y;
    u8 map_x;
    u8 tile_y;
    u8 fifo_x;
}pixel_fifo_context;
//https://gbdev.io/pandocs/OAM.html#object-priority-and-conflicts

typedef struct{
    u8 y;
    u8 x;
    u8 tile;
    unsigned f_cb_pn:3;
    unsigned f_cgb_vram_bank:1;
    unsigned f_pn:1;
    unsigned f_x_flip:1;
    unsigned f_y_flip:1;
    unsigned f_bgp:1;
}oam_entry;

typedef struct _oam_line_entry{
    oam_entry entry;
    struct _oam_line_entry *next;
}oam_line_entry;
/*
Byte 3 — Attributes/Flags
7	6	5	4	3	2	1	0
Attributes	Priority	Y flip	X flip	DMG palette	Bank	CGB palette
Priority: 0 = No, 1 = BG and Window colors 1–3 are drawn over this OBJ
Y flip: 0 = Normal, 1 = Entire OBJ is vertically mirrored
X flip: 0 = Normal, 1 = Entire OBJ is horizontally mirrored
DMG palette [Non CGB Mode only]: 0 = OBP0, 1 = OBP1
Bank [CGB Mode Only]: 0 = Fetch tile from VRAM bank 0, 1 = Fetch tile from VRAM bank 1
CGB palette [CGB Mode Only]: Which of OBP0–7 to use
*/
typedef struct{
    oam_entry oam_ram[40];
    u8 vram[0x2000];
    pixel_fifo_context pfc;

    u8 line_sprite_count;//0 to 10 sprites.
    oam_line_entry *line_sprites;//lined list of current sprites on line;
    oam_line_entry line_entry_array[10];//memory to use for list.

    u8 fetched_entry_count;
    oam_entry fetched_entries[3];//entries fetched durating pipeline.

    u8 window_line;
    
    u32 current_frame;
    u32 line_ticks;
    u32 *video_buffer;
}ppu_context;
void ppu_init();
void ppu_tick();
void ppu_oam_write(u16 address,u8 value);
u8 ppu_oam_read(u16 address);
void ppu_vram_write(u16 address,u8 value);
u8 ppu_vram_read(u16 address);
ppu_context* ppu_get_context();