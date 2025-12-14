#ifndef _EXP_H_
#define _EXP_H_

    #include "stdint.h"

    typedef struct {
        float fil_val;
        float k; // коэффициент фильтрации, 0.0-1.0
    } exp_run_avg_param_t;

    void init_exp_run_avg(exp_run_avg_param_t *param, uint16_t init_val, float k);
    uint16_t calc_exp_run_avg(exp_run_avg_param_t *param, uint16_t val);

#endif