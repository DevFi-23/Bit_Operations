#ifndef BITOPS_H
#define BITOPS_H
#include <stdint.h>
//1. Bit
#define BIT_SET(reg,pos)    \
    ((reg) |= (UINT32_C(1) << (pos)) ) //Set bit
#define BIT_CLR(reg,pos)    \
    ((reg) &= ~(UINT32_C(1) << (pos))) //Clear bit
#define BIT_TGL(reg,pos)    \
    ((reg) ^= (UINT32_C(1) << (pos)))  //Toggle bit
#define BIT_READ(reg,pos)   \
    ((((uint32_t)(reg)) >> (pos)) & UINT32_C(1))  //Read bit

//2. Bit Field
#define MASK(w)  (UINT32_C(0xFFFFFFFF) >> ((32u) - (w))) //create a mask w, 0 <= w <= 32

#define FIELD_GET(reg,pos,w)    \
    (((uint32_t)(reg) >> (pos)) & MASK(w))
#define FIELD_SET(reg,pos,w,val)    \
    ((((uint32_t)(reg)) & (~(MASK(w) << (pos)))) | ((((uint32_t)(val)) & MASK(w)) << (pos)))

#endif