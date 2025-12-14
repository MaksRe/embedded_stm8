/**
 *
 *
 *
 *
 */

#include "modifications.h"
#include "stm8s.h"

#ifdef RS485
    
    #include "stm8s_clk.h"
    #include "stm8s_itc.h"
    #include "stm8s_uart1.h"
    
    #include "string.h"
    #include "queue.h"
    
    #define ENABLE_TX   (1)
    #define ENABLE_RX   (0)
    
    #define PORT        GPIOD
    #define PIN_RX      GPIO_PIN_6
    #define PIN_TX      GPIO_PIN_5
    
    #define BAUD_RATE   ((uint32_t)115200)
    
    #define UART_QUEUE_NUM_OF_ITEMS (16)
    #define UART_QUEUE_ITEM_SIZE    (1)
    
    static cyclic_queue_t uart_queue;
    
    void uart_init(void)
    {
        cyclic_queue_init(&uart_queue, UART_QUEUE_NUM_OF_ITEMS, UART_QUEUE_ITEM_SIZE);
    
        // Peripheral Clock Enable 1, UART1
        CLK_PeripheralClockConfig(CLK_PERIPHERAL_UART1, ENABLE); 
        // Init GPIO (AN Temperature)
        // GPIO_Init(PORT, PIN_TEMP, GPIO_MODE_IN_FL_NO_IT);
        
        //
        // Инициализация UART 1 
        //
        // UART1_DeInit();
        
        // 115200 8N1
        UART1_Init(
            BAUD_RATE, 
            UART1_WORDLENGTH_8D, 
            UART1_STOPBITS_1, 
            UART1_PARITY_NO,
            UART1_SYNCMODE_CLOCK_DISABLE, 
            UART1_MODE_TXRX_ENABLE);  
    
        #if (ENABLE_TX == 1)
            UART1_ITConfig(UART1_IT_TXE, ENABLE);
        #endif
        #if (ENABLE_RX == 1)
            UART1_ITConfig(UART1_IT_RXNE, ENABLE);
        #endif
        
        UART1->CR1 &= (uint8_t)(~UART1_CR1_UARTD);
    }
    
    
    void uart_send(const uint8_t *data, uint8_t data_length)
    {
        uint8_t i = 0;
        
        if (data_length == 0)
            return;
            
        for (; i < data_length; i++)
        {
            uint8_t val = *(data + i);
            cyclic_queue_push(&uart_queue, (void *)&val);
        }
    
        UART1_ITConfig(UART1_IT_TXE, ENABLE);
    }
    
    void uart_send_ascii(uint32_t data)
    {
        uint8_t index = 0;
        uint8_t buf[6];
        
        while (data > 0 || index < 1)
        {
            buf[index++] = (uint8_t)(data % 10 + 0x30);
            data /= 10;
        }
        
        while (index > 0)
            cyclic_queue_push(&uart_queue, (void *)&buf[--index]);
        
        UART1_ITConfig(UART1_IT_TXE, ENABLE);
    }

#endif

@svlreg INTERRUPT_HANDLER(UART1_TX_IRQHandler, ITC_IRQ_UART1_TX)
{
    #ifdef RS485
        #if (ENABLE_TX == 1)
            if (UART1_GetITStatus(UART1_IT_TXE) == SET)
            {
                if (!cyclic_queue_is_empty(&uart_queue))
                {
                    uint8_t val = *((uint8_t *)cyclic_queue_pop(&uart_queue));
                    
                    UART1_SendData8(val);
                } else
                {
                    UART1_ITConfig(UART1_IT_TXE, DISABLE);
                }
            }
        #endif
    #endif
}

INTERRUPT_HANDLER(UART1_RX_IRQHandler, ITC_IRQ_UART1_RX)
{
    #ifdef RS485
        #if (ENABLE_RX == 1)
            UART1_ClearITPendingBit(UART1_IT_RXNE);
        #endif
    #endif
}
