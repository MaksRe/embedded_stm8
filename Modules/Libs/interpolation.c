/*
 * interpolation.c
 *
 *  Created on: 23 июл. 2020 г.
 *      Author: RevkovMS
 *
 *  Данная реализация представлена по ссылке https://aterlux.ru/article/ntcresistor
 */

#include "interpolation.h"


void interpolation_init(interpolation_data_t* data)
{
	// Under construction...
}

// Функция вычисляет значение температуры в десятых долях градусов Цельсия
// в зависимости от суммарного значения АЦП.
int16_t interpolation_calc(interpolation_data_t* data, uint32_t adc)
{
	temperature_table_index_type l = 0;
	temperature_table_index_type r = (temperature_table_index_type)(data->table_size - 1);
	temperature_table_entry_type thigh = data->table[r];
    temperature_table_entry_type tlow = data->table[0];
    temperature_table_entry_type vl;
	temperature_table_entry_type vr, vd;
	int16_t res;
    
	// Проверка выхода за пределы и граничных значений
	if (adc <= thigh) return data->over;
	if (adc >= tlow)  return data->under;

	// Двоичный поиск по таблице
	while ((r - l) > 1)
	{
		temperature_table_index_type m = (temperature_table_index_type)((l + r) >> 1);
		temperature_table_entry_type mid = data->table[m];
        
		if (adc > mid) r = m;
		else l = m;
	}
	vl = data->table[l];
	if (adc >= vl)
		return (l * data->table_step + data->table_start);

	vr = data->table[r];
	vd = vl - vr;
	res = data->table_start + r * data->table_step;

	if (vd) // Линейная интерполяция
		res -= (int16_t)(((data->table_step * (int32_t)(adc - vr) + (vd >> 1)) / vd));

	return res;
}
