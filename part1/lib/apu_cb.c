#include<SDL.h>
#include<apu.h>
#include<apu_priv.h>
#include<stdlib.h>
extern apu_priv pri;
static apu_context* ctx(){ return apu_get_context();}
static double noise(double dTime){	
	double dOutput = sin(2.0 * 3.14159 * dTime);
	return dOutput * 0.5+0.5; // Master Volume
}
#define SAMPLE_FREQ (44100/2)
#define PI 3.14159
static u64 tick = 0;
static int freq = 220*2;
static u8 noise2(){
    // static const int freq = 1;
    // static double omiga = 2*PI*freq; 
    int _8bit_max = (1<<8)-1;
    double  val = 0.5*sin(2*PI *freq*  (tick++)*(1.0/SAMPLE_FREQ))+0.5;// [0,1]
    tick%=SAMPLE_FREQ;
    return (u8)(_8bit_max*val);
}
//ffplay -f s16le -ac 2 -ar 44100 theshow.pcm
#define TEST_BUF_SIZE 2048
void fill_audio_callback_test(void*udata,Uint8*stream,int len){
    static u8 buf[TEST_BUF_SIZE];
    static Uint8* p;
    SDL_memset(stream,0,len);
    if(0==pri.audio_len)return;
    int buf_len = TEST_BUF_SIZE;
    p = buf;
    len = MIN(buf_len,len);
    for(int i=0;i<len;i++)p[i]=noise2();
    SDL_MixAudio(stream,p,len,SDL_MIX_MAXVOLUME);

    pri.audio_len = 0;
}
#define final const
static final int start = 990;
static final int end = 1000;
static int sum = 0;
static int sum_tick=0;
static FILE* fp = 0;
static final int file_max_len = 1024*1024;
static int file_len = 0;
static final bool gen_file_flag = false;
static final bool sum_check_enable = true;
static void cb_debug_scope(int len){
    if(gen_file_flag){
        if(NULL == fp){
            fp = fopen("/tmp/gen.pcm","wb");
            if(NULL==fp) NO_IMPL
        }
        for(int i=0;i<len;i++){
            if(file_len++<file_max_len){
                fwrite(&pri.audio_pos[i],1,1,fp);
                if(file_max_len==file_len){
                    fclose(fp);
                    printf("file closed\n");
                }
            }
        }
    }
    for(int i=0;i<len;i++){
        if(sum_check_enable&&0!=pri.audio_pos[i]){
            if(sum_tick++<end&&sum_tick>=start){
                sum+=pri.audio_pos[i];
                printf("%d ",sum);
            }
            if(sum_tick==end)printf(",dbg scope done\n");
        }
    }
}
#define AUDIO_CB_CHECK_SUM {int sum = 0;for(int i=0;i<len;i++)sum+=pri.audio_pos[i];if(0!=sum)printf("callback check sum,%d\n",sum);}
void fill_audio_callback_real(void*udata,Uint8*stream,int len){
    SDL_memset(stream,0,len);
    if(0==pri.audio_len)return;
    // AUDIO_CHECK_SUM
    len = len >pri.audio_len? pri.audio_len: len;
    cb_debug_scope(len);
    SDL_MixAudio(stream,pri.audio_pos,len,SDL_MIX_MAXVOLUME);
    pri.audio_pos += len;
    pri.audio_len -= len;
}


static void write_nr14(u8 value){
    bool trigger = !!(value&(1<<7));
    bool enable = !!(value&(1<<6));
    
    // printf("write nr14\n,%d,%d",enable,trigger);
    const bool zero_len = 0 == ctx()->pc1.lc.len;
    const bool enabled = ctx()->pc1.lc.enabled;
    const bool less_half_len_div = ctx()->pc1.lc.i<(len_div/2);
    const int full_len = ctx()->pc1.lc.max_len;
    if(enabled&&zero_len&&trigger)
        ctx()->pc1.lc.len = (enable && less_half_len_div)?(full_len-1):full_len;
    if(!enabled&&enable&&(zero_len&&trigger&&less_half_len_div))
        ctx()->pc1.lc.len = full_len-1;
    if(!enabled&&!enable&&trigger&&zero_len)
        ctx()->pc1.lc.len = full_len;
    if(!enabled&&enable&&!zero_len&&less_half_len_div)
        ctx()->pc1.lc.len--;

    ctx()->pc1.lc.enabled = enable;

    if(!trigger)return;
    const bool per_is_0 = 0==ctx()->fs.period;
    ctx()->pc1.ctl.channel_enable = ctx()->pc1.ctl.dac_enable;
    ctx()->pc1.div.i = 0;
    ctx()->pc1.div.freq_div = 1;
    ctx()->pc1.envelope.volume = ctx()->pc1.envelope.init_volume;
    ctx()->pc1.envelope.i = 0;
    ctx()->pc1.envelope.running = true;
    ctx()->fs.negate = false;
    ctx()->fs.negging = false;
    ctx()->fs.shadow_freq = ((apu_read(NR14)&0b111)<<8)|(apu_read(NR13));
    ctx()->fs.timer = per_is_0?8:ctx()->fs.period;
    ctx()->fs.counter_enable = !per_is_0 || (0!=ctx()->fs.shift);
    ctx()->fs.overflow = ctx()->fs.shift>0 && (FS_GET_NEXT_FREQ)>0x7ff;
}
