/**
 *
 *
 */

#include "stm8s_adc2.h"
#include "stm8s_clk.h"
#include "stm8s_gpio.h"
#include "stm8s_itc.h"
#include "adc.h"
#include "filters.h"

#define NUM_OF_IDLE_MEASUREMENT (1)

#define PORT                _ // GPIO_TypeDef
#define PIN                 _ // GPIO_Pin_TypeDef
#define CHANNEL             _ // ADC2_Channel_TypeDef

#define start_conversion()  (ADC2->CR1 |= ADC2_CR1_ADON)
#define stop_conversion()   (ADC2->CR1 &= (uint8_t)(~ADC2_CR1_ADON))

static middle_arif_param_t arif_filter;
static uint8_t cnt_idle_measurement = NUM_OF_IDLE_MEASUREMENT;

void adc_init(void)
{
    middle_arif_init(&arif_filter);
    
    // Peripheral Clock Enable 2, ADC
    CLK->PCKENR2 |= (uint8_t)((uint8_t)1 << ((uint8_t)CLK_PERIPHERAL_ADC & (uint8_t)0x0F));

    // Set Input mode
    PORT->DDR &= (uint8_t)(~(PIN));
   
    ADC2->CR2 &= (uint8_t)(~ADC2_CR2_ALIGN);        // Clear the align bit
    ADC2->CR2 |= (uint8_t)(ADC2_ALIGN_RIGHT);       // Configure the data alignment
    ADC2->CR1 &= (uint8_t)(~ADC2_CR1_CONT);         // Set the single conversion mode
    ADC2->CSR &= (uint8_t)(~ADC2_CSR_CH);           // Clear the ADC2 channels
    ADC2->CSR |= (uint8_t)(CHANNEL);                // Select the ADC2 channel

    ADC2->CR1 &= (uint8_t)(~ADC2_CR1_SPSEL);        // Clear the SPSEL bits
    ADC2->CR1 |= (uint8_t)(ADC2_PRESSEL_FCPU_D18);  // Select the prescaler division factor according to ADC2_PrescalerSelection values

    ADC2->CR2 &= (uint8_t)(~ADC2_CR2_EXTSEL);       // Clear the external trigger selection bits
    ADC2->CR2 &= (uint8_t)(~ADC2_CR2_EXTTRIG);      // Disable the selected external trigger
    ADC2->CR2 |= (uint8_t)(ADC2_EXTTRIG_TIM);       // Set the selected external trigger

    ADC2->CSR |= (uint8_t)ADC2_CSR_EOCIE;           // Enable the ADC2 interrupts
    
    start_conversion();
}

void adc_conversion(void)
{
    start_conversion();
}

uint16_t adc_get_raw_value(void)
{
    return middle_arif_calc(&arif_filter);
}

INTERRUPT_HANDLER(ADC2_IRQHandler, ITC_IRQ_ADC2)
{
    uint16_t tmp =  (uint16_t)((ADC2->DRL) | 
                    (uint16_t)((ADC2->DRH) << (uint8_t)8));

    if (cnt_idle_measurement > 0)
    {
        cnt_idle_measurement--;
    }
    else
    {
        middle_arif_add(&arif_filter, tmp);
    }

    // Clear the ADC2 EOC Flag.
    ADC2->CSR &= (uint8_t)(~ADC2_CSR_EOC);
}
