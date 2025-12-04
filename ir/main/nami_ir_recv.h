/*
 *              Copyright (c)  2025
 *  File header: 		nami_ir.h
 *  Created on : 		April 09, 2025
 *      Author : 		R&D Firmware Team (Firmware Developer)
 *      Contact:        hai.luong@namismart.vn
 */

#ifndef NAMI_IR_RECV_H_
#define NAMI_IR_RECV_H_


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
#include <hosal_timer.h>
#include "LibSupport.h"
#include <hosal_cvt.h>
#include <bl_gpio.h>
#include <stdlib.h>
#include <mqtt.h>
#include "hal_wifi.h"
#include "easyflash_common.h"

#include "bl602_ir.h"
#include "bl602_glb.h"
#include "bflb_platform.h"
#include <blog.h>
#include "bl_ir.h"

//#include "nami_ir_std.h"
#include "nami_ir.h"
#include "ir_reg.h"
#include "bl602_common.h"
#include "bl602_hbn.h"
#include "stdio.h"
#include "nami_ir_recv.h"




	#define IR_PIN_TX                       GLB_GPIO_PIN_11
	#define IR_PIN_RX                       GLB_GPIO_PIN_12

#define LEN_DATA    4

#ifdef HOSAL_PARAMETER_IR_DIV_CLOCK
#define PARAMETER_IR_DIV_CLOCK              HOSAL_PARAMETER_IR_DIV_CLOCK
#define PARAMETER_IR_COUNT_TIMEOUT          HOSAL_PARAMETER_IR_COUNT_TIMEOUT
#else
#define PARAMETER_IR_DIV_CLOCK              1
#define PARAMETER_IR_COUNT_TIMEOUT          100
#endif


/******************************************************************************/
/******************************************************************************/
/***                           InfraRed Transmiter                           **/
/******************************************************************************/
/******************************************************************************/

typedef struct{
    void (*NamiInitRecv)(void);
    BL_Err_Type (*NamiIR_RxInit)(IR_RxCfg_Type *);
    BL_Err_Type (*IR_NEC_TR)(void);
    void (*NMIR_Init)(void);
    void (*SuspendTask)(void);
    void (*ResumeTask)(void);
}nami_ir_recv_t;

//extern TaskHandle_t NamiRecvCallBackhandle;
// extern uint16_t rxdata[LEN_DATA];
// extern uint32_t ret;
// extern uint16_t rxdata_1[LEN_DATA];
// extern uint32_t ret_1;
extern void NamiIR_Init(uint8_t DataBit);
extern bool CopyBufferFlag;
extern nami_ir_recv_t NamiIRRecv;
extern TaskHandle_t NamiIRRecvTaskHandle;
extern void NamiInitRecv(void);
extern void NamiRecvCallBack(void *pvParameters);
extern uint32_t _IR_NEC_TR(void);
extern uint32_t NMIR_LearnToReceive(IR_RxMode_Type mode, uint16_t* U16Data, bool _ModeTimeoutFlag);
extern void NamiSuspendTask(void);
#endif /* NAMI_IR_RECV_H_ */
