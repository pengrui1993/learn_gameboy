#pragma once
#include<common.h>
#include<SDL.h>
#define AUDIO_BUF_SIZE 4096

typedef struct{
    SDL_AudioSpec wanted_spec;
    bool enabled;
    bool audio_running;
    Uint8 buf[AUDIO_BUF_SIZE];
    Uint8* audio_pos;
    u32 audio_len;
} apu_priv;
