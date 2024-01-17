#include <timer.h>
#include <interrupts.h>
static timer_context ctx = {0};
void timer_init() {
    ctx.div = 0xAC00;
}
//https://gbdev.io/pandocs/Timer_and_Divider_Registers.html
void timer_tick() {
    u16 prev_div = ctx.div;
    ctx.div++;
    bool timer_update = false;
    switch(ctx.tac &(0b11)){//https://gbdev.io/pandocs/Timer_Obscure_Behaviour.html
        case 0b00:
            timer_update = (prev_div&(1<<9))&&(!(ctx.div&(1<<9)));
            break;
        case 0b01:
            timer_update = (prev_div&(1<<3))&&(!(ctx.div&(1<<3)));
            break;
        case 0b10:
            timer_update = (prev_div&(1<<5))&&(!(ctx.div&(1<<5)));
            break;
        case 0b11:
            timer_update = (prev_div&(1<<7))&&(!(ctx.div&(1<<7)));
            break;
    }
    if(timer_update && ctx.tac &(1<<2)){
        ctx.tima++;
        if(0xff == ctx.tima){
            ctx.tima = ctx.tma;
            cpu_request_interrupt(IT_TIMER);
        }
    }
}
typedef enum {
    TR_DIV = 0xFF04
    ,TR_TIMA = 0xFF05
    ,TR_TMA = 0xFF06
    ,TR_TAC = 0xFF07
}time_register;
void timer_write(u16 address,u8 value){
    switch(address){
        case TR_DIV://DIV
        ctx.div = 0;break;
        case TR_TIMA://TIMA
        ctx.tima = value;break;
        case TR_TMA://TMA
        ctx.tma = value;break;
        case TR_TAC://TAC
        ctx.tac = value;break;
    }
}
u8 timer_read(u16 address){
    switch(address){
    case TR_DIV://DIV
        return ctx.div>>8;
    case TR_TIMA://TIMA
        return ctx.tima;
    case TR_TMA://TMA
        return ctx.tma;
    case TR_TAC://TAC
        return ctx.tac;
    }
    NO_IMPL
}
timer_context* timer_get_context(){return &ctx;}
