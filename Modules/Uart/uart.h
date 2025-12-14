/**
 *
 *
 *
 *
 */

#ifndef _UART_H_
#define _UART_H_

    void uart_init(void);

    void uart_send_block(const void *data, uint8_t length);
    void uart_send_word(uint16_t word);
    void uart_send(const uint8_t *data, uint8_t data_length);
    
    void uart_send_as_str(uint8_t data);
    void uart_send_ascii(uint32_t data);
    
#endif