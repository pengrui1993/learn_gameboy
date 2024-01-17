#include<stack.h>
#include<cpu.h>
#include<bus.h>
/* top|bottom   empty|full

    [top & empty] stack
    STACK
    SP=0XDFFF

    MEMORY:
    0XDFF7: 00
    0XDFF8: 00
    0XDFF9: 00
    0XDFFA: 00
    0XDFFB: 00
    0XDFFC: 00
    0XDFFD: 00
    0XDFFE: 00
    0XDFFF: 00  <- SP
----------------------
    PUSH 0x55
    SP-- = 0XDFFE
    MEMORY[0XFFE] = 0x55

    MEMORY:
    0XDFF7: 00
    0XDFF8: 00
    0XDFF9: 00
    0XDFFA: 00
    0XDFFB: 00
    0XDFFC: 00
    0XDFFD: 00
    0XDFFE: 55  <- SP
    0XDFFF: 00
----------------------
    PUSH 0x77
    SP-- = 0XDFFD
    MEMORY[0XDFFD] = 0x77

    MEMORY:
    0XDFF7: 00
    0XDFF8: 00
    0XDFF9: 00
    0XDFFA: 00
    0XDFFB: 00
    0XDFFC: 00
    0XDFFD: 77  <- SP
    0XDFFE: 55
    0XDFFF: 00
----------------------
    val = POP
    val = MEMORY[0XDFFD] = 0x77
    SP++ = 0XDFFE

    MEMORY:
    0XDFF7: 00
    0XDFF8: 00
    0XDFF9: 00
    0XDFFA: 00
    0XDFFB: 00
    0XDFFC: 00
    0XDFFD: 77
    0XDFFE: 55 <- SP
    0XDFFF: 00
----------------------
    PUSH 0x88 
    SP-- = 0XDFFD
    MEMORY[0XDFFD] = 0x88

    MEMORY:
    0XDFF7: 00
    0XDFF8: 00
    0XDFF9: 00
    0XDFFA: 00
    0XDFFB: 00
    0XDFFC: 00
    0XDFFD: 88 <- SP
    0XDFFE: 55
    0XDFFF: 00
----------------------
*/

void stack_push(u8 data){
    cpu_get_regs()->sp--;
    bus_write(cpu_get_regs()->sp,data);
}
void stack_push16(u16 data){
    stack_push((data>>8)&0xFF);
    stack_push(data&0xFF);
}

u8 stack_pop(){
    return bus_read(cpu_get_regs()->sp++);
}
u16 stack_pop16(){
    u16 lo = stack_pop();
    u16 hi = stack_pop();
    return (hi<<8)|lo;
}