/* Registers required to enable writing to the EEPROM area
 *
 */

/*	FLASH section
 */
volatile char FLASH_CR1     @0x505a;	/* Flash Control Register 1 */
volatile char FLASH_CR2     @0x505b;	/* Flash Control Register 2 */
volatile char FLASH_NCR2    @0x505c;	/* Flash Complementary Control Reg 2 */
volatile char FLASH_FPR     @0x505d;	/* Flash Protection reg */
volatile char FLASH_NFPR    @0x505e;	/* Flash Complementary Protection reg */
volatile char FLASH_IAPSR   @0x505f;	/* Flash in-appl Prog. Status reg */
volatile char FLASH_PUKR    @0x5062;	/* Flash Program memory unprotection reg */
volatile char FLASH_DUKR    @0x5064;	/* Data EEPROM unprotection reg */


