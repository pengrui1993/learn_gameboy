#pragma once
#include <common.h>
/* https://gbdev.io/gb-opcodes/optables/
:0x100
NOP
JP $0150

:0x150
XOR A
LD C,$10
DEC B

    PC: 0100,A:01,F:Z-HC,BC:0013
00 - NOP
    PC: 0101,A:01,F:Z-HC,BC:0013
C3 50 01 - JP $0150
    PC: 0150,A:01,F:Z-HC,BC:0013
AF - XOR A
    PC: 0151,A:00,F:Z---,BC:0013
AE 10 - LD C,$10
    PC: 0153,A:00,F:Z---,BC:0010
05 - DEC B
    PC: 0154,A:00,F:-NH-,BC:FF10
*/
typedef enum
{// https://gbdev.io/pandocs/CPU_Instruction_Set.html
    IN_NONE
    ,IN_NOP
    ,IN_LD
    ,IN_INC
    ,IN_DEC
    ,IN_RLCA//rotate A left
    ,IN_ADD
    ,IN_RRCA//rotate A right 
    ,IN_STOP
    ,IN_RLA //rotate A left through carry
    ,IN_JR  //jump relative
    ,IN_RRA //rotate A right through carry
    ,IN_DAA //decimal adjust A
    ,IN_CPL //Flips all the bits in the 8-bit A register, and sets the N and H flags.
    ,IN_SCF //Sets the carry flag, and clears the N and H flags.
    ,IN_CCF //Flips the carry flag, and clears the N and H flags.
    ,IN_HALT
    ,IN_ADC //add with carry
    ,IN_SUB
    ,IN_SBC // sub with carry
    ,IN_AND
    ,IN_XOR
    ,IN_OR
    ,IN_CP //compare
    ,IN_POP
    ,IN_JP
    ,IN_PUSH
    ,IN_RET
    ,IN_CB //control bit?
    ,IN_CALL
    ,IN_RETI //return interrupt
    ,IN_LDH // load height 0xff00 - 0xffff
    ,IN_JPHL//jump to HL, PC=HL
    ,IN_DI //disable interrupts, IME=0
    ,IN_EI //enable interrupts, IME=1
    ,IN_RST//Unconditional function call to the absolute fixed address defined by the opcode.
    ,IN_ERR
    //CB instructions
    ,IN_RLC
    ,IN_RRC
    ,IN_RL
    ,IN_RR
    ,IN_SLA
    ,IN_SRA
    ,IN_SWAP
    ,IN_SRL
    ,IN_BIT
    ,IN_RES
    ,IN_SET
} in_type;

typedef enum
{
    AM_IMP //default 
    ,AM_R_D16
    ,AM_R_R
    ,AM_MR_R//memory location register
    ,AM_R
    ,AM_R_D8
    ,AM_R_MR // register , memory register
    ,AM_R_HLI //hl increment move to register
    ,AM_R_HLD //hl decrement
    ,AM_HLI_R
    ,AM_HLD_R
    ,AM_R_A8  //fetch addres 8bit data to register   
    ,AM_A8_R
    ,AM_HL_SPR
    ,AM_D16
    ,AM_D8
    ,AM_D16_R
    ,AM_MR_D8
    ,AM_MR  //mem register?
    ,AM_A16_R
    ,AM_R_A16//address's data loaded to register
} addr_mode;

typedef enum
{
    RT_NONE
    ,RT_A
    ,RT_F
    ,RT_B
    ,RT_C
    ,RT_D
    ,RT_E
    ,RT_H
    ,RT_L
    ,RT_AF
    ,RT_BC
    ,RT_DE
    ,RT_HL
    ,RT_SP
    ,RT_PC
} reg_type;

typedef enum
{
    CT_NONE,CT_NZ,CT_Z,CT_NC,CT_C
} cond_type;
typedef struct
{
    in_type type;
    addr_mode mode;
    reg_type reg_1;
    reg_type reg_2;
    cond_type cond;
    u8 param;
} instruction;

instruction* instruction_by_opcode(u8 opcode);
const char *inst_name(in_type t);

