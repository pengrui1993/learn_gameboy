#include <cpu.h>
#include <bus.h>
#include <emu.h>
#include <instructions.h>
#include <interrupts.h>
#include <dbg.h>
#include <timer.h>
#define ORDER_LOG_ENABLE1
cpu_context ctx = {0};
static u16 pc = 0;
void cpu_init() {
    ctx.regs.pc = 0x100;
    ctx.regs.sp = 0xFFFE;
    *((short *)&ctx.regs.a) = 0xB001;
    *((short *)&ctx.regs.b) = 0x1300;
    *((short *)&ctx.regs.d) = 0xD800;
    *((short *)&ctx.regs.h) = 0x4D01;
    ctx.ie_register = 0;
    ctx.int_flags = 0;
    ctx.int_master_enabled = false;
    ctx.enabling_ime = false;
    timer_get_context()->div = 0xABCC;
}

static void fetch_instruction(){
    ctx.cur_opcode = bus_read(ctx.regs.pc++);
    ctx.cur_inst = instruction_by_opcode(ctx.cur_opcode);
    emu_cycles(1);
    if(NULL==ctx.cur_inst){
        printf("Unknown Instruction! %02X\n",ctx.cur_opcode);
        exit(-7);
    }
}
static void execute(){
    IN_PROC proc = inst_get_processor(ctx.cur_inst->type);
    if(NULL==proc){
        printf("no matched instruction handler,opcode:%02X ,name:%s\n"
        ,ctx.cur_opcode,inst_name(ctx.cur_inst->type));
        NO_IMPL
    }
    proc(&ctx);
    //printf("\tNot executing yet...\n");
}
static void message(){
#if defined ORDER_LOG_ENABLE
    char flags[16] = {0};
    sprintf(flags,"%c%c%c%c"
        ,ctx.regs.f&(1<<7)?'Z':'-'
        ,ctx.regs.f&(1<<6)?'N':'-'
        ,ctx.regs.f&(1<<5)?'H':'-'
        ,ctx.regs.f&(1<<4)?'C':'-'
    );
    char inst[16];
    inst_to_str(&ctx,inst);
    printf("%08llX - %04X: %-12s (%02X %02X %02X) A: %02X F:%s BC: %02X%02X DE: %02X%02X HL: %02X%02X\n"
        ,emu_get_context()->ticks,pc,inst,ctx.cur_opcode,bus_read(pc+1),bus_read(pc+2)
        ,ctx.regs.a,flags,ctx.regs.b,ctx.regs.c,ctx.regs.d,ctx.regs.e,ctx.regs.h,ctx.regs.l);
    //printf("Executing Instruction: %02X PC:%04X\n",ctx.cur_opcode,pc);
#endif
}
extern void fetch_data();
#define DEBUG_CONTER(n) static int counter=0;if(counter++>n)return false
#define DBG_TICK dbg_update();dbg_print()
#define SHOW_MESSAGE message()
bool cpu_step() {
    //DEBUG_CONTER(5);
    if(!ctx.halted){
        pc = ctx.regs.pc;
        fetch_instruction();//read once bus , used 1 machine cycle
        fetch_data();//use n machine cycles by instruction
        //DBG_TICK;
        SHOW_MESSAGE;
        execute();
    }else{
        emu_cycles(1);//call interrupt logic
        if(ctx.int_flags){
            ctx.halted = false;
        }
    }
    if(ctx.int_master_enabled){
        cpu_handle_interrputs();
        ctx.enabling_ime = false;
    }
    if(ctx.enabling_ime)ctx.int_master_enabled = true;
    return true;
}
