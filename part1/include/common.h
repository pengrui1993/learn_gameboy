#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define BIT(a, n) ((a & (1 << n)) ? 1 : 0)

//#define BIT_SET(a, n, on) (on ? (a) |= (1 << n) : (a) &= ~(1 << n))
#define BIT_SET(a, n, on) {if(on)a|=(1<<n);else (a) &= ~(1 << n);}

#define BETWEEN(a, b, c) ((a >= b) && (a <= c))
#define MIN(a,b)(a>b?b:a)
#define MAX(a,b)(a>b?a:b)
void delay(u32 ms);
u32 get_ticks();
#define NO_IMPL                                 \
    {                                           \
        fprintf(stderr, "NOT YET IMPLENTED\nFILE:%s\nLINE:%d\n",__FILE__,__LINE__); \
        exit(-5);                               \
    }