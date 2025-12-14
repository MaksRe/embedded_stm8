#ifndef _MIDDLE_ARIF_H_
#define _MIDDLE_ARIF_H_

    #include "stdint.h"

    #define MIDDLE_ARIFM_NUM_OF_ELEMENTS (8) 

    typedef struct {
        uint8_t index;
        uint8_t count;
        uint16_t buf[MIDDLE_ARIFM_NUM_OF_ELEMENTS];
        uint16_t avg;
    } middle_arif_param_t;

    void middle_arif_init(middle_arif_param_t *param);
    // бегущее среднее арифметическое
    void middle_arif_add(middle_arif_param_t *param, uint16_t val);
    uint16_t middle_arif_calc(middle_arif_param_t *param);

#endif