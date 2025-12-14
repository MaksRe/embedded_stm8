/*
 *
 *
 *
 *
 */
 
#include "stm8s.h"


/*******************************************************************************
 * Деление с округлением в большую сторону
 */
uint16_t ceil(uint16_t a, uint16_t b)
{
    uint16_t div = a / b;
    uint8_t rem = (uint8_t)((a % b) > 0);
    return (uint16_t)div + rem;
}

/*******************************************************************************
 * Поменять местами байты в uint16
 */
uint16_t swap_uint16(uint16_t val)
{
    return ((val >> 8) | (val << 8));
}

/*******************************************************************************
 * Поменять местами байты в uint32
 */
uint32_t swap_uint32(uint32_t val)
{
    return  ((val >> 24) & 0x000000ff) | // move byte 3 to byte 0
            ((val <<  8) & 0x00ff0000) | // move byte 1 to byte 2
            ((val >>  8) & 0x0000ff00) | // move byte 2 to byte 1
            ((val << 24) & 0xff000000);  // byte 0 to byte 3
}
