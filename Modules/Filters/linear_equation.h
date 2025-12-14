#ifndef _LINEAR_H_
#define _LINEAR_H_

    #include "stdint.h"

    typedef struct {
        type_x x1, x2;
        type_y y1, y2;
    } linear_equation_t;

    // Расчет линейного уравнения 
    // нахождение X по точкам [x1;x2][y1;y2]
    // val - значение Y
    int32_t calc_linear_equation(linear_equation_t *linear_equation, int32_t val);

#endif