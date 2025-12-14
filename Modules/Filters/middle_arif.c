#include "middle_arif.h"

void middle_arif_init(middle_arif_param_t *param)
{
	param->count = 0;
    param->index = 0;
    param->avg = 0;
}

// бегущее среднее арифметическое
void middle_arif_add(middle_arif_param_t *param, uint16_t val) 
{
    if (param->index >= MIDDLE_ARIFM_NUM_OF_ELEMENTS) 
        param->index = 0;
    
    param->avg -= param->buf[param->index];
    param->avg += val;
    param->buf[param->index] = val;
    param->index++;

	if (param->count < MIDDLE_ARIFM_NUM_OF_ELEMENTS)
		param->count++;
}

uint16_t middle_arif_calc(middle_arif_param_t *param) 
{
	if (param->count == 0)
		return 0;

	if (param->count < MIDDLE_ARIFM_NUM_OF_ELEMENTS)
		return ((uint16_t)param->avg / param->count); 
	else
		return ((uint16_t)param->avg >> MIDDLE_ARIFM_POWER);
}