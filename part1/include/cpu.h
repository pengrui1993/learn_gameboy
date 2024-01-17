#pragma once

#include <common.h>
#include <instructions.h>
/* https://gbdev.io/pandocs/CPU_Registers_and_Flags.html#cpu-registers-and-flags
AF	A	-	Accumulator & Flags
BC	B	C	BC
DE	D	E	DE
HL	H	L	HL
SP	-	-	Stack Pointer
PC	-	-	Program Counter/Pointer
*/
typedef struct
{
    u8 a;
    u8 f;
    u8 b;
    u8 c;
    u8 d;
    u8 e;
    u8 h;
    u8 l;
    u16 pc;
    u16 sp;
} cpu_registers;
typedef struct
{
    cpu_registers regs;
    u16 fetched_data;
    u16 mem_dest;
    bool dest_is_mem;
    u8 cur_opcode;
    instruction* cur_inst;
    bool halted;
    bool stepping;
    bool int_master_enabled;
    bool enabling_ime;
    u8 ie_register;//interrupt enable
    u8 int_flags;
} cpu_context;
void cpu_init();
bool cpu_step();
u16 cpu_read_reg(reg_type rt);
void cpu_set_reg(reg_type rt,u16 val);
typedef void(*IN_PROC)(cpu_context*);
IN_PROC inst_get_processor(in_type type);
//https://gbdev.io/pandocs/CPU_Registers_and_Flags.html#the-flags-register-lower-8-bits-of-af-register
#define CPU_FLAG_Z BIT(ctx->regs.f,7)
#define CPU_FLAG_N BIT(ctx->regs.f,6)
#define CPU_FLAG_H BIT(ctx->regs.f,5)
#define CPU_FLAG_C BIT(ctx->regs.f,4)
//https://gbdev.io/pandocs/CPU_Registers_and_Flags.html#the-flags-register-lower-8-bits-of-af-register
void cpu_set_flags(char z,char n,char h,char c);
void cpu_set_ie_register(u8);
u8 cpu_get_ie_register();
cpu_registers* cpu_get_regs();

u8 cpu_read_reg8(reg_type rt);
void cpu_set_reg8(reg_type rt,u8 val);
reg_type decode_reg(u8 reg);
u8 cpu_get_int_flags();
void cpu_set_int_flags(u8 value);
void inst_to_str(cpu_context *ctx, char *str);