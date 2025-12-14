/**
 *
 *
 *
 *
 *
 */
 
#ifndef _CAN_DTC_H_
#define _CAN_DTC_H_

    #include "stm8s.h"

    typedef enum {
        DTC_1, // Для примера ...
        DTC_2,

        DTC_NUMBER_OF_DTC,
        DTC_UNDEFINED
    } DTC_e;

    /** \union tDTC
     * \brief Структура DTC (Diagnostic Trouble Code) (32 bits)
     *
     * Поля расположены в данном порядке для корректного выравнивания структуры (SPN в старшей части 32-битного поля)
     */
    #pragma pack(push, 1)
        typedef union {
            struct {
                uint32_t CM:1; /**< SPN Conversion Method */
                uint32_t OC:7; /**< Occurrence Count (Счетчик переходов из активной ошибки в не активную. Пока недоступен -> 0x7f) */
                uint32_t FMI:5; /**< Failure Mode Identifier */
                uint32_t SPN:19; /**< Low Suspect Parameter Number */
            } format;
            uint8_t bytes[4];
            uint32_t all;
        } DTC_t;
    #pragma pack(pop)
    
    #define SIZEOF_DTC (sizeof(DTC_t))
    
    typedef struct {
        DTC_e name; // имя неисправности
        DTC_t *DTC; // указатель на неисправность
    } DTC_obj_t;
    
    typedef struct {
        uint8_t number_of_faults; // количество активных ошибок
        DTC_e names[5]; // имена неисправностей
        DTC_t DTCs[5]; // указатели на неисправности
    } DTC_group_t;

    typedef enum {
        DTC_RESET   = 0x0,
        DTC_ACTIVE  = 0x1,
        DTC_PASSIVE = 0x2
    } DTC_type_t;
    
    void dtc_init(void);
    void dtc_set(DTC_e n_dtc, DTC_type_t type);
    void dtc_set_passive_flag(DTC_e n_dtc, _Bool state);
    _Bool dtc_get_passive_flag(DTC_e n_dtc);
    DTC_type_t dtc_get_state(DTC_e n_dtc);
    uint8_t dtc_get_number(DTC_type_t type);
    void dtc_reset_all(void);
    DTC_t* dtc_get(DTC_e n_dtc);
    DTC_group_t* dtc_get_all(DTC_type_t type);
    DTC_obj_t* dtc_get_first(DTC_type_t type);
   
#endif


