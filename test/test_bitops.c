#include "../include/BitOps.h"
#include <stdio.h>

int main(void){
    uint32_t value = 1;
    BIT_SET(value,3);
    printf("%d",value);
    return 0;
}