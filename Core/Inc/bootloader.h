//
// Created by Piotr Sperka on 23.01.2021.
//

#ifndef BOOTLOADER_BOOTLOADER_H
#define BOOTLOADER_BOOTLOADER_H

#include "stm32f4xx_hal.h"


#define APP_ADDRESS (uint32_t)0x08020000
#define BOOT_ADDRESS (uint32_t)0x08000000

//hex line type
#define data_type 0x00 //not for edit
#define end_type 0x01 //not for edit
#define flashaddr_type 0x04 //not for edit
#define appaddr_type 0x05 //not for edit


#define MAX_LINES 1000 //Max lines in hex file
#define MAX_BYTES 50 //never be more


void fileCompile (char firmwareByte);
uint8_t EraseUserApplication();
int flashProgram ();
int JumpToAddress(uint32_t addr);
void JumpToApplication();
void JumpToBootloader();
uint8_t hexCharToUint(char c);
uint8_t hexCharsToUint(char high, char low);
uint32_t read_uint32_from_flash(uint32_t* flash_address);
void flashMemory (uint32_t address, uint32_t RAWfirmware);
uint8_t UserApplicationExists();




#endif //BOOTLOADER_BOOTLOADER_H
