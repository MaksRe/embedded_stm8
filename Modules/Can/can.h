/**
 *
 *
 *
 *
 */

#ifndef _CAN_H_
#define _CAN_H_

    #include "stm8s.h"
    #include "can_matrix.h"
    
    typedef enum {
        UDS_ANSWER,
        
        // Транспортный протокол DM1
        DM1_BAM,
        DM1_PACK
    } tx_message_e;

    typedef enum {
        CAN_SA_MODIF1   = 0x01,
        CAN_SA_DEFAULT  = 0x02
    } source_address_e;

    void can_init(void);
    void handler_rx_irq(j1939_message_t *message);
    void can_send_message(tx_message_e tx_message);
    void can_monitor_dm1(void);
    void can_change_source_address(source_address_e source_address);
    void control_transfer_dm1(void);

#endif