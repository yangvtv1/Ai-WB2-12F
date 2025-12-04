/*
 *              Copyright (c)  2025
 *  File header: 		main.h
 *  Created on : 		Jul 04, 2025
 *      Author : 		R&D Firmware Team (Firmware Developer)
 *      Contact:        hai.luong@namismart.vn
 */

#ifndef MAIN_H_
#define MAIN_H_


#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <hosal_timer.h>
#include <bl_gpio.h>
#include <blog.h>
#include <stdint.h>
#include <bl602.h>
#include <bl602_gpio.h>
#include <bl602_glb.h>
#include <bl_pwm.h>
#include "LibSupport.h"
#include <hosal_cvt.h>
#include "string.h"
#include "stdbool.h"
#include <bl602_pwm.h>
#include <stdlib.h>
#include <mqtt.h>
#include "hal_wifi.h"
#include "easyflash_common.h"
#include "plog.h"
#include "bl_sys.h"
#include "hosal_rng.h"
#include "hal_hbn.h"

#include <aos/yloop.h>
#include <aos/kernel.h>
#include <bl_uart.h>
#include <lwip/tcpip.h>
#include "wifi_execute.h"
#include "switch.h"
#include <hosal_uart.h>
#include <hosal_dma.h>

#include <stdint.h>
#include <bl_gpio.h>

#include "nami_ir.h"
#include "nami_ir_recv.h"


#include "bl602_aon.h"
#include "bl602_hbn.h"
#include "bl602_glb.h"
#include "bl602_pds.h"
#include "bl602_sf_cfg.h"
#include "bl602_sf_ctrl.h"
#include "bl602_sflash.h"
#include "bl602_sflash_ext.h"
#include "bl602_spi.h"
#include "bl602_xip_sflash.h"
#include "bl602_common.h"
#include "bflb_platform.h"
#include <bl_irq.h>
#include <bl_gpio.h>
#include <bl_flash.h>
#include <bl602_glb.h>
// #include "pds_level.h"
#include <hal_sys.h>

#include <hosal_adc.h>




typedef struct {
    uint32_t RetentionRam;
    // uint16_t var2;
    // uint8_t var3;
} RetentionData;
volatile RetentionData *retention_data;

typedef struct{
    bool LedState;
    uint32_t LedOFFCounter;
    uint32_t LedONCounter;
}Blink_Led_t;

typedef struct{
	uint32_t *RAMRetention;
	uint8_t *MainStatusBroker;
    uint8_t TimeSec;
    uint8_t *MainSetCount;
    uint8_t *MainUpdateFirmwareFlag;
    bool *MainChangeCalibFlag;
    bool *MainWifiNetworkFlag;
    uint8_t *MainCheckNetworkCount;
    uint8_t PinPercent;
    uint8_t UpdateFirmwareBuf[200];
    bool *GetStatusConnectFlag;
    bool UpdateCompletedFlag;

    uint32_t ForceHibernateCounter;
    bool EnterHibernateFlag;

    char FlashSSID[33];
    char FlashPass[65];

    Blink_Led_t LedWifiState;

    TickType_t MainTimerHbnCurr;
    TickType_t MainTimeHbnOld;

    bool SleepModeFlag;
    TimerHandle_t TimerMainHandler;

    bool StateSuccessFlag;
    bool StateCompletedFlag;
}general_t;

extern general_t Gen;


#endif /* MAIN_H_ */
