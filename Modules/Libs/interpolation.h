/*
 * interpolation.h
 *
 *  Created on: 23 июл. 2020 г.
 *      Author: RevkovMS
 */

#ifndef INTERPOLATION_H_
#define INTERPOLATION_H_

	#include "stm8s.h"

    // Тип каждого элемента в таблице, если сумма выходит в пределах 16 бит - uint16_t, иначе - uint32_t
    typedef uint16_t temperature_table_entry_type;
    // Тип индекса таблицы. Если в таблице больше 256 элементов, то uint16_t, иначе - uint8_t
    typedef uint8_t temperature_table_index_type;

	typedef struct {
		const temperature_table_entry_type *table;	// Указатель на таблицу с ключевыми значениями
		uint16_t table_size;	// Размер таблицы
		int16_t under;			// Значение температуры, возвращаемое если сумма результатов АЦП больше первого значения таблицы
		int16_t over;			// Значение температуры, возвращаемое если сумма результатов АЦП меньше последнего значения таблицы
		int16_t table_start;	// Значение температуры соответствующее первому значению таблицы
		int8_t table_step;		// Шаг таблицы
	} interpolation_data_t;

	/* Инициализация структуры interpolation_data_t для конкретной характеристики */
	void interpolation_init(interpolation_data_t* data);

	/* Интерполяция на основе входных параметров
	 * data - структура ключевых данных
	 * adc  - новое значение ADC датчика
	 */
	int16_t interpolation_calc(interpolation_data_t* data, uint32_t adc);

#endif /* INTERPOLATION_H_ */
