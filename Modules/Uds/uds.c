/**
 *
 *
 *
 *
 */

#include "stm8s.h"
//#include "stm8s_tim2.h"
#include "memory.h"
#include "modifications.h"
#include "string.h"

#include "events.h"
#include "dtc.h"
#include "fuel.h"
#include "can_driver.h"
#include "uds.h"
#include "reset.h"
#include "debug.h"

#define random_seed()           (uint16_t)(TIM2_GetCounter() * 0xABCD);

typedef struct {
    const data_identifier_e data_identifier;
    uint8_t *data;
    uint16_t data_length;
    const uint16_t max_data_length;
} data_block_by_id_t;

static data_block_by_id_t data_blocks[] = {
    // блоки данных (массивы)
    // ...
};
#define NUMBER_OF_DATA_IDENTIFIERS (sizeof(data_blocks)/sizeof(data_blocks[0]))

static void handle_any_service(UDS_t *UDS);
// Другие обработчики сервисов
// ...

typedef void (*ptr_service_handler)(UDS_t *UDS);
typedef struct {
    service_id_e sid;
    ptr_service_handler service_handler;
} service_handler_t;
service_handler_t service_handlers[] = {
    {SERVICE_EXAMPLE, &handle_any_service},
};
#define NUMBER_OF_SERVICE_HANDLERS (sizeof(service_handlers)/sizeof(service_handlers[0]))

// Блок данных с указанием остатка данных на передачу по протоколу UDS
// Формирование происходит с FirstFrame
static volatile struct {
    uint8_t *data;
    uint8_t data_length;
    
    uint8_t serial_number; // номер фрейма (начинается с 1)
    uint16_t cnt_tx_bytes; // текущее количество переданных байт
    uint8_t cnt_separation_time; // счетчик межинтервальных задержек
    _Bool ready_to_transfer; // признак разрешения на отпавку в фоне
} tx_data = { NULL, 0, 1, 0, 0, FALSE };

// Структура параметров для приёма данных от стороннего узла
// по протоколу UDS, начиная с First Frame
static volatile struct {
    uint8_t data[128]; // Буффер данных на приём 
    uint16_t data_length; // длина данных параметра в байтах

    uint8_t serial_number; // номер фрейма (начинается с 1)
    uint16_t cnt_rx_bytes; // текущее количество переданных байт
} rx_data = { {0}, 0, 1, 0 };

static volatile struct {
    uint8_t block_size;
    uint8_t separation_time;
} flow_control = {0, 0};

static uint16_t current_seed = 0; // промежуточное значение для расчета key 

/*******************************************************************************
 * Расчет KEY для получения доступа к закрытым параметрам 
 */
static uint16_t calc_key(uint16_t seed)
{
    return 0x1; // пример
}

/*******************************************************************************
 * Инициализация UDS параметров
 */
void uds_init(void)
{
    uint8_t i = 0;

    // Определение длины данных каждого параметра в байтах,
    // отбрасывая нулевые значения
    for (; i < NUMBER_OF_DATA_IDENTIFIERS; i++)
    {
        uint8_t cnt = 0;
        for (; cnt < data_blocks[i].data_length; cnt++)
            if (data_blocks[i].data[cnt] == '\0')
                break;
        data_blocks[i].data_length = cnt;
    }
}

/*******************************************************************************
 * Возвращает обработчик на сервис
 */
static ptr_service_handler get_service_handler(service_id_e sid)
{
    int8_t i = NUMBER_OF_SERVICE_HANDLERS - 1;
    for (; i >= 0; i--)
        if (service_handlers[i].sid == sid)
            return service_handlers[i].service_handler;
    return NULL;
}

/*******************************************************************************
 * Формирование фрейма с указанием ошибки
 */
static void form_error_frame(UDS_t *UDS, negative_resp_code_e code)
{
    // Формируем отрицательный ответ
    UDS->error_frame.service_id     = UDS->SF.service_id;
    UDS->error_frame.error_id       = 0x7f; // по умолчанию 0x7f
    
    // Подфункция не поддерживается.
    // Этот NRC указывает, что запрошенное действие не будет предпринято, 
    // потому что сервер не поддерживает специфичные для сервиса параметры сообщения запроса.
    UDS->error_frame.response_code  = code;

    UDS->error_frame.data_length    = 3;

    memset(&UDS->error_frame.data[0], 0xff, (uint8_t)sizeof(UDS->error_frame.data));
}

/*******************************************************************************
 * Формирование Single Frame (в случае если блок данных умещается в один фрейм)
 * Если передать NULL в data, то будет сформировано эхо с позитивным ответом (SID + 0x40)
 *
 * Копирование data_length байт данных из src в dst 
 */
static void form_answer_single_frame(UDS_t *UDS, 
                                     uint8_t *src, 
                                     uint8_t *dst, 
                                     uint16_t data_length)
{
    // Фрмируем простой Single Frame с соответствующими данными PID
    
    if (UDS == NULL)
        return;
    
    // 1 байт SID + data_length (количество байт данных)
    // DS->SF.frame_type   = (type_frame_e)SINGLE; // В запросе UDS уже указано SINGLE
    UDS->SF.data_length  = 1 + data_length; 
    UDS->SF.service_id   = (uint8_t)(UDS->SF.service_id + 0x40);
    
    if (src != NULL && dst != NULL)
        memcpy(dst, src, data_length);
}

/*******************************************************************************
 * Формирование First Frame (в случае если блок данных не умещается в один фрейм)
 *
 * формирование данного фрейма происходит в случае если по запросу Single Frame 
 * блок данных превышает свободную область ответа в простом Single Frame
 *
 * data - ответ на запрос
 */
static void form_rdbi_answer_first_frame(uint8_t *data, uint16_t data_length, UDS_t *UDS)
{
    answer_read_data_by_id *rdbi = &UDS->FF.data.rdbi;
    uint8_t size_param_data = sizeof(rdbi->param_data);
    
    const uint8_t sid = (uint8_t)(UDS->SF.service_id + 0x40);
    const uint16_t did = UDS->SF.data.rdbi.data_identifier;
    
    // Формируем First Frame
    rdbi->service_id      = sid;
    rdbi->data_identifier = did;

    // 1 байт SID + 2 байта DID
    data_length += 3; 

    UDS->FF.frame_type      = (type_frame_e)FIRST;
    UDS->FF.data_length_h   = (uint8_t)(data_length >> 8);
    UDS->FF.data_length_l   = (uint8_t)data_length;
        
    // Копируем первую часть данных
    memcpy(rdbi->param_data, data, size_param_data);
    
    // -------------------------------------------------------------------------
    // Настройка остатков данных для последующей отправки их в фоне (через таймер).
    // Обработка отправки через функцию control_of_sending_consecutive_frames()
    // -------------------------------------------------------------------------
    tx_data.data = data + size_param_data;
    // Сколько байт осталось передать
    tx_data.data_length = (uint8_t)(data_length - size_param_data - 3);
    
    // признак разрешения на отправку в фоне - tx_data.ready_to_transfer, выставляется в обработчике flow_control_processing(),
    // так как отправка последующих данных - Consecutive Frames осуществляется строго после приёма сообщения Flow Control Frame
}

/*******************************************************************************
 * Поиск данных по запрашиваемому ID (запрос 0x22, SERVICE_READ_DATA_BY_IDENTIFIER)
 */
static data_block_by_id_t* find_data_by_id(data_identifier_e did)
{
    int8_t i = NUMBER_OF_DATA_IDENTIFIERS - 1;
    for (; i >= 0; i--)
        if (data_blocks[i].data_identifier == did)
            return &data_blocks[i];
    return NULL;
}

/*******************************************************************************
 * Формирование ответа и обработка запроса на установку текущей сессии блока
 */
static void handle_service_session_control(UDS_t *UDS)
{
    req_session_control *sc = &UDS->SF.data.sc;
    session_type_e new_session = (session_type_e)sc->diagnostic_session_type;
    bool success = TRUE;
    
    if (new_session == SESSION_DEFAULT     ||
        new_session == SESSION_PROGRAMMING ||
        new_session == SESSION_EXTENDED    ||
        new_session == SESSION_EOL_KAMAZ)
    {
        ans_session_control *ans = &UDS->SF.data.sc_ans;
        
        // Проверяем на изменения границы ДУТ после расширенной сессии,
        // при которой, возможно, они были модифицированы
        // Переход от SESSION_EXTENDED к SESSION_DEFAULT
        if (eeprom_map.session_type == SESSION_EXTENDED &&
            new_session             == SESSION_DEFAULT)
        {
            fuel_update_borders_in_eeprom();
        }
        
        // При переключении на другую сессию запрещаем какой-либо доступ к параметрам.
        // Необходимо будет пройти процедуру SecurityAccess для получения разрешения.
        // Если доступ был ранее получен и сессия не изменяется, то доступ сохраняется.
        if (new_session != eeprom_map.session_type)
        {
            uint8_t access = (uint8_t)FALSE;
            
            write_to_eeprom(&eeprom_map.session_access, &access, sizeof(access));
            write_to_eeprom(&eeprom_map.session_type, &new_session, sizeof(new_session));
        }
        
        // Формирование позитивного ответа на запрос
        // SID          (1 байт) + 
        // session_type (1 байт) + 
        // p2_server    (2 байта) + 
        // p_2_server   (2 байта)
        UDS->SF.data_length  = 6; 
        UDS->SF.service_id   = (uint8_t)(UDS->SF.service_id + 0x40);
        
        ans->p2_server = 50; // 0x0032 - 50 миллисекунд
        ans->p_2_server = 500; // 0x01F4 - 500 миллисекунд 
        ans->bytes[0] = 0xff;
    }
    else
    {
        form_error_frame(UDS, SUB_FUNC_NOT_SUPPORTED);
    }
}
 
/*******************************************************************************
 * Формирование ответа и обработка запроса на чтение параметра по ID
 */
static void handle_service_read_data_by_id(UDS_t *UDS)
{
    req_read_data_by_id *rdbi = &UDS->SF.data.rdbi;
    data_identifier_e did = (data_identifier_e)rdbi->data_identifier;
    data_block_by_id_t *data_block = find_data_by_id(did);
    
    if (data_block != NULL)
    {
        const uint8_t block_size = sizeof(rdbi->bytes);
        uint16_t data_length = (uint16_t)(data_block->data_length);
        
        // Можем ли мы отправить одним фреймом?
        if (data_length <= block_size)
        {
            // SID (1 байт) + PID (2 байта) + data_length 
            UDS->SF.data_length  = 3 + data_length; 
            UDS->SF.service_id   = (uint8_t)(UDS->SF.service_id + 0x40);
            
            // Очищаем область данных
            memset(rdbi->bytes, 0xff, block_size);
            // Чтение параметров
            memcpy(
                rdbi->bytes,
                data_block->data,
                data_length);

            // form_answer_single_frame(data_block->data, data_length, UDS);
        }
        else // иначе формируем мультипакет, начиная с FF (First Frame)
        {
            form_rdbi_answer_first_frame(data_block->data, data_length, UDS);
        }
    }
    else
    {
        form_error_frame(UDS, SUB_FUNC_NOT_SUPPORTED);
    }
}

/*******************************************************************************
 * Формирование ответа и обработка запроса на получение или запись данных по ID.
 * Обработка запроса на запись параметра ао ID от Single Frame
 */
static void handle_service_write_data_by_id(UDS_t *UDS)
{
    req_write_data_by_id *wdbi = &UDS->SF.data.wdbi;
    data_identifier_e did = (data_identifier_e)wdbi->data_identifier;
    data_block_by_id_t *data_block = find_data_by_id(did);
    
    if (data_block != NULL)
    {
        // По умолчанию считаем, что параметр найден и нужно сформировать позитивный ответ
        negative_resp_code_e code = POSITIVE_RESPONSE;

        switch (did)
        {
            case DATA_ID_EXAMPLE:
            {
                // ...

                break;
            }

            // Иначе какой-то параметр из оставшихся
            default: 
            {
                // Количество байт полезных данных
                const uint8_t new_data_length = (uint8_t)(UDS->SF.data_length - 3); // 1 байт SID, 2 байта DID
                
                // Если длина новых данных превышаем максимально допустимую, то ошибка
                if (new_data_length > data_block->max_data_length)
                {
                    code = INCORRECT_MSG_LENGTH_OR_INVALID_FORMAT;
                } 
                else 
                {
                    // выставляем нуль-терминальный символ в конце строки
                    uint8_t data[sizeof(wdbi->bytes) + 1] = {'\0', '\0', '\0', '\0', '\0'}; 
                    
                    memcpy(data, wdbi->bytes, new_data_length);

                    // Копируем принятые данные из буфера в EEPROM
                    write_to_eeprom(
                        data_block->data, 
                        data, 
                        // Если максимальная длина совпадает с длиной новых данных, то нуль-терминальный символ не указываем
                        (uint8_t)((new_data_length == data_block->max_data_length) 
                            ? new_data_length 
                            : new_data_length + 1)
                    );

                    data_block->data_length = new_data_length;
                }
                
                break;
            }
        }
        
        // SID (1 байт) + PID (2 байта)
        UDS->SF.data_length  = 3;

        if (code == POSITIVE_RESPONSE)
        {
            UDS->SF.service_id = (uint8_t)(UDS->SF.service_id + 0x40);
            // Очищаем область данных
            memset(wdbi->bytes, 0xff, sizeof(wdbi->bytes));
        }
        else
        {
            // Формирование негативного ответа на запрос
            form_error_frame(UDS, code);
        }
    }
    else
    {
        form_error_frame(UDS, SUB_FUNC_NOT_SUPPORTED);
    }
}

static void handle_service_control_dtc_setting(UDS_t *UDS)
{
    // Формируем эхо (SID + 0x40)
    form_answer_single_frame(UDS, NULL, NULL, 0);
}

static void handle_service_communication_control(UDS_t *UDS)
{
    // Формируем эхо (SID + 0x40)
    form_answer_single_frame(UDS, NULL, NULL, 0);
}

/*******************************************************************************
 * Обработка сервиса SECURITY ACCESS
 */
static void handle_service_security_access(UDS_t *UDS)
{
    req_security_access *sa_req = &(UDS->SF.data.sa_req);
    ans_security_access *sa_ans = &(UDS->SF.data.sa_ans);
    
    switch (sa_req->security_access_type)
    {
        case REQUEST_SEED:
        {
            // SID                  = 1 байт
            // security_access_type = 1 байт 
            // SEED                 = 2 байта
            UDS->SF.data_length  = 4; 
            UDS->SF.service_id   = (uint8_t)(UDS->SF.service_id + 0x40);

            // Если процедура уже была пройдена и активен доступ,
            // то формируем нулевой SEED
            if (eeprom_map.session_access == TRUE)
                current_seed = 0;
            else // Расчет SEED
                current_seed = random_seed();

            sa_ans->seed = current_seed;
            
            break;
        }
        case SEND_KEY:
        {
            uint16_t key = sa_req->key;
            uint16_t true_key = calc_key(current_seed);
            
            if (true_key == key)
            {
                uint8_t access = (uint8_t)TRUE;
                
                // Положительный ответ
                UDS->SF.data_length  = 2; 
                UDS->SF.service_id   = (uint8_t)(UDS->SF.service_id + 0x40);
                
                write_to_eeprom(&eeprom_map.session_access, &access, sizeof(access));
            }
            else
            {
                // Неверный ключ
                form_error_frame(UDS, INVALID_KEY);
            }
            
            break;
        }
        default:
            // подфункция (security_access_type) не поддерживается
            form_error_frame(UDS, SUB_FUNC_NOT_SUPPORTED);
        break;
    }
}

/*******************************************************************************
 * Обработка запроса на предоставление информации об неисправностях системы (DTC)
 */
static void handle_service_read_dtc_info(UDS_t *UDS)
{
    // pass
}

/*******************************************************************************
 * Обработка сервиса очистки диагностической информации
 */
static void handle_service_claer_diagnostic_info(UDS_t *UDS)
{
    req_clear_diagnostic_info *cdi = &(UDS->SF.data.cdi);
    
    // Если сброс всех DTC (в соответствии с ISO 14229-1)
    if (cdi->group_of_dtc == 0xffffff)
    {
        dtc_reset_all();
        
        form_answer_single_frame(UDS, NULL, NULL, 0);
    }
    else
    {
        // Условия не верны
        // Этот NRC должен быть возвращен, если критерии для запроса ClearDiagnosticInformation не выполнены.
        form_error_frame(UDS, CONDITIONS_NOT_CORRECT);
    }
}

/*******************************************************************************
 * Обработка сервиса сброса
 */
static void handle_service_ecu_reset(UDS_t *UDS)
{
    req_ecu_reset *req = &(UDS->SF.data.er_req);
    ans_ecu_reset *ans = &(UDS->SF.data.er_ans);
    
    switch (req->reset_type)
    {
        // case HARD_RESET:
        // break;
        
        // case KEY_OFF_ON_RESET:
        // break;
        
        case SOFT_RESET:
            set_reset_status(SOFTWARE_RESET);
        break;  
        
        default:
            form_error_frame(UDS, SERVICE_NOT_SUPPORTED);
        return;
    }
    form_answer_single_frame(UDS, NULL, NULL, 0);
    
    // Этот параметр показывает минимальное время ожидания, 
    // в течение которого ЭБУ будет находиться в режиме отключения питания
    ans->power_down_time = 2; // 2 секунды
}

/*******************************************************************************
 *******************************************************************************
 *******************************************************************************
 *******************************************************************************
 *******************************************************************************
 *******************************************************************************
 *******************************************************************************
*/
/*******************************************************************************
 * Формирование ответа на запрос single frame
 * Определение, поддерживается ли сервис и вызов соответствующего обработчика 
 */
static void handle_single_frame(UDS_t *UDS)
{
    service_id_e service_id = (service_id_e)UDS->SF.service_id;
    ptr_service_handler handler = get_service_handler(service_id);
    if (handler != NULL)
        handler(UDS);
}

/*******************************************************************************
 * Формирование ответа на запрос first frame
 */
static void handle_first_frame(UDS_t *UDS)
{
    // Определяем какой SID
    service_id_e sid = UDS->FF.data.wdbi.service_id;
    
    if (sid == SERVICE_WRITE_DATA_BY_IDENTIFIER)
    {
        // Закрыт доступ (не пройдена процедура SecurityAccess)
        // Не выполнен переход на сессию Extended
        if (eeprom_map.session_type   != SESSION_EXTENDED ||
            eeprom_map.session_access != TRUE)
        {
            form_error_frame(UDS, SECURITY_ACCESS_DENIED);
        }
        else
        {
            data_identifier_e did = UDS->FF.data.wdbi.data_identifier;
            data_block_by_id_t *data_block = find_data_by_id(did);
            
            // Если данный DID поддерживается
            if (data_block != NULL)
            {
                uint8_t first_block_size = sizeof(UDS->FF.data.bytes);

                rx_data.data_length = UDS->FF.data_length_h << 8 | UDS->FF.data_length_l;
                rx_data.cnt_rx_bytes = first_block_size; 
                
                if (rx_data.data_length > data_block->max_data_length)
                {
                    // Параметр не поддерживается
                    form_error_frame(UDS, INCORRECT_MSG_LENGTH_OR_INVALID_FORMAT);
                }
                else
                {
                    memcpy(
                        rx_data.data,
                        UDS->FF.data.bytes,
                        first_block_size
                    );
                    
                    // Формирование Flow control Frame – FC – фрейм управления потоком
                    UDS->FC.flow_status = (flow_status_e)READY;
                    UDS->FC.frame_type = (type_frame_e)FLOW_CONTROL;
                    UDS->FC.block_size = 0; // если значение 0, то количество блоков не ограничено
                    UDS->FC.sep_time = 0x28; // приём пакета данных каждые ~40 msec
                    
                    memset(UDS->FC.data, 0xff, sizeof(UDS->FC.data));
                }
            }
            else
            {
                // Параметр не поддерживается
                form_error_frame(UDS, SUB_FUNC_NOT_SUPPORTED);
            }
        }
    } 
    else
    {
        // Сервис не поддерживается
        form_error_frame(UDS, SERVICE_NOT_SUPPORTED);
    }
}

/*******************************************************************************
 * Обработка получения последующих фреймов после First Frame
 * data_length - количество полезных байт данных в принятом сообщении
 */
static void handle_rx_consecutive_frame(j1939_message_t *message)
{
    // Получили ли ожидаемый номер фрейма
    // и корректное количество данных
    if (rx_data.serial_number == message->data.UDS.CF.serial_number &&
        message->data_length > 0)
    {
        // 7 байт банных в фрейме consecutive не считая SN.
        // 0-ой байт: type frame, serial number - 8 бит
        uint8_t rem = sizeof(message->data.UDS.CF.data); 
        if (rx_data.cnt_rx_bytes + rem > rx_data.data_length)
            rem = (uint8_t)(rx_data.data_length - rx_data.cnt_rx_bytes);

        memcpy(
            rx_data.data + rx_data.cnt_rx_bytes,
            message->data.UDS.CF.data,
            rem
        );
        
        rx_data.cnt_rx_bytes += rem;
        rx_data.serial_number++;
        
        // все данные приняты
        if (rx_data.cnt_rx_bytes >= rx_data.data_length)
        {
            // Определяем какой SID для вызова нужного обработчика данных
            service_id_e sid = (service_id_e)rx_data.data[0];

            switch (sid)
            {
                // Запись данных по ID
                case SERVICE_WRITE_DATA_BY_IDENTIFIER:
                {
                    // service_id_e sid = (service_id_e)rx_data.data[0];
                    data_identifier_e did = (data_identifier_e)rx_data.data[1] << 8 | rx_data.data[2];
                    // Количество служебных данных в байтах
                    uint8_t size_service_data = 3; // 1 байт SID, 2 байта DID
                    // Количество байт полезных данных
                    uint16_t new_data_length = (uint16_t)(rx_data.data_length - size_service_data); 
                    
                    data_block_by_id_t *data_block = find_data_by_id(did);

                    if (data_block != NULL)
                    {
                        // добавляем нуль-терминальный символ для определения конца строки
                        rx_data.data[rx_data.data_length] = '\0'; 
                        
                        // Копируем принятые данные из буфера в EEPROM
                        // смещение для исключения копирования служебных данных
                        write_to_eeprom(
                            data_block->data, 
                            rx_data.data + size_service_data, 
                            // учитываем нуль-терминальный символ
                            (uint8_t)((new_data_length == data_block->max_data_length) ? new_data_length : new_data_length + 1)
                        );
                        
                        data_block->data_length = new_data_length;
                    }
                    
                    break;
                }

                default: 
                break;
            }
            
            rx_data.data_length = rx_data.cnt_rx_bytes = 0;
            rx_data.serial_number = 1;
        }
    }
}

/*******************************************************************************
 * Обработка Flow Control
 */
static bool flow_control_processing(j1939_message_t *message)
{
    bool status = FALSE;
    UDS_t *UDS = &message->data.UDS;
    flow_status_e flow_status = UDS->FC.flow_status;
    
    if (flow_status == READY)
    {
        uint8_t block_size = UDS->FC.block_size;
        
        if (block_size > 0)
        {
            uint8_t data_block_size = sizeof(UDS->CF.data);
            uint8_t index = 0;

            UDS->CF.frame_type      = (type_frame_e)CONSECUTIVE;
            UDS->CF.serial_number   = 1;

            // Передача оставшегося блока данных
            while (tx_data.data_length > 0)
            {
                uint8_t bytes = (uint8_t)((data_block_size > tx_data.data_length) ? tx_data.data_length : data_block_size);
                
                memcpy(
                    UDS->CF.data, 
                    &tx_data.data[index],
                    bytes);
                // Заполняем нулями остаток пакета
                if (bytes < data_block_size)
                    memset(
                        &UDS->CF.data[bytes],
                        0,
                        data_block_size - bytes
                    );
                
                can_send(message, FALSE, 8);
                
                index += bytes;
                tx_data.data_length -= bytes; 
                UDS->CF.serial_number++;
            }
            
            status = TRUE;
        }
    }
    else
    {
        // Формируем ошибку
        // Данная реализация временна !!!
        
        form_error_frame(UDS, SUB_FUNC_NOT_SUPPORTED);
    }
    
    return status;
}

/*******************************************************************************
 * Обработка Flow Control
 */
static void flow_control_processing(UDS_t *UDS)
{
    flow_status_e flow_status = UDS->FC.flow_status;
    
    if (flow_status == READY)
    {
        flow_control.block_size = UDS->FC.block_size;
        flow_control.separation_time = UDS->FC.sep_time;
        
        // признак разрешения на отправку оставшихся данных в фоне
        tx_data.ready_to_transfer = TRUE;
        // Запустить передачу данных
        start_data_transfer(TRANSFER_UDS);
    }
}

/*******************************************************************************
 * Проверка на наличие фреймов для отправки после First Frame - Consecutive Frames
 *
 * msec - количество msec для расчета интервалов между фреймами
 *        на основе значения flow_control.separation_time, которое выставляется через сообщение Flow Control Frame
 */
void control_of_sending_consecutive_frames(uint8_t msec)
{
    if (tx_data.ready_to_transfer)
    {
        tx_data.cnt_separation_time += msec;
        if (tx_data.cnt_separation_time >= flow_control.separation_time)
        {
            // Также тут нужна проверка на разрешенное количество передаваемых фреймов flow_control.block_size
            // ...
            
            j1939_message_t message = {
                0x18000000, // Укажи свой ID !!! 
                {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
                8
            };
            UDS_t *UDS = &message.data.UDS;
            
            // фиксированный размер блока данных Consecutive Frame
            const uint8_t data_block_size = sizeof(UDS->CF.data); 
            // определение того, сколько сейчас передать байт данных
            const uint8_t bytes = (uint8_t)((data_block_size > (tx_data.data_length - tx_data.cnt_tx_bytes)) ? (tx_data.data_length - tx_data.cnt_tx_bytes) : data_block_size);
            
            UDS->CF.frame_type      = (type_frame_e)CONSECUTIVE;
            UDS->CF.serial_number   = tx_data.serial_number++;
            
            memcpy(
                UDS->CF.data, 
                &tx_data.data[tx_data.cnt_tx_bytes],
                bytes);
            
            // can_send(&message, FALSE, (uint8_t)(bytes + 1));
            can_send(&message);
            
            // увеличиваем количество переданных байт
            tx_data.cnt_tx_bytes += bytes; 
    
            // проверка на завершение передачи
            if (tx_data.cnt_tx_bytes == tx_data.data_length)
            {
                tx_data.serial_number = 1; // номер фрейма (начинается с 1)
                tx_data.cnt_tx_bytes = 0; // текущее количество переданных байт
                tx_data.ready_to_transfer = FALSE;
                
                // Останавливаем таймер, который управлял данной функцией
                stop_data_transfer(TRANSFER_UDS);
            }
            
            tx_data.cnt_separation_time = 0;
        }
    }
}

/*******************************************************************************
 * Обработка запроса UDS и формирование ответа
 */
void uds_processing_rx(j1939_message_t *message)
{
    UDS_t* UDS = &message->data.UDS;
    
    switch (UDS->data.frame_type)
    {
        case SINGLE:
            handle_single_frame(UDS);
        break;
        case FIRST:
            handle_first_frame(UDS);
        break;
        case CONSECUTIVE:
            handle_rx_consecutive_frame(message);
        return;
        case FLOW_CONTROL: 
            flow_control_processing(UDS);
        return;
        default: break;
    }

    message->data_length = 8;
    can_send(message);
}




