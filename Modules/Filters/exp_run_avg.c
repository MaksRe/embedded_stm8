#include "exp_run_avg.h"

void init_exp_run_avg(exp_run_avg_param_t *param, uint16_t init_val, float k)
{
    param->fil_val = (float)init_val;
    param->k = k;
}

uint16_t calc_exp_run_avg(exp_run_avg_param_t *param, uint16_t val) 
{
    param->fil_val += ((float)val - param->fil_val) * param->k;
    
    return (uint16_t)(param->fil_val);
}
