/*
 *              Copyright (c)  2025
 *  File header: 		nami_ir.h
 *  Created on : 		April 09, 2025
 *      Author : 		R&D Firmware Team (Firmware Developer)
 *      Contact:        hai.luong@namismart.vn
 */

#ifndef NAMI_IR_H_
#define NAMI_IR_H_


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
#include <bl_gpio.h>
#include <stdlib.h>
#include <mqtt.h>
#include "hal_wifi.h"
#include "easyflash_common.h"
#include "nami_ir_recv.h"
#include "LibSupport.h"
#include <hosal_cvt.h>
#include "plog.h"
#include "main.h"


#include <hosal_i2c.h>


/******************************************************************************/
/******************************************************************************/
/***                           InfraRed Transmiter                           **/
/******************************************************************************/
/******************************************************************************/

#define TEST_CONTROL
//#define TEST_AUTO_1


#define PWM_Get_Channel_Reg(ch)         (PWM_BASE+PWM_CHANNEL_OFFSET+(ch)*0x20)
#define PWM_STOP_TIMEOUT_COUNT          (160*1000)


#define MAXSIZE  				        500
#define TEMPERATURE_SIZE                1
#define CHARIRSIZE						800
#define DIVIDE_TIMER                    5
#define TYPE_NAMI_U16                   65535
#define NAMI_ENABLE                     100
#define NAMI_DISABLE                    103


#define INDEX_SIZE						1
#define LENGTH_SIZE						1
#define COPY_SIZE					    (LENGTH_SIZE - 2)


#define STATUS_NULL						0
#define STATUS_HAFT_BUFFER				1
#define STATUS_FULL_BUFFER				2


typedef enum{
	E_NORMAL,
	E_ZERO,
	E_FINISH
}merge_e;

typedef enum{
	IR_SUSPEND,
	IR_REUMSE,
	IR_SEND,
	IR_IDLE
}IR_SEND_STATUS;

typedef enum{
	AIR_CONDITIONER = 0,
	TELEVISION,
	AC_FAN
}IR_DEVICE_TYPE;



#ifdef NAMI_I2C_MASTER_SEND
#define IR_I2C_MASTER_SEND					NAMI_I2C_MASTER_SEND
#define IR_I2C_MASTER_RECV					NAMI_I2C_MASTER_RECV
#else
#define IR_I2C_MASTER_SEND					GDB_I2C_MASTER_SEND
#define IR_I2C_MASTER_RECV					GDB_I2C_MASTER_RECV
#endif


#ifdef HOSAL_PARAMETR_AHT20_ADDRESS
#define IR_I2C_PARA_ADDRESS                 HOSAL_PARAMETR_AHT20_ADDRESS  
#define IR_I2C_PARA_INTI                    HOSAL_PARAMETR_CMD_INIT      
#define IR_I2C_PARA_TRIGGER                 HOSAL_PARAMETR_CMD_TRIGGER     
#define IR_I2C_PARA_RESET                   HOSAL_PARAMETR_CMD_SOFT_RESET 
                           
#define IR_I2C_PARA_ID                      HOSAL_PARAMETR_I2C_ID        
#define IR_I2C_PARA_SPEED                   HOSAL_PARAMETR_I2C_SPEED      
#define IR_I2C_PARA_TIMEOUT                 HOSAL_PARAMETR_I2C_TIMEOUT_MS
#else
#define IR_I2C_PARA_ADDRESS                 GDB_PARAMETR_AHT20_ADDRESS  
#define IR_I2C_PARA_INTI                    GDB_PARAMETR_CMD_INIT      
#define IR_I2C_PARA_TRIGGER                 GDB_PARAMETR_CMD_TRIGGER     
#define IR_I2C_PARA_RESET                   GDB_PARAMETR_CMD_SOFT_RESET 
                           
#define IR_I2C_PARA_ID                      GDB_PARAMETR_I2C_ID        
#define IR_I2C_PARA_SPEED                   GDB_PARAMETR_I2C_SPEED      
#define IR_I2C_PARA_TIMEOUT                 GDB_PARAMETR_I2C_TIMEOUT_MS
#endif
typedef struct{
	// uint8_t DivTimer;
	// uint32_t BeginCount;
	// uint32_t DetectCount;
	uint32_t Count;
	uint32_t Index;
	uint32_t Length;
	uint32_t TimesOfPeriod;
	uint32_t PulseWidth[MAXSIZE];

	uint16_t TempPulse[MAXSIZE];
	uint32_t TempCount;
	// uint32_t *PulseWidthPtr;
	// uint32_t *PulseWidthPtrCount;
	uint8_t Temperature;
	uint8_t EasyFlashBuff[CHARIRSIZE];
    uint16_t EasyFlashBuffCount;

	uint16_t TotalLengthPlus;
	int BestDivisor;

	uint16_t PulseBuffer[INDEX_SIZE][LENGTH_SIZE];
	uint8_t CountCpBuf;
	uint8_t supplemental;
	uint8_t Totalsup;
	uint8_t  StatusBuffer[INDEX_SIZE];
	uint16_t PulseDelay[1];
	uint16_t CountDelay;
	uint8_t ReadySend;

	bool    DetectEndulseFlag;
	// bool    BeginCountFlag;
	// bool    DetectCountFlag;
	// bool    SendPulseHighFlag;
	// bool    SendPulseLowFlag;
}nami_sys_t;


typedef struct{
	uint8_t DebugGpio;
	uint8_t PWMGpio;
	PWM_CH_ID_Type PWMChannel;
	uint16_t clkDiv;
	uint16_t period;
	uint16_t threshold2;
	hosal_timer_dev_t timer;
	nami_sys_t NamiSystem;
	uint8_t *KeyNumber;
	// uint8_t *Status;
	uint8_t *UpdateFWFlag;
	uint8_t UpdateLogFlag;
	bool TransmitIRFlag;
	bool ReceiveIRFlag;
	bool ActiveFlag;
	float temperature;
	float humidity;
	TaskHandle_t NamiIRTaskHandle;
	void (*init)(void *);
	void (*transmiter)(void*);
}nami_ir_t;

extern TaskHandle_t NamiIRTaskHandle;
extern nami_ir_t NAMI_IR;
extern void InitNamiIR(void *pvParameters);
extern void NamiListen(void *pvParameters);
extern void NamiIRControlSend(int status);
// extern void NamiSuspendIR(void);
extern uint8_t NamiMergeIR(uint16_t *ir_data, uint16_t ir_data_len);
// extern void NamiIRControlTask(IR_SEND_STATUS status);

// extern char current_version[15];


#endif /* NAMI_IR_H_ */
