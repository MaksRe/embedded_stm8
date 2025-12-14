/**
 *
 *
 *
 *
 */

#include "config.h"

#include "stm8s.h"
#include "stm8s_tim6.h"

#include "can_driver.h"
#include "can_matrix.h"
#include "temperature.h"
#include "fuel.h"
#include "string.h"
#include "math.h"
#include "dtc.h"
#include "uds.h"
#include "memory.h"
#include "events.h"

#ifdef DEBUG
    #define TIMEOUT 150 /**< Таймаут обнаружения отсутствия активности на линии CAN x50 мс */
    #define BAM_TIMEOUT 100 /**< Таймаут ожидания приема транспортного сообщения после приема заголовка x50 мс */
#else
    #define TIMEOUT 30 /**< Таймаут обнаружения отсутствия активности на линии CAN x50 мс */
    #define BAM_TIMEOUT 20 /**< Таймаут ожидания приема транспортного сообщения после приема заголовка x50 мс */
#endif

/* Перечень ID сообщений в соответствии со стандартом J1939-71, J1939-73 */
static j1939_header_t headers_tx[] = {
    {0x18DAF901},   // UDS TX

    // Транспортный протокол DM1
    {0x1CECFF01},   // DM1 BAM
    {0x1CEBFF01},   // DM1 PACK
};
static j1939_header_t header_uds_rx = {
    0x18DA0102
};

#define NUMBER_OF_TX_MESSAGES (sizeof(headers_tx)/sizeof(headers_tx[0]))

static struct {
    uint8_t counter;
    uint16_t total_number_of_packets;
    bool bam_sent;
    
    union {
        struct {
            uint8_t SPN_987            : 2; // protect lamp
            uint8_t SPN_624            : 2; // amber warning lamp
            uint8_t SPN_623            : 2; // red stop lamp
            uint8_t SPN_1213           : 2; // malfunction indicator lamp
            uint8_t SPN_3038;               // Falsh MIL
            
            // 3 сообщения j1939_message_t для размещения DTC
            uint8_t other[(sizeof(j1939_message_t) * 3) - 2];
        } parts;
        uint8_t all[sizeof(j1939_message_t) * 3];
    } data;

} tp_DM1 = { 0, 0, FALSE, {0} };


/**********************************************
 * Инициализация параметров CAN модуля
 */
void can_init(void)
{
    if (can_driver_init())
    {
        source_address_e source_address = CAN_SA_DEFAULT;
        uint8_t i = 0;
    
        if (eeprom_map.can_source_address != 0)
            source_address = eeprom_map.can_source_address;
    
        // Установка каждому TX-сообщению сохраненный в EEPROM Source Address
        for (; i < NUMBER_OF_TX_MESSAGES; i++)
            headers_tx[i].parts.src = source_address;
    
        // Так же меняем SA назначения для ответного сообщения от тестера по UDS протоколу
        header_uds_rx.from_tester.dest = source_address;
        
        can_config_receive_message(header_uds_rx.id);
    }
}

/**********************************************
 *
 */
static bool change_source_address(tx_message_e message, uint8_t new_source_address)
{
    return FALSE;
}

/**********************************************
 *
 */
void handler_rx_irq(j1939_message_t *message)
{
    if ((message->header.from_tester.dest == header_uds_rx.from_tester.dest) &&
        (message->header.from_tester.src  == header_uds_rx.from_tester.src))
    {
        message->header.id = headers_tx[UDS_ANSWER].id;

        uds_processing_rx(message);
    }
}

/*******************************************************************************
 *
 */
static void form_message_DM1BAM(j1939_message_t *message)
{
    DTC_group_t *group = dtc_get_all(DTC_ACTIVE);
    uint8_t number_of_faults = group->number_of_faults;
    uint16_t total_message_size = 2 + (number_of_faults * SIZEOF_DTC);
    uint8_t n = 0;

    // Проверка на необходимость выставить неисправность в пассивное состояние,
    // одноразово передав соответствующее DTC с нулевым набором данных
    for (; n < number_of_faults; n++)
    {
        DTC_e name = group->names[n];
        if (dtc_get_passive_flag(name) == TRUE)
        {
            dtc_set(name, DTC_PASSIVE);
            dtc_set_passive_flag(name, FALSE);
        }
    }

    /*
     * Формирование BAM-сообщения для отправки
     */
    message->header.id = headers_tx[DM1_BAM].id;
    message->data.CM.BAM.control_byte              = TP_CM_BAM;
    message->data.CM.BAM.total_message_size        = swap_uint16(total_message_size); // Длина мультипакета в байтах
    message->data.CM.BAM.total_number_of_packets   = (uint8_t)ceil(total_message_size, 8); // Количество пакетов по 8 байт
    message->data.CM.BAM.maximum_number_of_packets = 0xff; // нет ограничений для отправителя
    message->data.CM.BAM.PGN                       = swap_uint32(headers_tx[DM1].parts.pgn) >> 8;

    /*
     * Формируем первое сообщение в пакетном DM1, 
     * в котором устанавливаются состояния ламп
     */
    tp_DM1.data.parts.SPN_624   = 1;  // amber warning lamp
    tp_DM1.data.parts.SPN_987   =
    tp_DM1.data.parts.SPN_623   =
    tp_DM1.data.parts.SPN_1213  = 0x3;
    tp_DM1.data.parts.SPN_3038  = 0xff;

    // Установка всех полезных данных в 0xff 
    // для обеспечения заполненности единицами свободных байт (так требует стандарт j1939)
    memset(
        tp_DM1.data.parts.other,
        0xff,
        sizeof(tp_DM1.data.parts.other)
    );
    
    /*
     * Копируем все активные DTC в промежуточный буффер для 
     * последующей обработчки функцией control_transfer_dm1 
     */    
    memcpy(
        // первые 3 байта - lamps (см. TP_DM1_t), далее идут DTC
        tp_DM1.data.parts.other,
        group->DTCs, 
        sizeof(DTC_t) * number_of_faults
    );

    tp_DM1.counter = 0;
    tp_DM1.total_number_of_packets = message->data.CM.BAM.total_number_of_packets;
    tp_DM1.bam_sent = TRUE;
}

/*******************************************************************************
 *
 */
static void form_message_DM1PACK(j1939_message_t *message)
{
    message->header.id = headers_tx[DM1_PACK].id; // назначение заголовка
    
    memcpy(
        &message->data.bytes[1], // начиная с первого байта (0-ой занят под SN)
        &tp_DM1.data[tp_DM1.counter * 7], // из буфера неисправностей (0-6 (7 айт), 7-13 (7 байт), ...)
        7 // Копирование 7 байт из буфера (без учета SN)
    );
    message->data.bytes[tp_DM1.counter] = tp_DM1.counter++; // установка номера сообщения (SN)
}

/*******************************************************************************
 *
 */
void can_monitor_dm1(void)
{
    uint8_t number_of_active = dtc_get_number(DTC_ACTIVE);
    
    if (number_of_active > 0)
    {
        j1939_message_t message = {
            {headers_tx[DM1].id},
            // Неиспользованные данные зполняем единицами
            {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
            8
        };
        
        // Если DTC один, то отправляем его по PGN DM1SINGLE
        if (number_of_active == 1)
        {
            const DTC_obj_t *obj = dtc_get_first(DTC_ACTIVE);

            if (obj != NULL)
            {
                DM1SINGLE_t *dm1 = &message.data.DM1SINGLE;
                
                dm1->SPN_624 = 0x1; // amber warning lamp
    
                // Проверка неисправности - не установлена ли в пассивное состояние
                if (dtc_get_passive_flag(obj->name) == TRUE)
                {
                    dtc_set(obj->name, DTC_PASSIVE);
                    dtc_set_passive_flag(obj->name, FALSE);
                }
                
                SET_DM1SINGLE(obj->DTC);
            }
        }
        // Если DTC более одного, то формируем мультипакет
        else
        {
            if (!tp_DM1.bam_sent)
            {
                // Формируем DM1 BAM
                form_message_DM1BAM(&message);
                
                start_data_transfer(TRANSFER_DM1);
            }
            else
                form_message_DM1PACK(&message);
        }

        can_send(&message);        
    }
}

/*******************************************************************************
 * 
 */
void can_send_message(tx_message_e tx_message)
{
    j1939_message_t message = {
        {headers_tx[tx_message].id},
        // Неиспользованные данные зполняем единицами        
        {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
        8
    };

    if (tx_message >= NUMBER_OF_TX_MESSAGES)
        return;
   
    switch (tx_message)
    {
        // ...
            
        default: break;
    }
    
    // Отправка сформированного сообщения
    can_send(&message);
}

/**********************************************
 * Изменения Source Address для всех TX-сообщений и сохранение в EEPROM
 */
void can_change_source_address(source_address_e source_address)
{
    uint8_t i = 0;

    if (eeprom_map.can_source_address == source_address)
        return;
    
    disableInterrupts();
        
        write_to_eeprom(&eeprom_map.can_source_address, 
                        &source_address, 
                        sizeof(eeprom_map.can_source_address));

        // Установка каждому TX-сообщению сохраненный в EEPROM Source Address
        for (; i < NUMBER_OF_TX_MESSAGES; i++)
            headers_tx[i].parts.src = source_address;

        // Так же меняем SA назначения для ответного сообщения от тестера по UDS протоколу
        header_uds_rx.from_tester.dest = source_address;

    enableInterrupts();
}

/*******************************************************************************
 * 
 */
void control_transfer_dm1(void)
{
    if (tp_DM1.bam_sent)
    {
        j1939_message_t message = {
            headers_tx[DM1_PACK].id, 
            {0}, 
            8
        };

        memcpy(
            &message.data.bytes[1], // начиная с первого байта (0-ой занят под SN)
            &tp_DM1.data.all[tp_DM1.counter * 7], // из буфера неисправностей (0-6 (7 айт), 7-13 (7 байт), ...)
            7 // Копирование 7 байт из буфера, т.к. 0-ой байт - SN
        );
        message.data.bytes[0] = ++tp_DM1.counter; // установка номера сообщения (SN)

        can_send(&message);

        if (tp_DM1.counter >= tp_DM1.total_number_of_packets)
        {
            stop_data_transfer(TRANSFER_DM1);

            tp_DM1.bam_sent = FALSE;
        }
    }
}

