#include "linear_equation.h"

/*******************************************************************************
 * Расчет линейного уравнения по двум точкам.
 */
int32_t calc_linear_equation(linear_equation_t *linear_equation, int32_t val)
{
    int32_t x1 = linear_equation->x1;
    int32_t x2 = linear_equation->x2;
    int32_t y1 = linear_equation->y1;
    int32_t y2 = linear_equation->y2;

    return (int32_t)((-val * (x2 - x1) - (x1 * y2 - x2 * y1)) / (y1 - y2));
}