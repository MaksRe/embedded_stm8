/**
 *
 *
 */

#ifndef _ADC_H_
#define _ADC_H_

    #include "stm8s.h"

    void adc_init(void);
    void adc_conversion(void);
    uint16_t adc_get_raw_value(void);
    
#endif