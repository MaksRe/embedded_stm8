#include "filter_average.h"

void filter_average_init(filter_average_t* filter)
{
	memset(filter->arr, 0, sizeof(filter->arr)/sizeof(filter->arr[0]));
	filter->position = filter->filled_on = 0;
}

void add_to_the_moving_average(filter_average_t *filter, uint16_t new_val)
{
	filter->arr[filter->position] = new_val;
	filter->position = (uint8_t)((filter->position + 1) % FILTER_AVERAGE_SIZE);
	if (filter->filled_on < FILTER_AVERAGE_SIZE)
		filter->filled_on++;
}

uint16_t calc_moving_average(filter_average_t *filter)
{
    uint32_t sum = calc_sum(filter->arr, filter->filled_on);
	return (uint16_t)(sum / filter->filled_on);
}

uint16_t calc_average(filter_average_t *filter)
{
    uint8_t filled_on = filter->filled_on;
	uint32_t sum = calc_sum(filter->arr, filled_on);
	return (uint16_t)(sum / filled_on);
}