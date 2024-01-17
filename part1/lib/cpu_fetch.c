#include<cpu.h>
#include<bus.h>
#include<emu.h>
extern cpu_context ctx;
void fetch_data(){
    ctx.mem_dest = 0;
    ctx.dest_is_mem = false;
    //if (ctx.cur_inst == NULL) return;
    switch(ctx.cur_inst->mode){
        case AM_IMP: return;
        case AM_R:
            ctx.fetched_data = cpu_read_reg(ctx.cur_inst->reg_1);
            return;
        case AM_R_R:
            ctx.fetched_data = cpu_read_reg(ctx.cur_inst->reg_2);
            return;
        case AM_R_D8://emu_cycles(1) one cpu cycle
            ctx.fetched_data = bus_read(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;
            return;
        case AM_R_D16:
        case AM_D16:{
            u16 lo = bus_read(ctx.regs.pc);
            emu_cycles(1);
            u16 hi = bus_read(ctx.regs.pc+1);
            emu_cycles(1);
            ctx.fetched_data = lo|(hi<<8);
            ctx.regs.pc+=2;
            return;
        }
        case AM_MR_R:{
            ctx.fetched_data = cpu_read_reg(ctx.cur_inst->reg_2);
            ctx.mem_dest = cpu_read_reg(ctx.cur_inst->reg_1);
            ctx.dest_is_mem = true;
            /* gbctr.pdf page 14
            LDH A, (C)
            Load to the 8-bit A register, data from the address specified by the 8-bit C register. The full 16-bit absolute
            address is obtained by setting the most significant byte to 0xFF and the least significant byte to the value of C,
            so the possible range is 0xFF00-0xFFFF.
            */
            if(RT_C==ctx.cur_inst->reg_1){
                ctx.mem_dest |= 0xff00;//LDH A,(C)
            }
            return;
        }
        case AM_R_MR:{//LD A,(BC)
            u16 addr = cpu_read_reg(ctx.cur_inst->reg_2);
            if(RT_C==ctx.cur_inst->reg_2){
                addr |= 0xff00;//LDH A,(C)
            }
            ctx.fetched_data = bus_read(addr);
            emu_cycles(1);
            return;
        }
        case AM_R_HLI:{
            ctx.fetched_data = 
                bus_read(cpu_read_reg(ctx.cur_inst->reg_2));
            emu_cycles(1);
            cpu_set_reg(RT_HL,cpu_read_reg(RT_HL)+1);
            return;
        }
        case AM_R_HLD:{
            ctx.fetched_data = 
                bus_read(cpu_read_reg(ctx.cur_inst->reg_2));
            emu_cycles(1);
            cpu_set_reg(RT_HL,cpu_read_reg(RT_HL)-1);
            return;
        }
        case AM_HLI_R:{
            ctx.fetched_data = cpu_read_reg(ctx.cur_inst->reg_2);
            ctx.mem_dest = cpu_read_reg(ctx.cur_inst->reg_1);
            ctx.dest_is_mem = true;
            cpu_set_reg(RT_HL,cpu_read_reg(RT_HL)+1);
            return;
        }
        case AM_HLD_R:{
            ctx.fetched_data = cpu_read_reg(ctx.cur_inst->reg_2);
            ctx.mem_dest = cpu_read_reg(ctx.cur_inst->reg_1);
            ctx.dest_is_mem = true;
            cpu_set_reg(RT_HL,cpu_read_reg(RT_HL)-1);
            return;
        }
        case AM_R_A8:{
            ctx.fetched_data = bus_read(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;
            return;
        }
        case AM_A8_R:{
            ctx.mem_dest = bus_read(ctx.regs.pc) | 0xff00;
            ctx.dest_is_mem = true;
            emu_cycles(1);
            ctx.regs.pc++;
            return;
        }
        case AM_HL_SPR:{
            ctx.fetched_data = bus_read(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;
            return;
        }
        case AM_D8:{
            ctx.fetched_data = bus_read(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;    
            return;
        }
        case AM_A16_R:
        case AM_D16_R:{
            u16 lo = bus_read(ctx.regs.pc);
            emu_cycles(1);
            u16 hi = bus_read(ctx.regs.pc+1);
            emu_cycles(1);
            ctx.mem_dest = lo|(hi<<8);
            ctx.dest_is_mem = true;
            ctx.regs.pc+=2;
            ctx.fetched_data = cpu_read_reg(ctx.cur_inst->reg_2);
            return;
        }
        case AM_MR_D8:{
            ctx.fetched_data = bus_read(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;
            ctx.mem_dest = cpu_read_reg(ctx.cur_inst->reg_1);
            ctx.dest_is_mem = true;
            return;
        }
        case AM_MR:{
            ctx.mem_dest = cpu_read_reg(ctx.cur_inst->reg_1);
            ctx.dest_is_mem = true;
            ctx.fetched_data = bus_read(cpu_read_reg(ctx.cur_inst->reg_1));
            emu_cycles(1);
            return;
        }
        case AM_R_A16:{//compare AM_R_D16 AM_D16 to learn what is difference
            u16 lo = bus_read(ctx.regs.pc);
            emu_cycles(1);
            u16 hi = bus_read(ctx.regs.pc+1);
            emu_cycles(1);
            u16 addr = lo|(hi<<8);
            ctx.regs.pc+=2;
            ctx.fetched_data = bus_read(addr);
            emu_cycles(1);
            return;
        }
        default:
            printf("Unknown Addressing Mode!%d (%02X)\n"
            ,ctx.cur_inst->mode,ctx.cur_opcode);
            exit(-7);
    }
}