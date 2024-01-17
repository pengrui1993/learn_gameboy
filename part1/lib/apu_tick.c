
#include<apu.h>
#include<stdlib.h>
extern u32 get_ticks();

typedef struct _sound_entry{
    struct _sound_entry*next;
    u16 value;
}sound_entry;
typedef struct{
    sound_entry* head;
    sound_entry* tail;
    int size;
}sound_context;
static sound_context sctx = {0};
static apu_context* ctx(){
    return apu_get_context();
}
static bool enable_limit = true;
static u32 push_data_limit = 1024*1024*1024;
#define LIMIT_PUSH if(sctx.size>=push_data_limit&&enable_limit)NO_IMPL

static void push_data(u16 sound){
    u32 now = get_ticks();
    sound_entry* p = malloc(sizeof(*p));
    p->next = NULL;
    p->value = sound;
    if(0==sctx.size){
        sctx.head = sctx.tail = p;
    }else{
        sctx.tail->next = p;
        sctx.tail = p;
    }
    ++sctx.size;
    LIMIT_PUSH
}
static u16 pop_data(){
    sound_entry* p = sctx.head;
    u16 temp = p->value;
    sctx.head = p->next;
    free(p);
    if(0==--sctx.size)sctx.tail = NULL;
    return temp;
}


static void envelope_tick(u8 ch){
    volume_envelope* p;
    volume_envelope* all[] = {
         &ctx()->pc1.envelope
        ,&ctx()->pc2.envelope
        ,NULL
        ,&ctx()->nc4.envelope
    };
    p = all[ch];
    if(!p)NO_IMPL
    if(!p->running)return;
    if(p->direction==DIR_INC&&p->volume==15)p->running = false;
    if(p->direction==DIR_DEC&&p->volume==0)p->running = false;
    if(!p->running)return;
    if(++p->i!=(p->sweep*env_div))return;
    p->i = 0;
    p->volume+=(p->direction == DIR_INC?1:-1);
}
static void counter_len_tick(u8 ch){
    length_counter* p;
    switch(ch){
        case 0:{p = &ctx()->pc1.lc;}break; //channel 1
        case 1:{p = &ctx()->pc2.lc;}break; //channel 2
        case 2:{p = &ctx()->wc3.lc;}break; //channel 3
        case 3:{p = &ctx()->nc4.lc;}break; //channel 3
        default: NO_IMPL
    }
    if(++p->i!=len_div)return;
    p->i = 0;
    if(p->len<=0)return;
    if(!p->enabled)return;
    p->len--;
}
static void sweep_tick(){
    if(++ctx()->fs.i != sweep_div)return;
    ctx()->fs.i = 0;
    if(!ctx()->fs.counter_enable)return;
    if(0!=--ctx()->fs.timer)return;//dec timer
    const bool per_is_0 = 0==ctx()->fs.period;
    ctx()->fs.timer = per_is_0 ? 8 :ctx()->fs.period;
    if(per_is_0)return;
    int new_freq = FS_GET_NEXT_FREQ;
    if(ctx()->fs.negate)ctx()->fs.negging = true;
    if(new_freq>0x7ff)ctx()->fs.overflow = true;
    if(ctx()->fs.overflow||0==ctx()->fs.shift)return;
    ctx()->fs.shadow_freq = new_freq;
    apu_write(NR13,new_freq&0xff);
    apu_write(NR14,(new_freq&0x700)>>8);
    if(FS_GET_NEXT_FREQ>0x7ff)ctx()->fs.overflow = true;
}

static void 
ch124_set_last_output(
u16* fetched_sound
,u8 last_output
,ch_ctl* ccp
,u8 vol){
    u16 sound = 0;
    u8 val = (u8)(last_output*vol);
    if(ccp->panning_left) sound |= (val<<8);
    if(ccp->panning_right) sound |= val;
    *fetched_sound = sound;
}
static void ch12_set_last_output(u8 ch){
    plus_channel* p = (0==ch)?&ctx()->pc1:&ctx()->pc2;
    ch124_set_last_output(&p->fetched_sound
        ,p->last_output
        ,&p->ctl
        ,p->envelope.volume);
}
static void ch3_set_last_output(){
    wave_channel* p = &ctx()->wc3;
    ch124_set_last_output(&p->fetched_sound
        ,p->last_output
        ,&p->ctl
        ,p->spe.volume);
    // u16 sound = 0;
    // u8 val = (u8)(ctx()->wc3.last_output*ctx()->wc3.spe.volume);
    // if(ctx()->wc3.ctl.panning_left) sound |= (val<<8);
    // if(ctx()->wc3.ctl.panning_right) sound |= val;
    // ctx()->wc3.fetched_sound = sound;
}
static u32 ch12_gen_tick = 0;

#define CH12_DBG_INFO {u32 now = get_ticks();if(now-ch12_gen_tick>10000){ printf("output channel %d,sound:%04X,vol:%d\n",ch+1,ppc->fetched_sound,vep->volume);ch12_gen_tick = now;}}

static void ch12_tick(u8 ch){
    if(ch>1)NO_IMPL
    bool ch1 = 0==ch;
    envelope_tick(ch);
    counter_len_tick(ch);
    if(ch1)sweep_tick();
    plus_channel* ppc = (ch1)?&ctx()->pc1:&ctx()->pc2;
    length_counter* lp = &ppc->lc;
    ch_ctl* ccp = &ppc->ctl;
    volume_envelope* vep = &ppc->envelope;
    if(ch1&&ccp->channel_enable&&((ctx()->fs.overflow||lp->enabled&&0==lp->len)))
        ccp->channel_enable = false;//TODO ram sync
    if(!ch1&&ccp->channel_enable&&lp->enabled&&0==lp->len)
        ccp->channel_enable = false;
    const bool e = ccp->channel_enable && ccp->dac_enable;
    if(!e){ppc->fetched_sound = 0;return;}
    if(0!=(--ppc->div.freq_div)){ch12_set_last_output(ch);return;}
    u8 lo = apu_read(ch1?NR13:NR23);
    u8 hi = (apu_read(ch1?NR14:NR24)&0b111);
    ppc->div.freq_div = (2048-((hi<<8)|lo))*4;
    //https://gbdev.io/pandocs/Audio_Registers.html#ff11--nr11-channel-1-length-timer--duty-cycle
    static u8 duty_tab[]= {0b00000001,0b10000001,0b10000111,0b01111110};
    ppc->last_output = (duty_tab[apu_read(ch1?NR11:NR21)>>6] & (1<<ppc->div.i))>>ppc->div.i;
    ppc->div.i = (ppc->div.i+1)%8;
    ch12_set_last_output(ch);
    CH12_DBG_INFO
}
static u8 ch3_get_wave_entry(){
    ctx()->wc3.spe.ticks_since_read = 0;
    ctx()->wc3.spe.last_read_addr = 0xff30 + ctx()->wc3.div.i/2;
    if(0==ctx()->wc3.spe.volume)return 0;
    switch(ctx()->wc3.spe.volume){
        case 0b01:
        case 0b10:
        case 0b11:{
            u8 buffer = ctx()->wc3.spe.buffer 
                = apu_read(ctx()->wc3.spe.last_read_addr);
            bool even = (0==(ctx()->wc3.div.i%2));
            u8 shift = ctx()->wc3.spe.volume - 1;//shift:[0,2]
            if(!BETWEEN(shift,0,2))NO_IMPL
            u8 value = even?((buffer>>4)&0x0f):(buffer&0x0f);
            return value>>shift;
        }
        default:NO_IMPL
    }
}
static void ch3_tick(){
    wave_channel* ppc =&ctx()->wc3;
    ppc->spe.ticks_since_read++;
    counter_len_tick(2);
    length_counter* lp = &ppc->lc;
    ch_ctl* ccp = &ppc->ctl;
    if(ccp->channel_enable&&lp->enabled&&0==lp->len)
        ccp->channel_enable = false;
    const bool e = ccp->channel_enable && ccp->dac_enable;
    if(!e){ppc->fetched_sound = 0;return;}
    if(0!=(--ppc->div.freq_div)){ch3_set_last_output();return;}
    u8 lo = apu_read(NR33);
    u8 hi = (apu_read(NR34)&0b111);
    ppc->div.freq_div = (2048-((hi<<8)|lo))*2;
    if(ppc->spe.triggered){ppc->spe.triggered = false;ppc->last_output = (ppc->spe.buffer>>4);}
    if(!ppc->spe.triggered)ppc->last_output=ch3_get_wave_entry();
    ppc->div.i = (ppc->div.i+1)%32;
    ch3_set_last_output();
}
static void ch4_set_last_output(){
    noise_channel* p=&ctx()->nc4;
    ch124_set_last_output(
        &p->fetched_sound
        ,p->last_output
        ,&p->ctl
        ,p->envelope.volume);
}
static u8 lfsr_bit_op(){
    noise_channel* ppc = &ctx()->nc4;
    bool mode7 = ppc->wm == BIT7;
    bool x = !!((ppc->lfsr_bits & 1)^((ppc->lfsr_bits & 2)>>1));
    ppc->lfsr_bits >>=1;
    ppc->lfsr_bits |=(x?(1<<14):0);
    if(mode7)ppc->lfsr_bits|=(x?(1<<6):0);
    return 1 & ~ppc->lfsr_bits;
}
static void ch4_tick(){
    envelope_tick(3);
    counter_len_tick(3);
    noise_channel* ppc = &ctx()->nc4;
    length_counter* lp = &ppc->lc;
    ch_ctl* ccp = &ppc->ctl;
    if(ppc->ctl.channel_enable&&lp->enabled&&0==lp->len)ppc->ctl.channel_enable = false;
    const bool e = ccp->channel_enable && ccp->dac_enable;
    if(!e){ppc->fetched_sound = 0;return;}
    if(0!=--ppc->pc.i){ch4_set_last_output();return;}
    ppc->pc.i = ppc->pc.shifted_divisor;
    ppc->last_output = lfsr_bit_op();
    ch4_set_last_output();
}

static bool more_data(){
    return sctx.size>0;
}
static const u32 dbg_start = 9990;
static const u32 dbg_end = 10000;
static int showed_count = 0;
static const bool show_debug = false;
static void debug_put_sound(int sound,int left_vol,int right_vol){
    if(show_debug&&ctx()->device.ticks+1==sounds_div&&0!=sound
        &&(showed_count++<dbg_end)&&(showed_count>=dbg_start)){
        printf("mixed sound:%04X,vl:%d,vr:%d\n"   //normal:0E 2A
            ,sound
            ,left_vol,right_vol);
    }
    u32 now = get_ticks();
    static u32 last_sound_tick = 0;
    if(0!=sound &&now-last_sound_tick>1000){
        last_sound_tick = now;
    }
}
#define act_ch 4
#define LMOD (left_vol/act_ch)
#define RMOD (right_vol/act_ch)
static void mixer_sounds(){
    u16 left = 0,right = 0;
    u8 left_vol = ((apu_read(NR50)>>4)&0b111);
    u8 right_vol = (apu_read(NR50)&0b111);
    left+=((ctx()->pc1.fetched_sound>>8)*LMOD);
    right+=((ctx()->pc1.fetched_sound&0xff)*RMOD);
    left+=((ctx()->pc2.fetched_sound>>8)* LMOD);
    right+=((ctx()->pc2.fetched_sound&0xff)*RMOD);
    left+=((ctx()->wc3.fetched_sound>>8)*LMOD);
    right+=((ctx()->wc3.fetched_sound&0xff)*RMOD);
    left+=((ctx()->nc4.fetched_sound>>8)*LMOD);
    right+=((ctx()->nc4.fetched_sound&0xff)*RMOD);
    u16 sound = (left<<8)|((right)&0xff);
    debug_put_sound(sound,left_vol,right_vol);
    ctx()->fetch_mixed_data = sound;
}
#define AUDIO_START_ADDR 0xff10
static void audio_start(){
    for(u16 addr = AUDIO_START_ADDR;addr<=0xff25;addr++){
        switch(addr){
            case NR11://channel 1,2,3,4
            case NR21:
            case NR31:
            case NR41:continue;
            default: apu_write(addr,0);break;
        }
    }
    apu_write(NR11,0b00111111& apu_read(NR11));
    apu_write(NR21,0b00111111& apu_read(NR21));
    apu_write(NR41,0b00111111& apu_read(NR21));
    apu_write(NR31, apu_read(NR31));
    //channe 1
    ctx()->pc1.div.i = 0;
    ctx()->pc1.lc.i = 8192;
    ctx()->fs.i = 8192;
    ctx()->fs.counter_enable = false;
    ctx()->pc1.envelope.running = false;
    ctx()->pc1.envelope.i = 8192;
    //channel 2
    ctx()->pc2.div.i = 0;
    ctx()->pc2.lc.i = 8192;
    ctx()->pc2.envelope.running = false;
    ctx()->pc2.envelope.i = 8192;
    //channel 3
    ctx()->wc3.div.i = 0;
    ctx()->wc3.spe.buffer = 0;
    ctx()->wc3.lc.i = 8192;
    //channel 4
    ctx()->nc4.lc.i = 8192;
    ctx()->nc4.lfsr_bits = 0x7fff;
    ctx()->nc4.envelope.running = false;
    ctx()->nc4.envelope.i = 8192;
}
static void audio_stop(){
    ctx()->pc1.ctl.channel_enable = false;
    //channel 2,3,4 TODO
    ctx()->pc2.ctl.channel_enable = false;
    ctx()->wc3.ctl.channel_enable = false;
    ctx()->nc4.ctl.channel_enable = false;
}
static void on_off_logic(){
    if(ctx()->temp_master_enabled==ctx()->master_enabled)return;
    if(ctx()->master_enabled)audio_start();
    if(!ctx()->master_enabled)audio_stop();
    ctx()->temp_master_enabled=ctx()->master_enabled;
}
static void audio_device_tick(){
    if(++ctx()->device.ticks!=sounds_div)return;
    ctx()->device.ticks = 0;
    push_data(ctx()->fetch_mixed_data);
    send_audio_data(pop_data,more_data);
}
void apu_tick(){
    on_off_logic();
    if(!ctx()->master_enabled){
        ctx()->fetch_mixed_data = 0;
    }else{
        ch12_tick(0);//channal 1
        ch12_tick(1);//channal 2
        ch3_tick();
        ch4_tick();
        mixer_sounds();
    }
    audio_device_tick();
}
