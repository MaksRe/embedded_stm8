/**
 * \file can_driver.h
 * \brief 
 *
 *
 */

#include "string.h"

#include "stm8s_gpio.h"
#include "stm8s_clk.h"
#include "stm8s_can.h"

#include "can_driver.h"
#include "can.h"
#include "can_matrix.h"
#include "queue.h"
#include "memory.h"

#define PORT    GPIOG
#define PIN_TX  GPIO_PIN_0
#define PIN_RX  GPIO_PIN_1

#define CAN_TX_ITEM_SIZE    (sizeof(j1939_message_t))
#define CAN_TX_BUFFER_SIZE  (8)

static cyclic_queue_t tx_queue;

typedef struct {
    CAN_ErrorCode_TypeDef err;
    uint16_t tec;
    uint16_t rec;
} can_err_s;
static can_err_s can_err;

/**********************************************
 *
 */
void can_config_receive_message(uint32_t rxid)
{
    const uint8_t RTR = 0;
    const uint8_t IDE = 1;

    // В соответствии со спецификацией (figure 148)
    // Устанавливаем между битами [9] и [10] PGN'а два бита - RTR и IDE
    // По спецификации 0-ой бит должен быть = 0
    
    uint32_t id  =  (((rxid & 0xFFFC0000) << 2) | ((uint32_t)RTR << 19) | ((uint32_t)IDE << 18) |
                      (rxid & 0x0003FFFF)) << 1;
    
    CAN_FilterInit(
        CAN_FilterNumber_0, // фильт для единственного сообщения на приём
        ENABLE,
        CAN_FilterMode_IdList,
        CAN_FilterScale_32Bit,
        
        (uint8_t)(id >> 24),    // uint8_t CAN_FilterID1,
        (uint8_t)(id >> 16),    // uint8_t CAN_FilterID2,
        (uint8_t)(id >> 8),     // uint8_t CAN_FilterID3,
        (uint8_t)(id),          // uint8_t CAN_FilterID4,
        0,0,0,0
    );
}

/* Входная частота на CAN модуль 24МГц */
static CAN_InitStatus_TypeDef set_bit_rate(can_baud_rate_e can_baud_rate)
{
    const struct {
        uint8_t prescaler;
        CAN_BitSeg1_TypeDef seg1;
        CAN_BitSeg2_TypeDef seg2;
    } baud_rate_parameters[] = {
        {12, CAN_BitSeg1_13TimeQuantum, CAN_BitSeg2_2TimeQuantum}, // 125 kbps
        {6, CAN_BitSeg1_13TimeQuantum, CAN_BitSeg2_2TimeQuantum}, // 250 kbps
        {3, CAN_BitSeg1_13TimeQuantum, CAN_BitSeg2_2TimeQuantum}, // 500 kbps
        {2, CAN_BitSeg1_10TimeQuantum, CAN_BitSeg2_1TimeQuantum} // 1000 kbps
    };
    
    if (can_baud_rate >= NUMBER_OF_TRANSMISSION_RATES)
        return CAN_InitStatus_Failed;
    
    /* QUANTA = (1 + BitSeg1 + BitSeg2)
     * 
     * http://www.bittiming.can-wiki.info/
     */
    return CAN_Init(
        CAN_MasterCtrl_AllDisabled,
        CAN_Mode_Normal,
        
        CAN_SynJumpWidth_1TimeQuantum,
        baud_rate_parameters[can_baud_rate].seg1,
        baud_rate_parameters[can_baud_rate].seg2,
        baud_rate_parameters[can_baud_rate].prescaler
    );
}

/**********************************************
 *
 */
bool can_driver_init(void)
{
    can_baud_rate_e baud_rate = CAN_BAUD_RATE_250_KBPS; // по умолчанию 250 kbps
    CAN_InitStatus_TypeDef status = CAN_InitStatus_Failed;
    
    // Peripheral Clock Enable 2, CAN
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_CAN, ENABLE); 
    
    GPIO_Init(PORT, PIN_TX, GPIO_MODE_OUT_PP_HIGH_FAST);
    GPIO_Init(PORT, PIN_RX, GPIO_MODE_IN_PU_NO_IT);
    
    /* CAN register init */
    CAN_DeInit();
    
    if (eeprom_map.can_baud_rate == CAN_BAUD_RATE_125_KBPS ||
        // eeprom_map.can_baud_rate == CAN_BAUD_RATE_250_KBPS || // по умолчанию
        eeprom_map.can_baud_rate == CAN_BAUD_RATE_500_KBPS ||
        eeprom_map.can_baud_rate == CAN_BAUD_RATE_1000_KBPS)
        baud_rate = eeprom_map.can_baud_rate;
    
    status = set_bit_rate(baud_rate);
    // ABOM bit activate
    // The Bus-Off state is exited automatically by hardware once 128 x 11 recessive bits have been monitored
    CAN->MCR |= 0x40; 

    // Отключаем режим совместимости с ST7 для активации TxMailBox3
    CAN_ST7CompatibilityCmd(CAN_ST7Compatibility_Disable);
    
    if (status == CAN_InitStatus_Success)
    {
        cyclic_queue_init(&tx_queue, CAN_TX_BUFFER_SIZE, CAN_TX_ITEM_SIZE);
        
        // FIFO  message pending interrupt
        CAN_ITConfig(CAN_IT_FMP, ENABLE); /*!< FIFO  message pending interrupt */
        CAN_ITConfig(CAN_IT_FOV, ENABLE); /*!< FIFO  overrun  interrupt */
        CAN_ITConfig(CAN_IT_ERR, ENABLE); /*!< Genaral Error interrupt */
        CAN_ITConfig(CAN_IT_EWG, ENABLE); /*!< Error warning interrupt */
        CAN_ITConfig(CAN_IT_EPV, ENABLE); /*!< Error passive  interrupt */
        CAN_ITConfig(CAN_IT_BOF, ENABLE); /*!< Bus-off interrupt */
        CAN_ITConfig(CAN_IT_LEC, ENABLE); /*!< Last error code interrupt */
        CAN_ITConfig(CAN_IT_TME, ENABLE); /*!< Transmit mailbox empty interrupt */
        
        return TRUE;
    }
    
    return FALSE;
}


/**********************************************
 *
 */
bool can_send(j1939_message_t *message)
{
    CAN_TxStatus_TypeDef tx_status;

    if (message == NULL || 
        message->data_length < 1 || 
        message->data_length > 8) 
        return FALSE;
    
    tx_status = CAN_Transmit(
        message->header.id,
        CAN_Id_Extended,
        CAN_RTR_Data,
        message->data_length,
        message->data.bytes);

    if (tx_status == CAN_TxStatus_Ok         ||
        tx_status == CAN_TxStatus_MailBox0Ok ||
        tx_status == CAN_TxStatus_MailBox1Ok ||
        tx_status == CAN_TxStatus_MailBox2Ok)
    {
        // Проверить другие состояния
        // ...
        
        // printf("CAN: Успешная передача сообщения.\r\n");
        
        // uart_send(tx_status);
        
        return TRUE;
    } 
    else
    {
        // CAN_CancelTransmit(mail_box);
        //printf("CAN: передача сообщения не удалась и отменена.\r\n");

        if (tx_status == CAN_TxStatus_NoMailBox)
        {
            if (!cyclic_queue_is_full(&tx_queue))
            {
                // disableInterrupts();
                cyclic_queue_push(&tx_queue, (void *)message);
                // enableInterrupts();
                
                return TRUE;
            }
        }
        
        
        return FALSE;
    }
}

static bool check_errors(void)
{
    can_err.tec = CAN->Page.Config.TECR;
    can_err.rec = CAN->Page.Config.RECR;
    
    if (CAN_GetITStatus(CAN_IT_FOV) == SET) /*!< FIFO overrun interrupt */
    {
        CAN_ClearFlag(CAN_FLAG_FOV);
        CAN_ClearITPendingBit(CAN_IT_FOV);
    }

    if (CAN_GetITStatus(CAN_IT_ERR) == SET) /*!< Genaral Error interrupt */
    {
        CAN_ClearFlag(CAN_FLAG_FOV);
        CAN_ClearITPendingBit(CAN_IT_ERR);
    }
    if (CAN_GetITStatus(CAN_IT_EWG) == SET) /*!< Error warning interrupt */
    {
        CAN_ClearFlag(CAN_FLAG_EWG);
        CAN_ClearITPendingBit(CAN_IT_EWG);
    }
    if (CAN_GetITStatus(CAN_IT_EPV) == SET) /*!< Error passive interrupt */
    {
        CAN_ClearFlag(CAN_FLAG_EPV);
        CAN_ClearITPendingBit(CAN_IT_EPV);
    }
    if (CAN_GetITStatus(CAN_IT_BOF) == SET) /*!< Bus-off interrupt */
    {
        CAN_ClearFlag(CAN_FLAG_BOF);
        CAN_ClearITPendingBit(CAN_IT_BOF);
    }
    if (CAN_GetITStatus(CAN_IT_LEC) == SET) /*!< Last error code interrupt */
    {
        can_err.err = CAN_GetLastErrorCode();
        
        CAN_ClearFlag(CAN_FLAG_LEC);
        CAN_ClearITPendingBit(CAN_IT_LEC);
    }

    return TRUE;
}

/**********************************************
 *
 */
@svlreg INTERRUPT_HANDLER(CAN_RX_IRQHandler, ITC_IRQ_CAN_RX)
{
    check_errors();
    
    if (CAN_GetITStatus(CAN_IT_FMP) == SET)
    {
        j1939_message_t message;
        uint8_t i = 0;

        CAN_Receive();

        message.data_length = CAN_GetReceivedDLC();
        message.header.id = CAN_GetReceivedId();
        for (; i < message.data_length; i++)
            message.data.bytes[i] = CAN_GetReceivedData(i);
        
        handler_rx_irq(&message);
    }
}


/**********************************************
 *
 */
@svlreg INTERRUPT_HANDLER(CAN_TX_IRQHandler, ITC_IRQ_CAN_TX)
{
    check_errors();
    
    if (CAN_GetITStatus(CAN_IT_TME) == SET)
    {
        CAN_ClearITPendingBit(CAN_IT_TME);
        
        if (!cyclic_queue_is_empty(&tx_queue))
        {
            j1939_message_t message = *((j1939_message_t *)cyclic_queue_pop(&tx_queue));
            
            can_send(&message); 
        }
    }
}

/**********************************************
 *
 */
bool can_change_baud_rate(can_baud_rate_e can_baud_rate)
{
    if (can_baud_rate >= NUMBER_OF_TRANSMISSION_RATES)
        return FALSE;
    
    if (eeprom_map.can_baud_rate != can_baud_rate)
    {
        disableInterrupts();

        write_to_eeprom(&eeprom_map.can_baud_rate, 
                        &can_baud_rate, 
                        sizeof(eeprom_map.can_baud_rate));
        
        enableInterrupts();
    }
    
    return TRUE;
}


