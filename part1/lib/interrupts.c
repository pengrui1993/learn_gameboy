#include<interrupts.h>
#include<stack.h>
extern cpu_context ctx;

static void int_handle(u16 address){
    stack_push16(ctx.regs.pc);
    ctx.regs.pc = address;
}
static bool int_check(u16 address,interrput_type it){
    if(ctx.int_flags&it && ctx.ie_register&it){
        int_handle(address);
        ctx.int_flags&=~it;
        ctx.halted = false;
        ctx.int_master_enabled = false;
        return true;
    }
    return false;
}
void cpu_request_interrupt(interrput_type t){
   ctx.int_flags |= t;
}
void cpu_handle_interrputs(){
    if(int_check(0x40,IT_VBLANK)){}
    else if(int_check(0x48,IT_LCD_STAT)){}
    else if(int_check(0x50,IT_TIMER)){}
    else if(int_check(0x58,IT_SERIAL)){}
    else if(int_check(0x60,IT_JOYPAD)){}
}