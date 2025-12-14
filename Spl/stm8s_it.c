/**
 *
 *
 *
 */
 
#include "stm8s_it.h"

INTERRUPT_HANDLER_TRAP(TRAP_IRQHandler               ) {}
INTERRUPT_HANDLER(TLI_IRQHandler,                   0) {}
INTERRUPT_HANDLER(AWU_IRQHandler,                   1) {}
INTERRUPT_HANDLER(CLK_IRQHandler,                   2) {}
INTERRUPT_HANDLER(EXTI_PORTA_IRQHandler,            3) {}
INTERRUPT_HANDLER(EXTI_PORTB_IRQHandler,            4) {}
INTERRUPT_HANDLER(EXTI_PORTC_IRQHandler,            5) {}
INTERRUPT_HANDLER(EXTI_PORTD_IRQHandler,            6) {}
INTERRUPT_HANDLER(EXTI_PORTE_IRQHandler,            7) {}
INTERRUPT_HANDLER(SPI_IRQHandler,                   10) {}
// INTERRUPT_HANDLER(TIM1_UPD_OVF_TRG_BRK_IRQHandler,  11) {}
// INTERRUPT_HANDLER(TIM1_CAP_COM_IRQHandler,          12) {}

#ifdef _COSMIC_
    INTERRUPT_HANDLER(NonHandledInterrupt, 25) {}
#endif // _COSMIC_

#if defined (STM8S903) || defined (STM8AF622x)
    INTERRUPT_HANDLER(TIM5_UPD_OVF_BRK_TRG_IRQHandler, 13) {}
    INTERRUPT_HANDLER(TIM5_CAP_COM_IRQHandler, 14) {}
#else // (STM8S208) || (STM8S207) || (STM8S105) || (STM8S103) || (STM8AF62Ax) || (STM8AF52Ax) || (STM8AF626x)
    // INTERRUPT_HANDLER(TIM2_UPD_OVF_BRK_IRQHandler, 13) {}
    INTERRUPT_HANDLER(TIM2_CAP_COM_IRQHandler, 14) {}
#endif // (STM8S903) || (STM8AF622x)

#if defined (STM8S903) || defined (STM8AF622x) 
    INTERRUPT_HANDLER(EXTI_PORTF_IRQHandler, 8) {}
#endif // (STM8S903) || (STM8AF622x) 

#if defined (STM8S208) || defined (STM8AF52Ax)
    // INTERRUPT_HANDLER(CAN_RX_IRQHandler, 8) {}
    // INTERRUPT_HANDLER(CAN_TX_IRQHandler, 9) {}
#endif // (STM8S208) || (STM8AF52Ax)

#if defined (STM8S208) || defined(STM8S207) || defined(STM8S007) || defined(STM8S105) || \
    defined(STM8S005) ||  defined (STM8AF62Ax) || defined (STM8AF52Ax) || defined (STM8AF626x)
    INTERRUPT_HANDLER(TIM3_UPD_OVF_BRK_IRQHandler, 15) {}
    INTERRUPT_HANDLER(TIM3_CAP_COM_IRQHandler, 16) {}
#endif // (STM8S208) || (STM8S207) || (STM8S105) || (STM8AF62Ax) || (STM8AF52Ax) || (STM8AF626x)

#if defined (STM8S208) || defined(STM8S207) || defined(STM8S007) || defined(STM8S103) || \
    defined (STM8S003) || defined(STM8S001) || defined (STM8AF62Ax) || defined (STM8AF52Ax) || defined (STM8S903)
    // INTERRUPT_HANDLER(UART1_TX_IRQHandler, 17) {}
    // INTERRUPT_HANDLER(UART1_RX_IRQHandler, 18) {}
#endif // (STM8S208) || (STM8S207) || (STM8S103) || (STM8S001) || (STM8S903) || (STM8AF62Ax) || (STM8AF52Ax)

#if defined(STM8AF622x)
    INTERRUPT_HANDLER(UART4_TX_IRQHandler, 17) {}
    INTERRUPT_HANDLER(UART4_RX_IRQHandler, 18) {}
#endif // (STM8AF622x) 

INTERRUPT_HANDLER(I2C_IRQHandler, 19) {}

#if defined(STM8S105) || defined(STM8S005) ||  defined (STM8AF626x)
    INTERRUPT_HANDLER(UART2_TX_IRQHandler, 20) {}
    INTERRUPT_HANDLER(UART2_RX_IRQHandler, 21) {}
#endif // (STM8S105) || (STM8AF626x) 

#if defined(STM8S207) || defined(STM8S007) || defined(STM8S208) || defined (STM8AF52Ax) || defined (STM8AF62Ax)
    INTERRUPT_HANDLER(UART3_TX_IRQHandler, 20) {}
    INTERRUPT_HANDLER(UART3_RX_IRQHandler, 21) {}
#endif // (STM8S208) || (STM8S207) || (STM8AF52Ax) || (STM8AF62Ax)

#if defined(STM8S207) || defined(STM8S007) || defined(STM8S208) || defined (STM8AF52Ax) || defined (STM8AF62Ax)
    // INTERRUPT_HANDLER(ADC2_IRQHandler, 22) {}
#else // STM8S105 or STM8S103 or STM8S903 or STM8AF626x or STM8AF622x
    INTERRUPT_HANDLER(ADC1_IRQHandler, 22) {}
#endif // (STM8S208) || (STM8S207) || (STM8AF52Ax) || (STM8AF62Ax)

// STM8S208 or STM8S207 or STM8S105 or STM8S103 or STM8AF52Ax or STM8AF62Ax or STM8AF626x
// INTERRUPT_HANDLER(TIM4_UPD_OVF_IRQHandler, 23) {}
INTERRUPT_HANDLER(EEPROM_EEC_IRQHandler, 24) {}

