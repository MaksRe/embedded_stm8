/**
 *
 *
 * 
 */
 
#ifndef _FLASH_H_
#define _FLASH_H_

    #pragma pack(push, 1)

        typedef struct {

            uint8_t first_init;

            // Весь список доступных данных.
            // Например, 
            // char param1[2];
            // uint8_t param2;
            // uint16_t param3
            // ...

        } eeprom_t;

    #pragma pack(pop)
    
    extern @eeprom eeprom_t eeprom_map;
    
    void flash_init_ob(void);
    void flash_init(void);
    void write_to_eeprom(void *dest, void *src, const uint8_t num_of_bytes);
    
#endif

