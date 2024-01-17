#include<stdlib.h>
#include<SDL.h>
#include<apu.h>
#include<apu_priv.h>
#include<math.h>
#define AUDIO_FILE_NAME "./test.pcm"
#define SHOW_APU_READ

extern void fill_audio_callback_test(void*udata,Uint8*stream,int len);
extern void fill_audio_callback_real(void*udata,Uint8*stream,int len);
#define fill_audio_callback fill_audio_callback_real

extern u32 get_ticks();
static apu_context ctx = {0};
apu_priv pri = {0};
apu_context* apu_get_context(){return &ctx;}

void apu_set_enable(bool b){
    ctx.init_enabled = b;
}
static int buf_cur_len = 0;
#define AUDIO_SEND_CHECK_SUM {int sum = 0;for(int i=0;i<size;i++)sum+=buffer[i];if(0!=sum)printf("send check sum,%d\n",sum);}

static void show_first_no_zero_data(){
    for(int i=0;i<pri.audio_len;i++){
        if(0!=pri.audio_pos[i]&&i+1<i<pri.audio_len)
            printf("no zero audio data:%04X\n"
                ,(pri.audio_pos[i]<<8)|pri.audio_pos[i+1]);
    }
}
void send_audio_data(u16(*pop)(void),bool(*more)(void)){
    if(0!=pri.audio_len)return;//audio is reading,next tick try
    int index = buf_cur_len;
    int size = AUDIO_BUF_SIZE;
    u8* buffer = pri.buf;
    u16 sound;
    while(index<size&&more()){
        sound = pop();
        buffer[index++] = sound >>8;
        buffer[index++] = sound &0xff;
    }
    buf_cur_len = (size==index)?0:index;
    if(0!=buf_cur_len)return;//no full ,do not notify
    // AUDIO_SEND_CHECK_SUM
    pri.audio_pos = buffer;//size = index = AUDIO_BUF_SIZE
    pri.audio_len = size; //notify sound audio system play data
    // show_first_no_zero_data();
}
static void start_audio(){
    if(pri.audio_running)NO_IMPL
    if(SDL_OpenAudio(&pri.wanted_spec,NULL)<0)NO_IMPL
    SDL_PauseAudio(0);
    pri.audio_running = true;
}
// sdl2 maybe error header file AUDIO_U8 AUDIO_S8
#define FMT_U8 AUDIO_S8
void apu_init(){
    if(!ctx.init_enabled){ printf("apu disabled\n");return;}
    // pri.wanted_spec.freq = 44100;
    pri.wanted_spec.freq = 44100/2;
    // pri.wanted_spec.format = AUDIO_U8;
    pri.wanted_spec.format = FMT_U8;
    pri.wanted_spec.channels = 2;
    pri.wanted_spec.silence = 0;
    pri.wanted_spec.samples = 1024;
    pri.wanted_spec.callback = fill_audio_callback;

    ctx.temp_master_enabled = ctx.master_enabled = false;

    ctx.pc1.div.freq_div = 1;
    ctx.pc1.ctl.dac_enable = 1;
    ctx.pc1.ctl.channel_enable = 1;
    ctx.pc1.ctl.panning_left = true;
    ctx.pc1.ctl.panning_right = true;
    ctx.pc1.ctl.channel_enable = true;

    ctx.pc1.lc.max_len = 64;
    // ctx.pc1.lc.i = 0;
    // ctx.pc1.lc.len = 0;
    // ctx.pc1.lc.enabled = false;

    // ctx.pc1.envelope.init_volume = 0;
    // ctx.pc1.envelope.direction = DIR_INC;
    // ctx.pc1.envelope.sweep = 0;
    // ctx.pc1.envelope.volume = 0;
    // ctx.pc1.envelope.i = 0;
    ctx.pc1.envelope.running = true;

    ctx.pc2.div.freq_div = 1;
    ctx.pc2.ctl.dac_enable = 1;
    ctx.pc2.ctl.channel_enable = 1;
    ctx.pc2.ctl.panning_left = true;
    ctx.pc2.ctl.panning_right = true;
    ctx.pc2.ctl.channel_enable = true;
    ctx.pc2.lc.max_len = 64;
    // ctx.pc2.lc.i = 0;
    // ctx.pc2.lc.len = 0;
    // ctx.pc2.lc.enabled = false;
    ctx.pc2.envelope.running = true;

    ctx.wc3.lc.max_len = 256;
    ctx.wc3.spe.ticks_since_read = 65536;

    ctx.nc4.lc.max_len = 64;
    ctx.priv = &pri;
    start_audio();
    printf("apu init done...\n");
}

void apu_read_debug(u16 address){
#if defined SHOW_APU_READ0
    static int limit = 10000;
    static int last_action_count = 0;
    static int last_addr = 0;
    u32 count = get_ticks();
    if(count-last_action_count>=limit){
        printf("debug apu read,addr:%04X,value:%02X,count:%d\n"
            ,address,ctx.ram[address-0xFF10],count);
        last_action_count = count;
    }
    last_addr = address;
#endif
}

static u8 apu_read0(u16 address){
    return ctx.ram[address-0xFF10];
}
u8 apu_read(u16 address){
    APU_ADDRESS_CHECK
    apu_read_debug(address);
    if(!BETWEEN(address,WAVE_RAM_START,WAVE_RAM_CLOSE))return apu_read0(address);
    bool ch_enabled = ctx.wc3.ctl.channel_enable&&ctx.wc3.ctl.dac_enable;
    if(!ch_enabled)return apu_read0(address);
    u16 last_addr = ctx.wc3.spe.last_read_addr;
    bool wave_ram_last_accessable = BETWEEN(last_addr,WAVE_RAM_START,WAVE_RAM_CLOSE);
    if(wave_ram_last_accessable&&ctx.wc3.spe.ticks_since_read<2)return apu_read0(last_addr);
    return 0xff;
    
}

//******************
static void apu_destroy(){}
