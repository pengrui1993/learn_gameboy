#include <iostream>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <SDL.h>
#define TEST_BUF_SIZE 2048
#define MIN(a,b) ((a>b)?b:a)
#define SAMPLE_FREQ (44100/2)
#define PI 3.14159

using u8 = Uint8;
using u64 = uint64_t;
using Phrase = std::pair<char,u8>;
using Music = std::vector<Phrase>;
struct PlayerContext{
    Phrase* cur{nullptr};
    int cur_index{-1};
    int remain{-1};
    int cur_pitch;
};
const static u8 max_dur = 200;
const static u8 min_dur = 2;
static bool running = true;
static int count = 0;
static Uint8 * audioChunk;
static Uint32 audio_len;
static Uint8 *audio_pos;
static u64 tick = 0;
static const double init_freq = 220*2*(1/powf(2,3/12.0));
static double wave_freq = init_freq;
static u8 dur = 4;
static PlayerContext ctx;
static Music music{
     {1,2},{2,2},{3,2},{1,2}
    ,{1,2},{2,2},{3,2},{1,2}

    ,{3,2},{4,2},{5,4}
    ,{3,2},{4,2},{5,4}
    
    ,{5,1},{6,1},{5,1},{4,1},{3,2},{1,2}
    ,{5,1},{6,1},{5,1},{4,1},{3,2},{1,2}

    ,{3,2},{5,2},{1,4}
    ,{3,2},{5,2},{1,4}
};
static bool show_crab_noise = false;
static u8 noise(){
    // static double omiga = 2*PI*freq; 
    static const int _8bit_max = (1<<8)-1;
    const auto sample_param = (tick++)*(1.0/SAMPLE_FREQ);
    double val = 0.5*sin(2*PI * wave_freq * sample_param)+0.5;// [0,1]
    if(show_crab_noise)tick%=SAMPLE_FREQ;
    return (u8)(_8bit_max*val);
}
void fill_data_buffer(void *userData,Uint8 * stream,int len)
{
    static u8 buf[TEST_BUF_SIZE];
    static Uint8* p;
    SDL_memset(stream,0,len);
    if(0==audio_len)return;
    int buf_len = TEST_BUF_SIZE;
    p = buf;
    len = MIN(buf_len,len);
    for(int i=0;i<len;i++)p[i]=noise();
    SDL_MixAudio(stream,p,len,SDL_MIX_MAXVOLUME);
    audio_len--;
}

static void music_cmd(){
    while(true){
        std::printf("1. q for quit\n2. + or - change duration and replay\n3. other cmd will ignore\n");
        std::cout <<"4. c to continue" << std::endl;
        int res = std::cin.get();
        std::printf("cmd:%c,num:%d\n",res,res);
        switch(res){
            case 'q':running = false;return;
            case '+':
                dur = (dur+1)>max_dur?max_dur:dur+1;
                ctx.cur_index = -1;
                return;
            case '-':
                dur = (dur-1)<min_dur?min_dur:dur-1;
                ctx.cur_index = -1;
                return;
            case 'c':
                ctx.cur_index = -1;
                return;;
            case '\n':
                std::cout << "ignore newline " << std::endl;
                continue;
            default:
                std::printf("ignore unknow cmd and play\n");
                continue;
        }
    }
}
static void fetch_music(){
    switch(ctx.cur_index){
        case -1:{
            ctx.cur_index = 0;
            ctx.cur = &music[ctx.cur_index];
            ctx.remain = ctx.cur->second;
            ctx.cur_pitch = ctx.cur->first;
        }break;
        default:{
            ctx.cur = &music[ctx.cur_index];
            if(ctx.remain==0){
                if(++ctx.cur_index==music.size()){
                    std::cout << "music done" << std::endl;
                    music_cmd();
                    if(ctx.cur_index<0)return;
                }
                ctx.remain = music[ctx.cur_index].second;
                ctx.cur_pitch = music[ctx.cur_index].first;
            }else{
                while(audio_len > 0)SDL_Delay(1);
                static int map[] = {0,2,4,5,7,9,11};
                wave_freq = 440*pow(2,map[ctx.cur_pitch-1]/12.0);
                ctx.remain--;
                audio_len = dur;
            }
        }break;
    }
}
int main(int argc,char** argv)
{
    if (SDL_Init(SDL_INIT_AUDIO)){
        std::cerr << "fail to initialize the SDL - " << SDL_GetError() << std::endl;
        return -1;
    }

    //ffplay -f s16le -ac 2 -ar 44100 theshow.pcm
    SDL_AudioSpec audioSpec;//设置audioSpec
    audioSpec.channels = 2;
    audioSpec.format = AUDIO_U8;
    audioSpec.samples = 1024;
    audioSpec.silence = 0;
    audioSpec.freq = 44100;
    audioSpec.callback = fill_data_buffer;
  
    if (SDL_OpenAudio(&audioSpec,NULL) < 0){  //打开音频
        printf("fail to open audio\n");
        return -1;
    }
    SDL_PauseAudio(0);  //播放
    while(running)fetch_music();
    SDL_Quit(); //释放申请的资源
    return 0;
}
