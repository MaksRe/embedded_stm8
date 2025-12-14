/**
 *
 *
 *
 *
 *
 */

#include "dtc.h"
#include "string.h"
#include "memory.h"
#include "math.h"

#define OC_UNAVAILABLE 0x7f // Все единицы если счетчик повторений недоступен

#define SPN_EXAMPLE_1   (uint16_t)12 
#define SPN_EXAMPLE_2   (uint16_t)1234

typedef enum {
    DTA_ABOVE_NORM_OPER_RANGE   = 0,    // DATA VALID BUT ABOVE NORMAL OPERATIONAL RANGE
    DTA_BELOW_NORM_OPER_RANGE   = 1,    // DATA VALID BUT BELOW NORMAL OPERATIONAL RANGE
    DTA_ERRATIC                 = 2,    // DATA ERRATIC, INTERMITTENT OR INCORRECT
    SHORTED_TO_HIGH_SRC         = 3,    // VOLTAGE ABOVE NORMAL, OR SHORTED TO HIGH SOURCE
    SHORTED_TO_LOW_SRC          = 4,    // VOLTAGE BELOW NORMAL, OR SHORTED TO LOW SOURCE
    OPEN_CIRCUIT                = 5,    // CURRENT BELOW NORMAL OR OPEN CIRCUIT
    GROUNDED_CIRCUIT            = 6,    // CURRENT ABOVE NORMAL OR GROUNDED CIRCUIT
    OUT_OF_CALIBRATION          = 13    // OUT OF CALIBRATION
} FMI_e;

static DTC_t DTCs_const[DTC_NUMBER_OF_DTC] = {
//   CM   OC              FMI                     SPN
    {0x0, OC_UNAVAILABLE, OPEN_CIRCUIT,           SPN_EXAMPLE_1}, // Обрыв ДУТ
    {0x0, OC_UNAVAILABLE, SHORTED_TO_HIGH_SRC,    SPN_EXAMPLE_1}, // КЗ ДУТ
    {0x0, OC_UNAVAILABLE, OPEN_CIRCUIT,           SPN_EXAMPLE_2}, // Обрыв ДТ
    {0x0, OC_UNAVAILABLE, SHORTED_TO_HIGH_SRC,    SPN_EXAMPLE_2}, // КЗ ДТ
    {0x0, OC_UNAVAILABLE, OUT_OF_CALIBRATION,     SPN_EXAMPLE_1}, // Не калиброван УТ ДУТ
}; 

static DTC_t DTCs[DTC_NUMBER_OF_DTC] = {
//   CM   OC              FMI                     SPN
    {0x0, OC_UNAVAILABLE, OPEN_CIRCUIT,           SPN_EXAMPLE_1}, // Обрыв ДУТ
    {0x0, OC_UNAVAILABLE, SHORTED_TO_HIGH_SRC,    SPN_EXAMPLE_1}, // КЗ ДУТ
    {0x0, OC_UNAVAILABLE, OPEN_CIRCUIT,           SPN_EXAMPLE_2}, // Обрыв ДТ
    {0x0, OC_UNAVAILABLE, SHORTED_TO_HIGH_SRC,    SPN_EXAMPLE_2}, // КЗ ДТ
    {0x0, OC_UNAVAILABLE, OUT_OF_CALIBRATION,     SPN_EXAMPLE_1}, // Не калиброван УТ ДУТ
}; 

/*******************************************************************************
 * Инициализация массива объестов DTC
 */
void dtc_init(void)
{
    uint8_t i = 0;
    for (; i < DTC_NUMBER_OF_DTC; i++)
    {
        // Intel to Motorolla
        uint32_t spn = swap_uint32(DTCs_const[i].format.SPN) >> 13;
        
        DTCs_const[i].format.SPN = DTCs[i].format.SPN = spn;
    }
}

/*******************************************************************************
 * Установка неисправности в активное/пассивное состяние
 */
void dtc_set(DTC_e n_dtc, DTC_type_t type)
{
    if (n_dtc < DTC_NUMBER_OF_DTC)
    {
        // Сброс старого состояния
        uint16_t state = eeprom_map.faults_list.all & ~(0x3 << (n_dtc << 1));
        // Установка нового состояния
        state |= type << (n_dtc << 1);
        
        write_to_eeprom(&eeprom_map.faults_list.all, &state, sizeof(eeprom_map.faults_list));
        
        if (type == DTC_ACTIVE)
            // Выставляем значения DTC отличные от 0
            DTCs[n_dtc].all = DTCs_const[n_dtc].all;
    }
}

void dtc_set_passive_flag(DTC_e n_dtc, _Bool state)
{
    uint16_t mask = 0x1 << n_dtc;
    uint16_t new_state = eeprom_map.faults_flags.all;
    
    if (state)
    {
        new_state |= mask;
        
        // Сбрасываем DTC в 0
        DTCs[n_dtc].all = 0;
    }
    else
    {
        new_state &= ~mask;
    }

    write_to_eeprom(&eeprom_map.faults_flags.all, &new_state, sizeof(new_state));
}

_Bool dtc_get_passive_flag(DTC_e n_dtc)
{
    return (_Bool)(eeprom_map.faults_flags.all & ((uint16_t)1 << n_dtc));
}

/*******************************************************************************
 * Состояние ошибки
 */
DTC_type_t dtc_get_state(DTC_e n_dtc)
{
    if (n_dtc >= DTC_NUMBER_OF_DTC)
        return DTC_UNDEFINED;
        
    return (DTC_type_t)((eeprom_map.faults_list.all >> (n_dtc << 1)) & 0x3);
}

/*******************************************************************************
 * Получить количество активных/пассивных ошибок
 */
uint8_t dtc_get_number(DTC_type_t type)
{
    uint16_t list = eeprom_map.faults_list.all;
    uint8_t cnt = 0;
    
    for (; list != 0; list >>= 2)
        if (((uint8_t)list & 0x3) == type)
            cnt++;

    return cnt;
}

/*******************************************************************************
 * Сброс активного состояния неисправности
 */
void dtc_reset_all(void)
{
    uint16_t state = 0; // сброшенное состояние
    
    // Сбрасываем неисправность
    write_to_eeprom(&eeprom_map.faults_list.all, &state, sizeof(state));
    write_to_eeprom(&eeprom_map.faults_flags.all, &state, sizeof(state));
}

/*******************************************************************************
 * [0; NUMBER_OF_DTC-1]
 */
DTC_t* dtc_get(DTC_e n_dtc)
{
    if (n_dtc >= DTC_NUMBER_OF_DTC)
        return NULL;
        
    return &DTCs[n_dtc];
}

/*******************************************************************************
 * Активные DTC
 */
DTC_group_t* dtc_get_all(DTC_type_t type)
{
    static DTC_group_t group = {
        0, 
        {DTC_UNDEFINED, DTC_UNDEFINED, DTC_UNDEFINED, DTC_UNDEFINED, DTC_UNDEFINED},
        {0, 0, 0, 0, 0}
    };
    
    uint16_t list = eeprom_map.faults_list.all;
    uint8_t n = 0;

    group.number_of_faults = 0;
    for (; list != 0; ++n, list >>= 2)
    {
        if (((uint8_t)list & 0x3) == type)
        {
            group.names[group.number_of_faults] = (DTC_e)n;
            group.DTCs[group.number_of_faults++] = DTCs[n];
        }
    }

    return &group;
}

/*******************************************************************************
 * Первый встречный активный DTC 
 */
DTC_obj_t* dtc_get_first(DTC_type_t type)
{
    static DTC_obj_t obj = {DTC_UNDEFINED, NULL};
    uint16_t list = eeprom_map.faults_list.all;
    uint8_t n = 0;
    
    for (; list != 0; ++n, list >>= 2)
    {
        if (((uint8_t)list & 0x3) == type)
        {
            obj.name = (DTC_e)n;
            obj.DTC = &DTCs[n];
            
            return &obj;
        }
    }

    return NULL;
}
