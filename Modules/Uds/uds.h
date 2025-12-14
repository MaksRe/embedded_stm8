/**
 *
 *
 *
 *
 */

#ifndef _UDS_H_
#define _UDS_H_

    #include "can_matrix.h"

    // Типы сессий (управляется сервисом SERVICE_SESSION_CONTROL (0x10) )
    typedef enum {
        SESSION_DEFAULT         = 0x01,
        SESSION_PROGRAMMING     = 0x02,
        SESSION_EXTENDED        = 0x03,
        SESSION_EOL_KAMAZ       = 0x40
    } session_type_e;

    // Список поддерживаемых сервисов (Мнемоника)
    // Service ID
    typedef enum {
        SERVICE_EXAMPLE                         = 0x01, // для примера !!!

        SERVICE_READ_DATA_BY_IDENTIFIER         = 0x22, // Read data by identifier
        SERVICE_READ_MEMORY_BY_ADDRESS          = 0x23, // Read memory by address
        SERVICE_WRITE_DATA_BY_IDENTIFIER		= 0x2E, // Write data by identifier
        SERVICE_SESSION_CONTROL                 = 0x10,
        SERVICE_ECU_RESET                       = 0x11,
        SERVICE_CONTROL_DTC_SETTING             = 0x85,
        SERVICE_COMMUNICATION_CONTROL           = 0x28,
        SERVICE_SECURITY_ACCESS                 = 0x27,
        SERVICE_READ_DTC_INFO                   = 0x19,
        SERVICE_CLEAR_DIAGNOSTIC_INFO           = 0x14,

        SERVICE_UNDEFINED                       = 0xff,
        SID_END                                 = 0x85  
    } service_id_e;

    // Список поддерживаемых параметров
    // Parameter ID
    typedef enum {
        PARAMETER_ENTER_PROGRAMMING_MODE        = 0x02,
        PARAMETER_ENTER_DEFAULT_SESSION         = 0x81,
        PARAMETER_DTC_OFF                       = 0x82,
        PARAMETER_DISABLE_RX_TX                 = 0x83
        
    } parameter_id_e;

    // Идентификаторы данных по запросу SID 0x22 (ReadDataByIdentifier)
    // Структура read_data_by_identifier_u для фрейма Single Frame
    typedef enum {
        
        DATA_ID_EXAMPLE                                 = 0x0001

    } data_identifier_e;
    
    #define SF_data_length (message->data.UDS.SF.data_length_h << 16 | message->data.UDS.SF.data_length_l)
    
    typedef enum {
        SINGLE          = 0, // Single Frame
        FIRST           = 1, // First Frame (Первый фрейм из серии фреймов)
        CONSECUTIVE     = 2, // Consecutive Frame (последующий фрейм)
        FLOW_CONTROL    = 3  // Flow control Frame (фрейм управления потоком)
    } type_frame_e;

    typedef enum {
        READY,
        WAIT,
        OVERFLOW
    } flow_status_e;
    
    typedef enum {
        REQUEST_SEED    = 0x01,
        SEND_KEY        = 0x02
    } security_access_type_e;

    typedef enum {
        POSITIVE_RESPONSE       = 0x00,

        GENERAL_REJECT          = 0x10,
        SERVICE_NOT_SUPPORTED   = 0x11,
        SUB_FUNC_NOT_SUPPORTED  = 0x12,
        
        // Неверная длина сообщения или неверный формат
        // Этот NRC должен быть отправлен, если длина сообщения или формат параметров неверен.
        INCORRECT_MSG_LENGTH_OR_INVALID_FORMAT = 0x13,
        
        // Условия не верны
        // Этот NRC должен быть возвращен, если критерии для запроса ClearDiagnosticInformation не выполнены.
        CONDITIONS_NOT_CORRECT  = 0x22,
        
        // необходимо пройти процедуру SecurityAccess
        SECURITY_ACCESS_DENIED  = 0x33,
        
        INVALID_KEY             = 0x35
    } negative_resp_code_e; // NRC 
    
    typedef enum {
        REPORT_DTC_BY_STATUS_MASK = 0x02
    } report_dtc_type_e;
    
    typedef enum {
        HARD_RESET          = 0x1,
        KEY_OFF_ON_RESET    = 0x2,
        SOFT_RESET          = 0x3
    } reset_type_e;
    
    typedef struct {
        session_type_e current; // текущая сессия
        _Bool access; // доступ к параметрам на данной сессии
    } session_t;

    void uds_init(void);
    void uds_processing_rx(j1939_message_t *message);
    void control_of_sending_consecutive_frames(uint8_t msec);
    
#endif