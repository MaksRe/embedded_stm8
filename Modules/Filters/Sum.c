#include "Sum.h"

uint32_t calc_sum(uint16_t* arr, uint16_t length)
{
	uint32_t sum = 0;
    int16_t i = length - 1;
    
    if (length == 1)
		return arr[0];
    for (; i >= 0 ; i--)
		sum += arr[i];

	return sum;
}