#ifndef _AVG_H_
#define _AVG_H_

    #include "stdint.h"

	typedef struct {
		uint16_t arr[FILTER_AVERAGE_SIZE];
		uint8_t position; /* Внутренний указатель на элементы массива */
		uint8_t filled_on; /* На сколько заполнен массив arr актуальными значениями */
	} filter_average_t;

    // Расчет скользящего среднего\
	void filter_average_init(filter_average_t* filter);
	void add_to_the_moving_average(filter_average_t *filter, uint16_t new_val);
	uint16_t calc_moving_average(filter_average_t *filter);
    uint16_t calc_average(filter_average_t *filter);
    
#endif