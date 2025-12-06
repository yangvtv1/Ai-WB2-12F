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
#include <hosal_cvt.h>
#include <LibSupport.h>
// #include "pds_level.h"
#include <hal_sys.h>

#include <hosal_adc.h>
#include <math.h>



#define RETENTION_RAM_BASE 									0x40010000

#define WAKEUP_FALLING_PIN 			  					    7
#define WAKEUP_RISING_PIN 			  					    8
#define STATUS_DOOR                                         14
#define PDS_WAKEUP_MS         								50


/**********   BL602  ************
*    channel0   ----->     gpio12 
*    channel1   ----->     gpio4
*    channel2   ----->     gpio14
*    channel3   ----->     gpio13
*    channel4   ----->     gpio5
*    channel5   ----->     gpio6
*    channel7   ----->     gpio9
*    channel9   ----->     gpio10
*    channel10  ----->     gpio11
*    channel11  ----->     gpio15
*/

// channel 0
#define GPIO_ADC_PIN 11
#define ADC_CHANNEL 10
// // channel 2
// #define GPIO_ADC_PIN 14
// #define ADC_CHANNEL 2
// channel 4
// #define GPIO_ADC_PIN_4 4
// #define ADC_CHANNEL_4 1
// // channel 10
// #define GPIO_ADC_PIN 11
// #define ADC_CHANNEL 10



#ifdef NAMI_DATA_WIDTH_8BIT
#define NAMI_PRE_DATA_WIDTH_8BIT                        NAMI_DATA_WIDTH_8BIT
#define NAMI_PRE_NO_PARITY                              NAMI_NO_PARITY
#define NAMI_PRE_STOP_BITS_1                            NAMI_STOP_BITS_1
#define NAMI_PRE_UART_MODE_POLL                         NAMI_UART_MODE_POLL
#define NAMI_PRE_OPERATE_UART_MODE                      NAMI_OPERATE_UART_MODE
#define NAMI_PRE_OPERATE_STP_BITS                       NAMI_OPERATE_STP_BITS
#define NAMI_PRE_OPERATE_PARITY                         NAMI_OPERATE_PARITY
#define NAMI_PRE_OPERATE_DATA_WIDTH                     NAMI_OPERATE_DATA_WIDTH
#define NAMI_PRE_SYSTEM_INIT                            NAMI_SYSTEM_INIT
#define NAMI_PRE_UART_INIT                              NAMI_UART_INIT
#define NAMI_PRE_ADC_INIT                               NAMI_ADC_INIT
#define NAMI_PRE_ADC_ADD_CHANNEL                        NAMI_ADC_ADD_CHANNEL
#define NAMI_PRE_GPIO_ENABLE_INPUT                      NAMI_GPIO_ENABLE_INPUT
#define NAMI_PRE_GPIO_INPUT_GET_VALUE                   NAMI_GPIO_INPUT_GET_VALUE
#define NAMI_PRE_GPIO_OUTPUT_SET                        NAMI_GPIO_OUTPUT_SET
#define NAMI_PRE_UART_RECEIVE                           NAMI_UART_RECEIVE
#define NAMI_PRE_ADC_VALUE_GET                          NAMI_ADC_VALUE_GET
#define NAMI_PRE_SYSTEM_RESET                           NAMI_SYSTEM_RESET
#define NAMI_PRE_GPIO_ENABLE_OUTPUT                     NAMI_GPIO_ENABLE_OUTPUT
#define NAMI_PRE_GPIO_IRQ_MASK                          NAMI_GPIO_IRQ_MASK
#define NAMI_PRE_GPIO_INIT                              NAMI_GPIO_INIT
#define NAMI_PRE_GPIO_IRQ_SET                           NAMI_GPIO_IRQ_SET
#else
#define NAMI_PRE_DATA_WIDTH_8BIT                        HOSAL_DATA_WIDTH_8BIT
#define NAMI_PRE_NO_PARITY                              HOSAL_NO_PARITY
#define NAMI_PRE_STOP_BITS_1                            HOSAL_STOP_BITS_1
#define NAMI_PRE_UART_MODE_POLL                         HOSAL_UART_MODE_POLL
#define NAMI_PRE_OPERATE_UART_MODE                      HOSAL_OPERATE_UART_MODE
#define NAMI_PRE_OPERATE_STP_BITS                       HOSAL_OPERATE_STP_BITS
#define NAMI_PRE_OPERATE_PARITY                         HOSAL_OPERATE_PARITY
#define NAMI_PRE_OPERATE_DATA_WIDTH                     HOSAL_OPERATE_DATA_WIDTH
#define NAMI_PRE_SYSTEM_INIT                            HOSAL_SYSTEM_INIT
#define NAMI_PRE_UART_INIT                              HOSAL_UART_INIT
#define NAMI_PRE_ADC_INIT                               HOSAL_ADC_INIT
#define NAMI_PRE_ADC_ADD_CHANNEL                        HOSAL_ADC_ADD_CHANNEL
#define NAMI_PRE_GPIO_ENABLE_INPUT                      HOSAL_GPIO_ENABLE_INPUT
#define NAMI_PRE_GPIO_INPUT_GET_VALUE                   HOSAL_GPIO_INPUT_GET_VALUE
#define NAMI_PRE_GPIO_OUTPUT_SET                        HOSAL_GPIO_OUTPUT_SET
#define NAMI_PRE_UART_RECEIVE                           HOSAL_UART_RECEIVE
#define NAMI_PRE_ADC_VALUE_GET                          HOSAL_ADC_VALUE_GET
#define NAMI_PRE_SYSTEM_RESET                           HOSAL_SYSTEM_RESET
#define NAMI_PRE_GPIO_ENABLE_OUTPUT                     HOSAL_GPIO_ENABLE_OUTPUT
#define NAMI_PRE_GPIO_IRQ_MASK                          HOSAL_GPIO_IRQ_MASK
#define NAMI_PRE_GPIO_INIT                              HOSAL_GPIO_INIT
#define NAMI_PRE_GPIO_IRQ_SET                           HOSAL_GPIO_IRQ_SET
#endif




typedef struct {
    uint32_t RetentionRam;
    // uint16_t var2;
    // uint8_t var3;
} RetentionData;
volatile RetentionData *retention_data;

#define TRIGGER_FACTOR 17
typedef struct{
	uint32_t *RAMRetention;
	uint8_t *MainStatusBroker;
    uint32_t TimeSec;
    uint8_t *MainSetCount;
    
    uint16_t *MainDuration;
    int32_t ResultData;
    uint8_t PercentLight;
    uint8_t SampleLight;
    uint8_t SignalCurr;
    uint8_t SignalOld;

    uint8_t PresenBuf[100];
    uint16_t PresenCnt;

    float TotalCalib;
    float Log10Result;
    uint32_t SettingFactorRaw;
    float RawSettingFactorOfficial[TRIGGER_FACTOR];


    uint8_t UartReceiveBuffer[200];
    uint32_t UartReceiveCount;
    uint32_t UartPostionHead;
    uint32_t UartPostionTail;
    uint8_t UartGetLen;
    uint8_t UartValDistanceBuff[10];
    uint16_t UartValDistance;
    uint8_t UartValDistanceApprove;
    uint8_t UartNoDetect;
    uint8_t CountDetectMotion;
    uint16_t UartReceiveAck;
    uint16_t UartReceivePercent;

    uint16_t UartMotionTrigger;
    uint16_t UartMotionHoldThreshold;
    uint16_t UartMicroMotionHoldThreshold;

    uint8_t UpdateFirmwareBuf[200];
    uint8_t *MainUpdateFirmwareFlag;

    uint8_t TimerPublish;
    bool *GetStatusConnectFlag;
    bool *MChangeCalibFlag;
    bool *MModeCalibFlag;

    bool *MainConfigBeginFlag;
}general_t;



extern general_t Gen;

// uint32_t sleep_time_ms = 0; // deep sleep time, If you do not need to periodically wake up, set sleep_time_ms=0
uint8_t wakeup_pins[2];
uint8_t PinPercent;

extern void ProcPresenceMQTT2way(uint8_t *_array, uint16_t Len);


#endif /* MAIN_H_ */
