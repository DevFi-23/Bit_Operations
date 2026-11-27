#ifndef BITOPS_H
#define BITOPS_H
#include <stdint.h>

//1. Bit
#define BIT_SET(reg,pos)    \
    do{\
        if ((pos) < 32u) { \
            (reg) |= (UINT32_C(1) << ((pos) & 31u)); \
        } \
    } while(0)
#define BIT_CLR(reg,pos)    \
    do{\
        if ((pos) < 32u) { \
            (reg) &= ~(UINT32_C(1) << ((pos) & 31u)); \
        } \
    } while(0)
#define BIT_TGL(reg,pos)    \
    do{\
        if ((pos) < 32u) { \
            (reg) ^= (UINT32_C(1) << ((pos) & 31u)); \
        } \
    } while(0)
#define BIT_READ(reg,pos)   \
    ((pos) < 32u ? \
    ((((uint32_t)(reg)) >> ((pos) & 31u)) & UINT32_C(1)) : \
    (UINT32_C(0)))  //Read bit

//2. Bit Field
#define MASK(w)  ((w) == 0u ? UINT32_C(0) : \
                  ((w) >= 32u ? UINT32_C(0xFFFFFFFF) : \
                   (UINT32_C(0xFFFFFFFF) >> (32u - (w))))) //create a mask w, 0 <= w <= 32

#define FIELD_GET(reg,pos,w)    \
    ((pos) < 32u && (w) <= 32u && (w) <= (32u - (pos)) ? \
    (((uint32_t)(reg) >> ((pos) & 31u)) & MASK(w)) :\
    (UINT32_C(0)))
#define FIELD_SET(reg,pos,w,val)    \
    do{\
        if ((pos) < 32u && (w) <= 32u && (w) <= (32u - (pos))) { \
            (reg) = (((uint32_t)(reg) & ~(MASK(w) << ((pos) & 31u))) | \
                     (((uint32_t)(val) & MASK(w)) << ((pos) & 31u))); \
        } \
    } while(0)

#endif