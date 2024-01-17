#include<cpu.h>
#include<instructions.h>
#include<emu.h>
#include<bus.h>
#include<stack.h>
static bool ignore_none = false;
//extern cpu_context ctx;
static void proc_none(cpu_context*ctx){
    if(ignore_none)
        return;
    //ctx->cur_inst->type
    printf("INVALID INSTRUCTION!\n");
    printf("opcode:%02X,%02X:%s\n",ctx->cur_opcode
        ,ctx->cur_inst->type
        ,inst_name(ctx->cur_inst->type));
    exit(-7);
}
static void proc_nop(cpu_context*ctx){}

static void proc_di(cpu_context*ctx){
    ctx->int_master_enabled = false;
}
static void proc_ei(cpu_context*ctx){
    ctx->enabling_ime = true;
}
static void proc_ldh(cpu_context*ctx){
    if(RT_A==ctx->cur_inst->reg_1){
        cpu_set_reg(ctx->cur_inst->reg_1
            ,bus_read(0xFF00|ctx->fetched_data));
    }else{
        // bus_write(0xFF00|ctx->mem_dest,ctx->regs.a);
        bus_write(ctx->mem_dest,ctx->regs.a);
    }
    emu_cycles(1);
}
static void proc_xor(cpu_context*ctx){
    //ctx->regs.a ^= ctx->fetched_data //TO SEE LATER
    ctx->regs.a ^= ctx->fetched_data & 0xff;
    cpu_set_flags(0==ctx->regs.a,0,0,0);
}
static void proc_cmp(cpu_context*ctx){
    int n = (int)ctx->regs.a-(int)ctx->fetched_data;
    char half_c = (((int)ctx->regs.a & 0x0f)-((int)ctx->fetched_data&0x0f))<0;
    cpu_set_flags(0==n,1,half_c,n<0);
}
static void proc_and(cpu_context*ctx){
    ctx->regs.a &=ctx->fetched_data;
    cpu_set_flags(0==ctx->regs.a,0,1,0);
}
static void proc_or(cpu_context*ctx){
    ctx->regs.a |= ctx->fetched_data & 0xff;
    cpu_set_flags(0==ctx->regs.a,0,0,0);
}
static void proc_pop(cpu_context*ctx){
    u16 lo = stack_pop();
    emu_cycles(1);
    u16 hi = stack_pop();
    emu_cycles(1);
    u16 n = (hi<<8)|lo;
    cpu_set_reg(ctx->cur_inst->reg_1,n);
    if(RT_AF==ctx->cur_inst->reg_1){
        cpu_set_reg(ctx->cur_inst->reg_1,n&0xFFF0);
    }
}
static void proc_push(cpu_context*ctx){
    u16 hi = (cpu_read_reg(ctx->cur_inst->reg_1)>>8)&0xFF;
    emu_cycles(1);
    stack_push(hi);
    // u16 lo = cpu_read_reg(ctx->cur_inst->reg_2)&0xFF;
    u16 lo = cpu_read_reg(ctx->cur_inst->reg_1)&0xFF;
    emu_cycles(1);
    stack_push(lo);
    emu_cycles(1);
}
static bool is_16_bit(reg_type rt){
    return rt>=RT_AF;
}
static void proc_ld(cpu_context*ctx){
    if(ctx->dest_is_mem){//LD (BC),A
        if(is_16_bit(ctx->cur_inst->reg_2)){
            emu_cycles(1);
            bus_write16(ctx->mem_dest,ctx->fetched_data);
        }else{
            bus_write(ctx->mem_dest,ctx->fetched_data);
            // bus_write(ctx->mem_dest,ctx->fetched_data&0x00ff);//TODO
        }
        emu_cycles(1);
        return;
    }
    if(AM_HL_SPR==ctx->cur_inst->mode){//LD HL,SP+r8 [00HC]
        //TO comprehend
        u8 hflag = (cpu_read_reg(ctx->cur_inst->reg_2)&0xf)
            +(ctx->fetched_data&0xf)>=0x10;
        u8 cflag = (cpu_read_reg(ctx->cur_inst->reg_2)&0xff)
            +(ctx->fetched_data&0xff)>=0x100;
        cpu_set_flags(0,0,hflag,cflag);
        cpu_set_reg(ctx->cur_inst->reg_1
            ,cpu_read_reg(ctx->cur_inst->reg_2)
                +(int8_t)ctx->fetched_data //pos or neg
        );
        return;
    }
    cpu_set_reg(ctx->cur_inst->reg_1,ctx->fetched_data);
}
static bool check_cond(cpu_context* ctx){
    bool z = CPU_FLAG_Z;
    bool c = CPU_FLAG_C;
    switch(ctx->cur_inst->cond){
        case CT_NONE: return true;
        case CT_C: return c;
        case CT_NC: return !c;
        case CT_Z: return z;
        case CT_NZ: return !z;
    }
    return false;
}
static void goto_addr(cpu_context*ctx,u16 addr,bool pushpc){
    if(check_cond(ctx)){
        if(pushpc){
            emu_cycles(2);
            stack_push16(ctx->regs.pc);
        }
        ctx->regs.pc = addr;
        emu_cycles(1);
    }else{
        //printf("goto_addr(),cond=false\n");
        // NO_IMPL
    }
}
static void proc_jp(cpu_context*ctx){
    goto_addr(ctx,ctx->fetched_data,false);
}
static void proc_call(cpu_context*ctx){
    goto_addr(ctx,ctx->fetched_data,true);
}
static void proc_rst(cpu_context*ctx){
    goto_addr(ctx,ctx->cur_inst->param,true);
}
static void proc_inc(cpu_context*ctx){
    u16 val = cpu_read_reg(ctx->cur_inst->reg_1)+1;
    if(is_16_bit(ctx->cur_inst->reg_1))emu_cycles(1);
    if(RT_HL==ctx->cur_inst->reg_1
        &&AM_MR==ctx->cur_inst->mode){
        val = bus_read(cpu_read_reg(RT_HL))+1;
        val &=0xFF;
        bus_write(cpu_read_reg(RT_HL),val);
    }else{
        cpu_set_reg(ctx->cur_inst->reg_1,val);
        val = cpu_read_reg(ctx->cur_inst->reg_1);
    }
    if(0x03==(ctx->cur_opcode &0x03)){// 0b0011
        return;//see inst set , that inc cmd dont need to set flag register
    }
    cpu_set_flags(0==val,0,0==(val&0x0f),-1);
}
static void proc_dec(cpu_context*ctx){
    u16 val = cpu_read_reg(ctx->cur_inst->reg_1)-1;
    if(is_16_bit(ctx->cur_inst->reg_1)){
        emu_cycles(1);
    }
    if(RT_HL==ctx->cur_inst->reg_1
        &&AM_MR==ctx->cur_inst->mode){
        val = bus_read(cpu_read_reg(RT_HL))-1;
        bus_write(cpu_read_reg(RT_HL),val);
    }else{
        cpu_set_reg(ctx->cur_inst->reg_1,val);
        val = cpu_read_reg(ctx->cur_inst->reg_1);
    }
    if(0x0B==(ctx->cur_opcode &0x0B)){
        return;//see inst set , that dec cmd dont need to set flag register
    }
    cpu_set_flags(0==val,1,0x0f==(val&0xf),-1);
}

static void proc_adc(cpu_context*ctx){
    u16 u = ctx->fetched_data;
    u16 a = ctx->regs.a;
    u16 c = CPU_FLAG_C;
    ctx->regs.a = (a+u+c)&0xff;
    cpu_set_flags(0==ctx->regs.a,0
        ,(a&0xf)+(u&0xf)+c>0xf,a+u+c>0xff);
}
#define CPU_REG1 ctx->cur_inst->reg_1
#define CPU_RD_REG1 cpu_read_reg(CPU_REG1)
static void proc_add(cpu_context*ctx){
    u32 val = ctx->fetched_data +CPU_RD_REG1;
    bool is16bit = is_16_bit(CPU_REG1);
    if(is16bit)emu_cycles(1);
    if(RT_SP==CPU_REG1)val = (int8_t)ctx->fetched_data+CPU_RD_REG1;
    int z = 0==(val&0xff);
    int h = (0xf & ctx->fetched_data)+(0xf & CPU_RD_REG1)
                >=0x10;
    int c = (int)(0xff & ctx->fetched_data)+ (int)(0xff & CPU_RD_REG1)
                >=0x100;
    if(is16bit){
        z = -1;
        h = (0xfff & CPU_RD_REG1) +(0xfff & ctx->fetched_data) 
            >=0x1000;
        u32 n = (u32)CPU_RD_REG1+(u32)ctx->fetched_data;
        c = n>=0x10000;
    }
    if(RT_SP==CPU_REG1){
        z = 0;
        h = (0xf & ctx->fetched_data)+(0xf & CPU_RD_REG1)
                >=0x10;
        c = (int)(0xff & ctx->fetched_data)+(int)(0xff & CPU_RD_REG1)
                >=0x100;
    }
    cpu_set_reg(CPU_REG1,val&0xffff);
    cpu_set_flags(z,0,h,c);
}
static void proc_sub(cpu_context*ctx){
    u16 val = CPU_RD_REG1-ctx->fetched_data;
    int z = 0==val;
    int h = ((int)CPU_RD_REG1 & 0xF)-((int)ctx->fetched_data & 0xf)<0;
    int c = (int)CPU_RD_REG1- (int)ctx->fetched_data<0;
    cpu_set_reg(CPU_REG1,val);
    cpu_set_flags(z,1,h,c);
}
static void proc_sbc(cpu_context*ctx){
    u8 val = CPU_FLAG_C+ctx->fetched_data;
    int z = CPU_RD_REG1-val == 0;
    int h = (0xf & (int)CPU_RD_REG1)
            -(int)CPU_FLAG_C
            -(0xf & (int)ctx->fetched_data)<0;
    int c = (int)CPU_RD_REG1
            -(int)CPU_FLAG_C
            - (int)ctx->fetched_data<0;
    cpu_set_reg(CPU_REG1,CPU_RD_REG1-val);
    cpu_set_flags(z,1,h,c);
}
static void proc_jr(cpu_context*ctx){
    int8_t rel = (int8_t)(ctx->fetched_data&0xFF);
    u16 addr = ctx->regs.pc+rel;
    goto_addr(ctx,addr,false);
}
static void proc_ret(cpu_context*ctx){
    if(CT_NONE!=ctx->cur_inst->cond)emu_cycles(1);
    if(check_cond(ctx)){
        u16 lo = stack_pop();
        emu_cycles(1);
        u16 hi = stack_pop();
        emu_cycles(1);
        u16 n = (hi<<8)|lo;
        ctx->regs.pc = n;
        emu_cycles(1);
    }
}
static void proc_rlca(cpu_context*ctx){
    u8 u = ctx->regs.a;
    bool c = (u>>7)&1;
    u = (u<<1)|c;
    ctx->regs.a = u;
    cpu_set_flags(0,0,0,c);
}
static void proc_rrca(cpu_context*ctx){
    u8 b = ctx->regs.a &1;
    ctx->regs.a >>=1;
    ctx->regs.a |= (b<<7);
    cpu_set_flags(0,0,0,b);
}
static void proc_rla(cpu_context*ctx){
    u8 u = ctx->regs.a;
    u8 cf = CPU_FLAG_C;
    u8 c = (u>>7)&1;
    ctx->regs.a = (u<<1)|cf;
    cpu_set_flags(0,0,0,c);
}
static void proc_rra(cpu_context*ctx){
    u8 carry = CPU_FLAG_C;
    u8 new_c = ctx->regs.a&1;
    ctx->regs.a >>=1;
    ctx->regs.a |= (carry<<7);
    cpu_set_flags(0,0,0,new_c);
}

static void proc_reti(cpu_context*ctx){
    ctx->int_master_enabled = true;
    proc_ret(ctx);
}
static void proc_stop(cpu_context*ctx){
    fprintf(stderr,"STOPPING\n");
    NO_IMPL
}
static void proc_daa(cpu_context*ctx){
    u8 u = 0;
    int fc = 0;
    if(CPU_FLAG_H||(!CPU_FLAG_N&&(ctx->regs.a&0xf)>9)){
        u = 6;
    }
    if(CPU_FLAG_C||(!CPU_FLAG_N && (ctx->regs.a>0x99))){
        u|=0x60;
        fc = 1;
    }
    ctx->regs.a+= CPU_FLAG_N? -u:u;
    cpu_set_flags(ctx->regs.a==0,-1,0,fc);
}
static void proc_cpl(cpu_context*ctx){
    ctx->regs.a = ~ctx->regs.a;
    cpu_set_flags(-1,1,1,-1);
}
static void proc_scf(cpu_context*ctx){
    cpu_set_flags(-1,0,0,1);
}
static void proc_ccf(cpu_context*ctx){
    cpu_set_flags(-1,0,0,CPU_FLAG_C^1);
}
static void proc_halt(cpu_context*ctx){
    ctx->halted = true;
}
//Prefix CB pattern
//https://gbdev.io/pandocs/CPU_Instruction_Set.html#rotate-and-shift-instructions
static void proc_cb(cpu_context*ctx){
    u8 op = ctx->fetched_data;
    reg_type reg = decode_reg(op&0b111);
    u8 bit = (op>>3)&0b111;
    u8 bit_op = (op>>6)&0b11;
    u8 reg_val = cpu_read_reg8(reg);
    emu_cycles(1);
    if(RT_HL == reg)emu_cycles(2);
    typedef enum{
        BIT = 1
        ,RST
        ,SET = 3
    }cb_op_type;
    switch(bit_op){
        case BIT:cpu_set_flags(!(reg_val&(1<<bit)),0,1,-1);return;
        case RST:reg_val &=~(1<<bit);cpu_set_reg8(reg,reg_val);return;
        case SET:reg_val |= (1<<bit);cpu_set_reg8(reg,reg_val);return;
    } 
    typedef enum{
        RLC = 0 //rotate left carry
        ,RRC //rotate right carry
        ,RL //rotate left
        ,RR //rotate right
        ,SLA //shift left arithmetic
        ,SRA //shift right arithmetic
        ,SWAP//exchange low/hi-nibble
        ,SRL = 7//shift right logical
    }cb_bit_type;
    bool flagC = CPU_FLAG_C;
    switch(bit){
        case RLC:{
            bool setC = false;
            u8 result = (reg_val <<1)&0xff;
            if(0!=((1<<7)&reg_val)){
                result |=1;
                setC = true;
            }
            cpu_set_reg8(reg,result);
            cpu_set_flags(0==result,false,false,setC);
        }return;
        case RRC:{
            u8 old = reg_val;
            reg_val >>=1;
            reg_val |= (old<<7);
            cpu_set_reg8(reg,reg_val);
            cpu_set_flags(!reg_val,false,false,old&1);
        }return;
        case RL:{
            u8 old = reg_val;
            reg_val <<=1;
            reg_val |= flagC;
            cpu_set_reg8(reg,reg_val);
            cpu_set_flags(!reg_val,false,false,!!(old&0x80));
        }return;
        case RR:{
            u8 old = reg_val;
            reg_val >>=1;
            reg_val |= (flagC<<7);
            cpu_set_reg8(reg,reg_val);
            cpu_set_flags(!reg_val,false,false,old&1);
        }return;
        case SLA:{
            u8 old = reg_val;
            reg_val <<=1;
            cpu_set_reg8(reg,reg_val);
            cpu_set_flags(!reg_val,false,false,!!(old&0x80));
        }return;
        case SRA:{
            u8 u = (int8_t)reg_val>>1;
            cpu_set_reg8(reg,u);
            cpu_set_flags(!u,0,0,reg_val&1);
        }return;
        case SWAP:{
            reg_val = ((reg_val&0xF0)>>4)|((reg_val&0xf)<<4);
            cpu_set_reg8(reg,reg_val);
            cpu_set_flags(0==reg_val,false,false,false);
        }return;
        case SRL:{
            u8 u = reg_val >>1;
            cpu_set_reg8(reg,u);
            cpu_set_flags(!u,0,0,reg_val&1);
        }return;
    }
    fprintf(stderr,"ERROR: INVALID CB:%02X\n",op);
    NO_IMPL
}
//processes CPU instructions...
static IN_PROC processors[] = {
   [IN_NONE]= proc_none
   ,[IN_NOP]= proc_nop
   ,[IN_LD]= proc_ld
   ,[IN_JP]= proc_jp
   ,[IN_DI]= proc_di
   ,[IN_EI]= proc_ei
   ,[IN_LDH] = proc_ldh
   ,[IN_POP] = proc_pop
   ,[IN_PUSH] = proc_push
   ,[IN_CALL] = proc_call
   ,[IN_JR] = proc_jr
   ,[IN_RET] = proc_ret
   ,[IN_RETI] = proc_reti
   ,[IN_RST] = proc_rst
   ,[IN_INC] = proc_inc  
   ,[IN_DEC] = proc_dec
   ,[IN_ADD] = proc_add
   ,[IN_ADC] = proc_adc
   ,[IN_SUB] = proc_sub
   ,[IN_SBC] = proc_sbc
   ,[IN_CP] = proc_cmp
   ,[IN_AND] = proc_and 
   ,[IN_OR] = proc_or
   ,[IN_XOR] = proc_xor  
   ,[IN_CB] = proc_cb
   ,[IN_RLCA] = proc_rlca
   ,[IN_RRCA] = proc_rrca
   ,[IN_RRA] = proc_rra
   ,[IN_RLA] = proc_rla
   ,[IN_STOP] = proc_stop
   ,[IN_DAA] = proc_daa
   ,[IN_CPL] = proc_cpl
   ,[IN_SCF] = proc_scf
   ,[IN_CCF] = proc_ccf
   ,[IN_HALT] = proc_halt
   
};

IN_PROC inst_get_processor(in_type type){
    return processors[type];
}