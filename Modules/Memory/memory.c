/**
 *
 *
 *
 */

#include "stm8s.h"
#include "stm8s_flash.h"
#include "stm8s_flash_extra.h"

#include "memory.h"
#include "string.h"
#include "uds.h"
#include "can_driver.h"
#include "can.h"
#include "dtc.h"

#define eeprom_lock         FLASH->IAPSR &= (uint8_t)FLASH_MEMTYPE_DATA;
#define eeprom_unlock       FLASH->DUKR = FLASH_RASS_KEY2;\
                            FLASH->DUKR = FLASH_RASS_KEY1;

#define flash_lock          FLASH->IAPSR &= (uint8_t)FLASH_MEMTYPE_DATA;
#define flash_unlock        FLASH->DUKR = FLASH_RASS_KEY1;\
                            FLASH->DUKR = FLASH_RASS_KEY2;

// Адреса Options Bytes
#define OPT_FLASH_WAIT_STATES       0x480D
#define OPT_FLASH_BOOTLOADER        0x487E // Записать 0x55AA для активации встроенного bootloader

#define ALLOWED_WAIT_STATES         0x01
#define WAIT_STATES_IS_ALLOWED      0x01FE // при данном значении в Option Byte - WAIT_STATES разрешен 

#define ALLOW_BOOTLOADER            0x55 // значение по адресу OPT_FLASH_BOOTLOADER для активации встроенного загрузчика
#define BOOTLOADER_IS_ALLOWED       0x55AA // при данном значении в Option Byte - bootloader разрешен 

@eeprom eeprom_t eeprom_map @0x4000;

eeprom_t memory_map = {
    1,

    // Тут список дефолтных данных памяти
    // ...
};

void flash_init_ob(void)
{
    eeprom_unlock;
    
        // Инициализация Option Bytes
        // Установка Flash_Wait_States, так как используется внешний кварц (24 MHz)
        if (FLASH_ReadOptionByte(OPT_FLASH_WAIT_STATES) != WAIT_STATES_IS_ALLOWED)
            FLASH_ProgramOptionByte(OPT_FLASH_WAIT_STATES, ALLOWED_WAIT_STATES);
        
        // Активация bootloader
        if (FLASH_ReadOptionByte(OPT_FLASH_BOOTLOADER) != BOOTLOADER_IS_ALLOWED)
            FLASH_ProgramOptionByte(OPT_FLASH_BOOTLOADER, ALLOW_BOOTLOADER);
            
    eeprom_lock;
}

/*******************************************************************************
 * Инициализация flash памяти и Options Bytes
 */
void flash_init(void)
{
    eeprom_unlock;

        // Проверка eeprom на первоначальную инициализацию памяти
        if (eeprom_map.first_init != 0x1)
        {
            memcpy(
                &eeprom_map, 
                &memory_map, 
                sizeof(eeprom_t)
            );
        }
    
    eeprom_lock;
}

void write_to_eeprom(void *dest, void *src, const uint8_t num_of_bytes)
{
    uint16_t timeout = 0xffff;
    
    disableInterrupts();

        // Data EEPROM unlocked flag mask
        if (!(FLASH->IAPSR & FLASH_IAPSR_DUL))
        {
            FLASH_DUKR = 0xAE; // unlock EEPROM
            FLASH_DUKR = 0x56;
        }

        // check protection off
        while (!(FLASH->IAPSR & FLASH_IAPSR_DUL) && timeout)
            timeout--;

        memcpy(dest, src, num_of_bytes);

        FLASH->IAPSR &= (uint8_t)~(FLASH_IAPSR_DUL); // lock EEPROM

    enableInterrupts();
}
    
