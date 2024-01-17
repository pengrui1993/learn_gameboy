#include<cpu.h>
#include<bus.h>
extern cpu_context ctx;

static u16 reverse(u16 n){
    return ((n&0xff00)>>8)|((n&0x00ff)<<8);
}
u16 cpu_read_reg(reg_type rt){
    switch (rt)
    {
    case RT_A:return ctx.regs.a;
    case RT_F:return ctx.regs.f;
    case RT_B:return ctx.regs.b;
    case RT_C:return ctx.regs.c;
    case RT_D:return ctx.regs.d;
    case RT_E:return ctx.regs.e;
    case RT_H:return ctx.regs.h;
    case RT_L:return ctx.regs.l;
    //bc    b(hi) c(lo) ,&b b is lower ,then reverse
    case RT_AF:return reverse(*((u16*)&ctx.regs.a));
    case RT_BC:return reverse(*((u16*)&ctx.regs.b));
    case RT_DE:return reverse(*((u16*)&ctx.regs.d));
    case RT_HL:return reverse(*((u16*)&ctx.regs.h));

    case RT_PC:return ctx.regs.pc;
    case RT_SP:return ctx.regs.sp;
    default:
        return 0;
    }
}
void cpu_set_reg(reg_type rt,u16 val){
    switch(rt){
        case RT_A:ctx.regs.a = val&0xff;break;
        case RT_F:ctx.regs.f = val&0xff;break;
        case RT_B:ctx.regs.b = val&0xff;break;
        case RT_C:ctx.regs.c = val&0xff;break;
        case RT_D:ctx.regs.d = val&0xff;break;
        case RT_E:ctx.regs.e = val&0xff;break;
        case RT_H:ctx.regs.h = val&0xff;break;
        case RT_L:{
            ctx.regs.l = val&0xff;break;
        }
        case RT_AF:*((u16*)&ctx.regs.a) = reverse(val);break;
        case RT_BC:*((u16*)&ctx.regs.b) = reverse(val);break;
        case RT_DE:*((u16*)&ctx.regs.d) = reverse(val);break;
        case RT_HL:{
            *((u16*)&ctx.regs.h) = reverse(val);break;
        }
        case RT_PC:ctx.regs.pc = val;break;
        case RT_SP:ctx.regs.sp = val;break;

        case RT_NONE:break;
        default:
            NO_IMPL
    }
}

void cpu_set_flags(char z,char n,char h,char c){
    if(-1!=z)BIT_SET(ctx.regs.f,7,z)
    if(-1!=n)BIT_SET(ctx.regs.f,6,n)
    if(-1!=h)BIT_SET(ctx.regs.f,5,h)
    if(-1!=c)BIT_SET(ctx.regs.f,4,c)
}
void cpu_set_ie_register(u8 value){
    ctx.ie_register = value;
}
u8 cpu_get_ie_register(){
    return ctx.ie_register;
}
cpu_registers* cpu_get_regs(){
    return &ctx.regs;
}

u8 cpu_read_reg8(reg_type rt){

    switch (rt){
        case RT_A:return ctx.regs.a;
        case RT_F:return ctx.regs.f;
        case RT_B:return ctx.regs.b;
        case RT_C:return ctx.regs.c;
        case RT_D:return ctx.regs.d;
        case RT_E:return ctx.regs.e;
        case RT_H:return ctx.regs.h;
        case RT_L:return ctx.regs.l;
        case RT_HL:return bus_read(cpu_read_reg(RT_HL));
        default:
            printf("**ERR INVALID READ REG8:%d\n",rt);
            NO_IMPL
    }
}
void cpu_set_reg8(reg_type rt,u8 val){
    switch (rt){
        case RT_A: ctx.regs.a = val&0xff;return;
        case RT_F: ctx.regs.f = val&0xff;return;
        case RT_B: ctx.regs.b = val&0xff;return;
        case RT_C: ctx.regs.c = val&0xff;return;
        case RT_D: ctx.regs.d = val&0xff;return;
        case RT_E: ctx.regs.e = val&0xff;return;
        case RT_H: ctx.regs.h = val&0xff;return;
        case RT_L: ctx.regs.l = val&0xff;return;
        case RT_HL: bus_write(cpu_read_reg(RT_HL),val); return;
        default:
            printf("**ERR INVALID SET REG8:%d\n",rt);
            NO_IMPL
    }
}
static reg_type rt_lookup[] = {
    RT_B,
    RT_C,
    RT_D,
    RT_E,
    RT_H,
    RT_L,
    RT_HL,
    RT_A
};
reg_type decode_reg(u8 reg){
    if(reg>0b111)return RT_NONE;
    return rt_lookup[reg];
}

u8 cpu_get_int_flags(){
    return ctx.int_flags;
}
void cpu_set_int_flags(u8 value){
    ctx.int_flags = value;
}