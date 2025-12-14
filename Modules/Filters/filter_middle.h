#ifndef _MIDDLE_H_
#define _MIDDLE_H_

    #include "stdint.h"

	typedef struct {
		uint16_t arr[FILTER_MIDDLE_SIZE];
		uint8_t position; /* Внутренний указатель на элементы массива */
	} filter_middle_param_t;

    // Медианный фильтр
	void init_filter_middle(filter_middle_param_t* filter, uint16_t init_val);
	uint16_t calc_middle_of_3(filter_middle_param_t* filter, uint16_t new_val);

#endif