//
// Created by Piotr Sperka on 23.01.2021.
//

#include "bootloader.h"
#include "main.h"

char firmwareByte, firmware[MAX_LINES][MAX_BYTES];
uint8_t sign,dataCounter, whichByte,length, byteCounter=0, checksum, line_error;
uint16_t address, hex_type;
uint32_t whichLine;
uint64_t bytesSum;

uint8_t ledcounter;


void fileCompile (char firmwareByte) { //

	if (ledcounter == 0xFF) HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
	ledcounter++;

	if (firmwareByte == ':') {

		whichLine++;
		bytesSum = 0;
		byteCounter = 0;
		sign = 0;
	}

	switch (sign) {
		case 0: // ':'
			firmware[whichLine][0] = firmwareByte;
			sign = 1;
			break;
		case 1: // length 1
			firmware[whichLine][1] = firmwareByte;
			sign = 2;
			break;
		case 2: // length 2
			firmware[whichLine][2] = firmwareByte;
			length = ((hexCharToUint(firmware[(whichLine)][1]) << 4) | (hexCharToUint(firmware[(whichLine)][2])));
			bytesSum = bytesSum + length;
			sign = 3;
			break;
		case 3: //address 1
			firmware[whichLine][3] = firmwareByte;
			sign = 4;
			break;
		case 4: //address 2
			firmware[whichLine][4] = firmwareByte;
			bytesSum = bytesSum + ((hexCharToUint(firmware[whichLine][3]) << 4) | (hexCharToUint(firmware[whichLine][4])));
			sign = 5;
			break;
		case 5: //address 3
			firmware[whichLine][5] = firmwareByte;
			sign = 6;
			break;
		case 6: ///address 4
			firmware[whichLine][6] = firmwareByte;
			bytesSum = bytesSum + ((hexCharToUint(firmware[whichLine][5]) << 4) | (hexCharToUint(firmware[whichLine][6])));
			sign = 7;
			break;
		case 7: //type 1
			firmware[whichLine][7] = firmwareByte;
			sign = 8;
			break;
		case 8: //type 2
			firmware[whichLine][8] = firmwareByte;
			hex_type = (hexCharToUint(firmware[whichLine][7]) << 4) | (hexCharToUint(firmware[whichLine][8]));
			bytesSum = bytesSum + hex_type;
			sign = 9;
			if (length == 0) sign = 10; //Only for end type, without any data
			break;
		case 9: //data

			if (byteCounter != (2*length - 1)) {
				firmware[whichLine][9+byteCounter] = firmwareByte;
				if (byteCounter % 2 != 0)
					bytesSum = bytesSum + ((hexCharToUint(firmware[whichLine][8+byteCounter]) << 4) | (hexCharToUint(firmware[whichLine][9+byteCounter])));
			}
			if (byteCounter == (2*length - 1)) {
				firmware[whichLine][9+byteCounter] = firmwareByte;
				bytesSum = bytesSum + ((hexCharToUint(firmware[whichLine][8+byteCounter]) << 4) | (hexCharToUint(firmware[whichLine][9+byteCounter])));
				byteCounter = 0;
				sign = 10;
			}
			byteCounter++;
			break;
		case 10: //checksum1
			firmware[whichLine][(9 + 2*length)] = firmwareByte;
			sign = 11;
			break;
		case 11: //checksum2
			firmware[whichLine][(10 + 2*length)] = firmwareByte;
			checksum = ((hexCharToUint(firmware[(whichLine)][(9 + 2*length)]) << 4) | (hexCharToUint(firmware[(whichLine)][(10 + 2*length)])));
			uint8_t checksumCalc = (~bytesSum) + 1;
			if (checksum != checksumCalc) line_error = whichLine;

			if (hex_type == end_type) { //End of: program
				EraseUserApplication();
				flashProgram();
			}
			sign = 12;
			break;
		case 12:
			//just wait for next line, this case ignore for example '/r' or something like that
			break;
	}

}





uint8_t EraseUserApplication() {
    HAL_StatusTypeDef success = HAL_ERROR;
    uint32_t errorSector = 0;

    if (HAL_FLASH_Unlock() == HAL_OK) {
        FLASH_EraseInitTypeDef eraseInit = {0};
        eraseInit.NbSectors = 18; // Count of sectors to erase
        eraseInit.Sector = FLASH_SECTOR_5; // First sector to erase
        eraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3; //
        eraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;

        success = HAL_FLASHEx_Erase(&eraseInit, &errorSector);

        HAL_FLASH_Lock();
    }

    return success == HAL_OK ? 1 : 0;
}


int flashProgram () {
	uint32_t 	RAWfirmware;
	uint8_t  	RAWByteCounter;
	uint32_t  	appStartAddr;
	uint8_t 	makeByte;
	uint32_t 	startFlashAddress;

	for (uint16_t 	lineCounter = 1; lineCounter < whichLine+1; lineCounter++) {
		HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

			uint8_t     type = ((hexCharToUint(firmware[lineCounter][7]) << 4) | (hexCharToUint(firmware[lineCounter][8]))); //Get type
			uint16_t   	length = ((hexCharToUint(firmware[lineCounter][1]) << 4) | (hexCharToUint(firmware[lineCounter][2])));
			uint16_t 	address = ((hexCharToUint(firmware[lineCounter][3]) << 12)| (hexCharToUint(firmware[lineCounter][4]) << 8) |
							(hexCharToUint(firmware[lineCounter][5]) << 4) | (hexCharToUint(firmware[lineCounter][6])));

		if (type == flashaddr_type){
				uint8_t makeByte1=(hexCharToUint(firmware[lineCounter][9]) << 4) | hexCharToUint(firmware[lineCounter][10]);
				uint8_t makeByte2=(hexCharToUint(firmware[lineCounter][11]) << 4) | hexCharToUint(firmware[lineCounter][12]);
				startFlashAddress = (makeByte1 << 24 | makeByte2 << 16);
		}

	    if (type == data_type) {
	    		RAWByteCounter = 1;
	    	    for (uint16_t ASCIICounter = 0; ASCIICounter < ((2*length) - 1); ASCIICounter += 2) { //ACII Counter (2 sign = 1 Byte)
	    	    	makeByte=(hexCharToUint(firmware[lineCounter][ASCIICounter + 9]) << 4) | hexCharToUint(firmware[lineCounter][ASCIICounter + 10]); //make bytes
	    	    	RAWfirmware = (RAWfirmware << 8) | makeByte;
	    	    	if  ((RAWByteCounter == 4) || (RAWByteCounter == 8) || (RAWByteCounter == 12) || (RAWByteCounter == 16)){ //if 4 bytes +offset
	    	    		flashMemory((startFlashAddress | (address + RAWByteCounter - 4)), RAWfirmware);
	    	    		}
	    	    		if (RAWByteCounter == length) RAWByteCounter = 0;
	    	    		RAWByteCounter++; //in next step RAWByteCounter +1
	    	    	}

	    	    }

		if (type ==  end_type) {
			NVIC_SystemReset;
		}

		if (type ==  appaddr_type) {
			RAWByteCounter = 1;
			for (uint16_t ASCIICounter = 0; ASCIICounter < ((2*length) - 1); ASCIICounter += 2) {
				makeByte=(hexCharToUint(firmware[lineCounter][ASCIICounter + 9]) << 4) | hexCharToUint(firmware[lineCounter][ASCIICounter + 10]); //make bytes
				appStartAddr = (appStartAddr << 8) | makeByte;
				RAWByteCounter++;
			}
		}
	} //End of: loop
	return 0;
}




typedef void (*pFunction)(void);
int JumpToAddress(uint32_t addr) {
    uint32_t JumpAddress = *(uint32_t *) (addr + 4);
    pFunction Jump = (pFunction) JumpAddress;

    HAL_RCC_DeInit();
    HAL_DeInit();
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    SCB->VTOR = addr;
    __set_MSP(*(uint32_t *) addr);

    Jump();
    return 0;
}

void JumpToApplication() {
    JumpToAddress(APP_ADDRESS);
}

void JumpToBootloader() {
    JumpToAddress(BOOT_ADDRESS);
}

uint8_t hexCharToUint(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else {
        return 0;  // Znak nie jest heksadecymalny
    }
}
uint8_t hexCharsToUint(char high, char low) {
    return (hexCharToUint(high) << 4) | hexCharToUint(low);
}

uint32_t read_uint32_from_flash(uint32_t* flash_address){
    uint32_t value = *flash_address;
    return value;
}
uint8_t problemsCounter = 0;
void flashMemory (uint32_t address, uint32_t RAWfirmware) {
	if (HAL_FLASH_Unlock() == HAL_OK) {
		uint32_t swapped = __builtin_bswap32(RAWfirmware);

		HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, swapped);
		uint32_t readWriteData = read_uint32_from_flash(address);

 		if (readWriteData == swapped) {
            HAL_FLASH_Lock();
        }
        if (readWriteData != swapped) {
        	problemsCounter++;
        	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, RAWfirmware); //try one more
            HAL_FLASH_Lock();
        }
	}
}


uint8_t UserApplicationExists() {
    uint32_t bootloaderMspValue = *(uint32_t *) (FLASH_BASE);
    uint32_t appMspValue = *(uint32_t *) (APP_ADDRESS);

    return appMspValue == bootloaderMspValue ? 1 : 0;
}


