#include "filter_middle.h"

void init_filter_middle(filter_middle_param_t* filter, uint16_t init_val)
{
	uint8_t i = 0;

	for (; i < FILTER_MIDDLE_SIZE; i++)
		filter->arr[i] = init_val;
		
	filter->position = 0;
}

uint16_t calc_middle_of_3(filter_middle_param_t* filter, uint16_t new_val)
{
	uint16_t a = 0;
    uint16_t b = 0; 
    uint16_t c = 0;
	
	filter->arr[filter->position] = new_val;
	filter->position = (uint8_t)((filter->position + 1) % FILTER_MIDDLE_SIZE);

    a = filter->arr[0];
    b = filter->arr[1];
    c = filter->arr[2];
    
	if ((a <= b) && (a <= c))
	  return (b <= c) ? b : c;
	else
	{
	  if ((b <= a) && (b <= c))
		  return (a <= c) ? a : c;
	  else
		  return (a <= b) ? a : b;
	}
}