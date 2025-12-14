/**
 * \file can_driver.h
 * \brief 
 *
 *
 */
 
 
#ifndef _CAN_DRIVER_H_
#define _CAN_DRIVER_H_

    #include "stm8s.h"
    #include "can.h"
    #include "can_matrix.h"

    typedef enum {
        CAN_BAUD_RATE_125_KBPS = 0,
        CAN_BAUD_RATE_250_KBPS,
        CAN_BAUD_RATE_500_KBPS,
        CAN_BAUD_RATE_1000_KBPS,
        NUMBER_OF_TRANSMISSION_RATES
    } can_baud_rate_e;

    bool can_driver_init(void);
    bool can_send(j1939_message_t *message);
    bool can_change_baud_rate(can_baud_rate_e can_baud_rate);
    void can_config_receive_message(uint32_t rxid);
    
#endif