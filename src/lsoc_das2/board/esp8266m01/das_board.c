/**
 *
 *  Filename:      das_board.c
 *
 *  Description:   BSP adapative layer
 *
 *  Created by strawmanbobi 2022-01-10
 *
 *  Copyright (c) 2022 Aliyun-IoT
 *
 **/

#include <stdlib.h>

#include "IRBabyIRIS.h"

size_t das_hal_firmware_version(char *buf, size_t size) {
    return getIRISKitVersion(buf, size);
}

size_t das_hal_device_id(char *buf, size_t size) {
    return getDeviceID(buf, size);
}

#if (DAS_SERVICE_ROM_ENABLED)

extern uint32_t __flash_text_start__;
extern uint32_t __flash_text_end__;

int das_hal_rom_info(das_rom_bank_t banks[DAS_ROM_BANK_NUMBER]) {

    banks[0].address = &__flash_text_start__;
    banks[0].size = &__flash_text_end__ - &__flash_text_start__;

    return DAS_ROM_BANK_NUMBER;
}

#endif /* DAS_SERVICE_ROM_ENABLED */
