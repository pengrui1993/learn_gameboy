#include<apu.h>
static apu_context* ctx(){return apu_get_context();}
static bool ignore_write = true;
typedef struct _record_entry{
    struct _record_entry *next;
    u16 addr;
    u8 value;
}record_entry;
typedef struct{
    bool init;
    int count;
    int last_action_count;
    int last_addr;
    int last_value;
    int entry_count;
    record_entry* head;
    record_entry* tail;
}audio_write_context;
static audio_write_context awc = {0};
void apu_write0(u16 address,u8 value){
    ctx()->ram[address-0xFF10] = value;
}
static void entry_push(u16 addr,u8 value){
    record_entry* p = malloc(sizeof(*p));
    p->next = NULL;
    p->addr = addr;
    p->value = value;
    if(NULL==awc.head){
        awc.head = awc.tail = p;
    }else{
        awc.tail->next = p;
        awc.tail = p;
    }
    awc.entry_count++;
}
static bool entry_pop(u16* addr,u8* value){
    if(awc.entry_count<=0)return false;
    record_entry* p = awc.head;
    awc.head = p->next;
    *addr = p->addr;
    *value = p->value;
    free(p);
    awc.entry_count--;
    if(0==awc.entry_count)awc.tail = NULL;
    return true;
}
static void data_pre_process(u16 address,u8 value){
    if(!BETWEEN(address,WAVE_RAM_START,WAVE_RAM_CLOSE)){apu_write0(address,value);return;}
    bool ch_enabled = ctx()->wc3.ctl.channel_enable&&ctx()->wc3.ctl.dac_enable;
    if(!ch_enabled){apu_write0(address,value); return;}
    u16 last_addr = ctx()->wc3.spe.last_read_addr;
    bool wave_ram_last_accessable = BETWEEN(last_addr,WAVE_RAM_START,WAVE_RAM_CLOSE);
    if(!(wave_ram_last_accessable&&ctx()->wc3.spe.ticks_since_read<2))return;
    apu_write0(last_addr,value);
}
static void data_post_process(u16 address,u8 value){
    entry_push(address,value);
    if(awc.count-awc.last_action_count>=100){
        u16 addr;
        u8 val;
        int loop = 0;
        printf("entry count:%d,count:%d\n"
            ,awc.entry_count,awc.count);
        while(entry_pop(&addr,&val)){
                printf("[%d,%04X,%02X] "
                ,++loop,address,value);
        }
        awc.last_action_count = awc.count;
    }
}

static void write_nrx4_len(bool trigger,bool enable){
    length_counter* lp = &ctx()->wc3.lc;
    const bool zero_len = 0 == lp->len;
    const bool enabled = lp->enabled;
    const bool less_half_len_div = lp->i<(len_div/2);
    const int full_len = lp->max_len;
    if(enabled&&zero_len&&trigger)
        lp->len = (enable && less_half_len_div)?(full_len-1):full_len;
    if(!enabled&&enable&&(zero_len&&trigger&&less_half_len_div))
        lp->len = full_len-1;
    if(!enabled&&!enable&&trigger&&zero_len)
        lp->len = full_len;
    if(!enabled&&enable&&!zero_len&&less_half_len_div)
        lp->len--;

    lp->enabled = enable;
}
#define WV_STAR WAVE_RAM_START
static void write_nr34(u8 value){
    bool trigger = !!(value&(1<<7));
    bool channel_enable = ctx()->wc3.ctl.channel_enable;
    bool dac_enable = ctx()->wc3.ctl.dac_enable;
    bool freq_eq_2 = 2==ctx()->wc3.div.freq_div;
    bool cond1 = trigger&&channel_enable&&dac_enable&&freq_eq_2;
    u32 pos = ctx()->wc3.div.i/2;
    bool pos_less_4 = pos<4;
    if(cond1&&pos_less_4)apu_write(WV_STAR,apu_read(WV_STAR+pos));
    if(cond1&&!pos_less_4){pos&=(~3);for(int j=0;j<4;j++)apu_write(WV_STAR+j,apu_read(WV_STAR+((pos+j)%0x10)));}
    write_nrx4_len(trigger,!!(value&(1<<6)));
    if(!trigger)return;
    ctx()->wc3.ctl.channel_enable = ctx()->wc3.ctl.dac_enable;
    ctx()->wc3.div.i = 0;
    ctx()->wc3.div.freq_div = 6;
    ctx()->wc3.spe.triggered = true;
}
static void write_nr44(u8 value){
    bool trigger = !!(value&(1<<7));
    write_nrx4_len(trigger,!!(value&(1<<6)));
    if(!trigger)return;
    ctx()->nc4.ctl.channel_enable = ctx()->nc4.ctl.dac_enable;
    ctx()->nc4.lfsr_bits = 0x7fff;
    ctx()->nc4.envelope.volume = ctx()->nc4.envelope.init_volume;
    ctx()->nc4.envelope.i = 0;
    ctx()->nc4.envelope.running = true;
}
static void write_nr12_4(u8 value,bool sweep,plus_channel* pch){
    bool trigger = !!(value&(1<<7));
    bool enable = !!(value&(1<<6));
    ch_ctl* pctl = &pch->ctl;
    write_nrx4_len(trigger,enable);
    if(!trigger)return;
    pctl->channel_enable = pctl->dac_enable;
    pch->div.i = 0;
    pch->div.freq_div = 1;
    pch->envelope.volume = pch->envelope.init_volume;
    pch->envelope.i = 0;
    pch->envelope.running = true;
    if(!sweep)return;
    const bool per_is_0 = 0==ctx()->fs.period;
    ctx()->fs.negate = false;
    ctx()->fs.negging = false;
    ctx()->fs.shadow_freq = ((apu_read(NR14)&0b111)<<8)|(apu_read(NR13));
    ctx()->fs.timer = per_is_0?8:ctx()->fs.period;
    ctx()->fs.counter_enable = !per_is_0 || (0!=ctx()->fs.shift);
    ctx()->fs.overflow = ctx()->fs.shift>0 && (FS_GET_NEXT_FREQ)>0x7ff;
}
static void write_nrx1(u8 value,length_counter* p){
    // u8 duty_c = (value)>>6;
    const int new_val = 64-(value&0b00111111);
    p->len = 0 == new_val?p->max_len:new_val;
}
static void write_nrx2(u8 value,volume_envelope *penv,ch_ctl* pcc){
    bool inc = !!(value&(1<<3));
    bool dac_enable = !!(value&0b11111000);
    penv->init_volume = ((value)>>4);
    penv->direction = inc?DIR_INC:DIR_DEC;
    penv->sweep = (value&0b0111);
    pcc->dac_enable = dac_enable;
    pcc->channel_enable = pcc->channel_enable&&dac_enable;
}
static void write_nr43(u8 value){
    static int divisor_map[] = {
        [0]=8
        ,[1]=16
        ,[2]=32
        ,[3]=48
        ,[4]=64
        ,[5]=80
        ,[6]=96
        ,[7]=112
    };
    int index = value&0b111;
    if(!BETWEEN(index,0,7))NO_IMPL
    int divisor = divisor_map[index];
    int clock_shift = value>>4;
    ctx()->nc4.pc.shifted_divisor = (divisor<<clock_shift);
    ctx()->nc4.pc.i = 1;
    ctx()->nc4.wm = (!!(value&(1<<3)))?BIT7:BIT15;
}
static void _inner_write(u16 address,u8 value){
    switch(address){
        case NR10:{//0xFF10
            //u8 pace = (value&0b01110000)>>4;
            u8 pace = (value>>4)&0b0111;
            bool dir_dec = !!(value&0b1000);//0:plus 1:minus
            u8 step = value&0b0111;
            ctx()->fs.period = pace;
            ctx()->fs.negate = dir_dec;
            ctx()->fs.shift = step;
            if(ctx()->fs.negging&&!ctx()->fs.negate)
                ctx()->fs.overflow = true;
        }break;
        case NR11:{write_nrx1(value,&ctx()->pc1.lc);}break;//0xFF11
        case NR12:{write_nrx2(value,&ctx()->pc1.envelope,&ctx()->pc1.ctl);}break;//0xFF12
        case NR13:{}break;//0xFF13
        case NR14:{write_nr12_4(value,true,&ctx()->pc1);}break;//0xFF14
        case NR21:{write_nrx1(value,&ctx()->pc2.lc);}break;
        case NR22:{write_nrx2(value,&ctx()->pc2.envelope,&ctx()->pc2.ctl);}break;
        case NR23:{}break;
        case NR24:{write_nr12_4(value,false,&ctx()->pc2);}break;
        case NR30:{
            bool dac_on = !!((1<<7)&value);
            ctx()->wc3.ctl.dac_enable = dac_on;
            ctx()->wc3.ctl.channel_enable = 
                ctx()->wc3.ctl.channel_enable &&dac_on;
        }break;//0xFF1A
        case NR31:{
            u8 v = 256-value;
            ctx()->wc3.lc.len = (0==v)?v:ctx()->wc3.lc.max_len;
        }break;//0xFF1B
        case NR32:{//0xFF1C
            //https://gbdev.io/pandocs/Audio_Registers.html#ff1c--nr32-channel-3-output-level
            ctx()->wc3.spe.volume = ((value>>5)&0b11);//output level
            // if(0!=ctx()->wc3.spe.volume )NO_IMPL
        }break;
        case NR33:{}break;//0xFF1D
        case NR34:{write_nr34(value);}break;//0xFF1E
        case NR41:{
            u8 v = 64-(value&0b00111111);
            ctx()->wc3.lc.len = (0==v)?v:ctx()->wc3.lc.max_len;
        }break;
        case NR42:{write_nrx2(value,&ctx()->nc4.envelope,&ctx()->nc4.ctl);}break;
        case NR43:{write_nr43(value);}break;
        case NR44:{write_nr44(value);}break;
        case NR50:{}break;//Master volume & VIN panning
        case NR51:{// 0xFF25
            ctx()->pc1.ctl.panning_right = !!((1)&value);
            ctx()->pc2.ctl.panning_right = !!((1<<1)&value);
            ctx()->wc3.ctl.panning_right = !!((1<<2)&value);
            ctx()->nc4.ctl.panning_right = !!((1<<3)&value);
            ctx()->pc1.ctl.panning_left = !!((1<<4)&value);
            ctx()->pc2.ctl.panning_left = !!((1<<5)&value);
            ctx()->wc3.ctl.panning_left = !!((1<<6)&value);
            ctx()->nc4.ctl.panning_left = !!((1<<7)&value);
        }break;
        case NR52:{//0xFF26
            ctx()->temp_master_enabled = ctx()->master_enabled;
            ctx()->master_enabled = !!((1<<7)&value);
            ctx()->pc1.ctl.channel_enable =  !(1&value);
            ctx()->pc2.ctl.channel_enable =  !((1<<1)&value);
            ctx()->wc3.ctl.channel_enable = !((1<<2)&value);
            ctx()->nc4.ctl.channel_enable = !((1<<3)&value);
        }break;
        default:{
            if(BETWEEN(address,WAVE_RAM_START,WAVE_RAM_CLOSE)
                ||(address==NR_FF15_NO_USED)
                ||(address==NR_FF1F_NO_USED)
            )break;//ignore
            printf("write unknown addr:%04X,val:%02X\n",address,value);
            NO_IMPL
        }break;
    }
}
void audio_write_process(u16 address,u8 value){
     if(!awc.init){
        awc.count = 0;
        awc.last_addr = 0;
        awc.last_value = 0;
        awc.last_action_count = 0;
        awc.init = true;
    } 
    awc.count++;
    if(ignore_write||1==awc.count){
        if(awc.count-awc.last_action_count>=100){
            #if defined SHOW_WRITE
            printf("ignore audio write ,count %d\n",awc.count);
            #endif
            awc.last_action_count = awc.count;
        }
    }
}
void apu_write(u16 address,u8 value){
    APU_ADDRESS_CHECK
    audio_write_process(address,value);
    data_pre_process(address,value);
    _inner_write(address,value);
    data_post_process(address,value);
}