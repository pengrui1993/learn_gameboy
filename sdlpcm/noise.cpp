#include <iostream>
#include <cmath>
#include <SDL.h>
#define HARF_SQ(n) powf(2,n/12.0)
#define TEST_BUF_SIZE 2048
#define MIN(a,b) ((a>b)?b:a)
#define SAMPLE_FREQ (44100/2)
#define PI 3.14159

static bool running = true;

using u8 = Uint8;
using u64 = uint64_t;
static u64 tick = 0;
static u8 noise(){
    static const int _8bit_max = (1<<8)-1;
    static int shift = 0;
    static int freq = 440*pow(2,shift/12.0);// static double omiga = 2*PI*freq; 
    double  val = 0.5*sin(2*PI *freq*  (tick++)*(1.0/SAMPLE_FREQ))+0.5;// [0,1]
    return (u8)(_8bit_max*val);
}
void fill_data_buffer(void *userData,Uint8 * stream,int len){
    static u8 buf[TEST_BUF_SIZE];
    static Uint8* p;
    SDL_memset(stream,0,len);
    if(0==len)return;
    int buf_len = TEST_BUF_SIZE;
    p = buf;
    len = MIN(buf_len,len);
    for(int i=0;i<len;i++)p[i]=noise();
    SDL_MixAudio(stream,p,len,SDL_MIX_MAXVOLUME);
}
int main(int argc,char** argv){
    if (SDL_Init(SDL_INIT_AUDIO)){
        std::cerr << "fail to initialize the SDL - " << SDL_GetError() << std::endl;
        return -1;
    }
    SDL_AudioSpec audioSpec;  
    audioSpec.channels = 2;
    audioSpec.format = AUDIO_U8;
    audioSpec.samples = 1024;
    audioSpec.silence = 0;
    audioSpec.freq = 44100;
    audioSpec.callback = fill_data_buffer;
    if (SDL_OpenAudio(&audioSpec,NULL) < 0){  //open
        printf("fail to open audio\n");
        return -1;
    }
    SDL_PauseAudio(0);//play
    while(running){
        std::printf("q for quit\n");
        int cmd;
        switch(cmd=std::cin.get()){
            case 'q':running = false;break;
            case '\n':break;// ignore newline
            default:
            std::cout <<"q for quit,continue."
                <<"current cmd:"<<(char)cmd
                <<",num:"<<cmd<< std::endl;
            break;
        }
    }
    SDL_Quit();
    return 0;
}
