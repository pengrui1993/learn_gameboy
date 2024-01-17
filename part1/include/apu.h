#pragma once

//https://nightshade256.github.io/2021/03/27/gb-sound-emulation.html
//https://gbdev.io/pandocs/Audio_Registers.html#audio-registers
//https://github.com/Malax/elmboy/tree/main/src/elm/Component/APU
//https://github.com/PumpMagic/ostrich/blob/master/gameboy/gameboy/Source/GameboyAPU/GameboyAPU.swift
//https://github.com/stoneface86/gbapu
//https://github.com/sysprog21/gameboy-emu/blob/master/apu.c
#include<common.h>
#define cpu_speed 4194304
#define sound_sample_speed 44100
#define len_div  (cpu_speed/256)
#define env_div  (cpu_speed/64)
#define sweep_div  (cpu_speed/128)
#define sounds_div  (cpu_speed/(sound_sample_speed/2))
typedef enum {
    NR10 = 0xFF10
    ,NR11 = 0xFF11
    ,NR12 = 0xFF12
    ,NR13 = 0xFF13 
    ,NR14 = 0xFF14
    ,NR_FF15_NO_USED = 0xFF15
    ,NR21 = 0xFF16
    ,NR22 = 0xFF17
    ,NR23 = 0xFF18
    ,NR24 = 0xFF19

    ,NR30 = 0xFF1A
    ,NR31 = 0xFF1B
    ,NR32 = 0xFF1C
    ,NR33 = 0xFF1D
    ,NR34 = 0xFF1E
    ,NR_FF1F_NO_USED = 0xFF1f
    ,NR41 = 0xFF20
    ,NR42 = 0xFF21
    ,NR43 = 0xFF22
    ,NR44 = 0xFF23

    ,NR50 = 0xFF24
    ,NR51 = 0xFF25
    ,NR52 = 0xFF26

    ,WAVE_RAM_START = 0xff30
    ,WAVE_RAM_CLOSE = 0xff3f    
}apu_addr;

void apu_set_enable(bool b);
void apu_init();
void apu_tick();
void apu_write(u16 address,u8 value);
u8 apu_read(u16 address);

typedef enum{
    DIR_INC,DIR_DEC
}Driection;

typedef struct{
    u8 init_volume;
    u8 volume;
    int i;
    Driection direction;
    u8 sweep;
    bool running;
}volume_envelope;
typedef struct{
    bool channel_enable;
    bool dac_enable;
    bool panning_left;
    bool panning_right;
}ch_ctl;
typedef struct{
    bool enabled;
    u32 len;
    int max_len;
    int i;
}length_counter;
typedef struct{
    u32 freq_div;
    u32 i;
}diver;
typedef struct {
    ch_ctl ctl;
    diver div;
    u16 fetched_sound;
    length_counter lc;
    volume_envelope envelope;
    u8 last_output;
}plus_channel;
typedef struct{
    u8 buffer;
    bool triggered;
    u16 last_read_addr;
    int ticks_since_read;
    u8 volume;
}wave_spec; 
typedef struct {
    int wave_pos;//wave position
    ch_ctl ctl;
    u16 fetched_sound;
    diver div;
    wave_spec spe;
    length_counter lc;
    int last_output;
}wave_channel;
typedef enum{
    BIT15,BIT7
}width_mode;
typedef struct{
    int i;
    int shifted_divisor;
} poly_counter;
typedef struct {
    volume_envelope envelope;
    width_mode wm;
    ch_ctl ctl;
    u16 fetched_sound;
    length_counter lc;
    int lfsr_bits;//linear-feedback shift register
    poly_counter pc;
    u8 last_output;
}noise_channel;

//https://avmedia.0voice.com/?id=43026
//https://gbdev.io/pandocs/Audio_Registers.html#sound-channel-1--pulse-with-period-sweep
typedef struct{
    u8 period;//3 bit
    bool negate;//1 bit
    u8 shift;//3 bit
    int timer;
    int shadow_freq;
    u32 i;
    bool overflow;
    bool counter_enable;
    bool negging;
}fre_sweep;
typedef struct{
    int ticks;
    void* priv;
}sounds_device;
typedef struct {
    bool init_enabled;
    sounds_device device;
    void* priv;
    //apu enumlate context
    bool master_enabled;
    bool temp_master_enabled;
    u32 freq_divider;
    fre_sweep fs;
    plus_channel pc1;//pluse channel
    plus_channel pc2;
    wave_channel wc3;//wave channel
    noise_channel nc4;//noise channel
    u8 ram[0xFF40-0xFF10];
    u16 fetch_mixed_data;
}apu_context;
apu_context* apu_get_context();
#define FS_GET_NEXT_FREQ apu_get_context()->fs.shadow_freq \
    +(apu_get_context()->fs.negate \
    ?(-(apu_get_context()->fs.shadow_freq>>apu_get_context()->fs.shift)) \
    :(apu_get_context()->fs.shadow_freq>>apu_get_context()->fs.shift))
void send_audio_data(u16(*pop)(void),bool(*ctn)(void));
#define APU_ADDRESS_CHECK if(!BETWEEN(address,0xff10,0xff3f))NO_IMPL
/*
    cpu cycle
*/
/*
sample : pulse channel -> float
sample channel = 
    case array.get channel.dutyCyclePosition dutyCycle1 
        of Just precalculatedSample ->
            precalculatedSample * channel.volume

sample :noise channel -> float
sample channel = channel.currentOutput * channel.volume

mix samples: list float -> float
mix samples samples = 
    let 
        sum = list.sum samples
        length = list.length samples
    in
        sum/ to float length

mix samples
[pluse channel.sample update channel 1
,pluse channel.sample update channel 2
,wave channel.sample update channel 3
,noise channel.sample update channel 4
]
{apu|audio buffer = mixed sample :: apu.audio buffer}
*/
