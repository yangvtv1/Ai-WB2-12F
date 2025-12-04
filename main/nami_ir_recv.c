/*
 *              Copyright (c)  2025
 *  File header: 		nami_ir.c
 *  Created on : 		April 09, 2025
 *      Author : 		R&D Firmware Team (Firmware Developer)
 *      Contact:        hai.luong@namismart.vn
 */


#include "nami_ir_recv.h"

/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/
nami_ir_recv_t NamiIRRecv;
TaskHandle_t NamiIRRecvTaskHandle;
// TaskHandle_t NamiRecvCallBackhandle = NULL;
static IR_TxCfg_Type txCfg = {
	32,                                                  /* 32-bit data */
	DISABLE,                                             /* Disable signal of tail pulse inverse */
	ENABLE,                                              /* Enable signal of tail pulse */
	DISABLE,                                             /* Disable signal of head pulse inverse */
	ENABLE,                                              /* Enable signal of head pulse */
	DISABLE,                                             /* Disable signal of logic 1 pulse inverse */
	DISABLE,                                             /* Disable signal of logic 0 pulse inverse */
	ENABLE,                                              /* Enable signal of data pulse */
	ENABLE,                                              /* Enable signal of output modulation */
	ENABLE                                              /* Disable signal of output inverse */
};

static IR_TxPulseWidthCfg_Type txPWCfg = {
	1,                                                   /* Pulse width of logic 0 pulse phase 1, 562.5us @2MHz source clock*/
	1,                                                   /* Pulse width of logic 0 pulse phase 0 */
	3,                                                   /* Pulse width of logic 1 pulse phase 1, 1687.5us */
	1,                                                   /* Pulse width of logic 1 pulse phase 0 */
	8,                                                   /* Pulse width of head pulse phase 1, 4.5ms */
	16,                                                  /* Pulse width of head pulse phase 0, 9ms */
	1,                                                   /* Pulse width of tail pulse phase 1 */
	1,                                                   /* Pulse width of tail pulse phase 0 */
	35,                                                  /* Modulation phase 1 width, 37.7kHz, duty=1/3 */
	18,                                                  /* Modulation phase 0 width, 37.7kHz, duty=1/3 */
	1125                                                 /* Pulse width unit */
};

static IR_RxCfg_Type rxCfg = {
	IR_RX_SWM, //IR_RX_NEC,                                           /* Set ir rx mode NEC */
	ENABLE,                                              /* Disable signal of input inverse */
	0xFFFF, //45000,                                                /* Pulse width threshold to trigger end condition, 4.5ms @2MHz source clock */
	10000,                                                /* Pulse width threshold for logic 0/1 detection, 1.7ms */
	DISABLE,                                             /* Disable input de-glitch function */
	0                                                    /* De-glitch function cycle count */
};

	
	uint16_t rxdata[600];
	uint32_t ret;
	// uint8_t rxdata_1[1];
	uint32_t ret_1;
	bool CopyBufferFlag;
	uint8_t length[1];
	uint16_t rxlength;
	
	bool ChangeFlag;
/******************************************************************************/
/******************************************************************************/
/***                      Private functions declare                          **/
/******************************************************************************/
/******************************************************************************/
uint32_t NMIR_LearnToReceive(IR_RxMode_Type mode, uint16_t* U16Data, bool _ModeTimeoutFlag);
BL_Err_Type _NamiIR_RxInit(IR_RxCfg_Type *irRxCfg);

/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/

/******************************************************************************/
/******************************************************************************/
/***                            Private functions                            **/
/******************************************************************************/
/******************************************************************************/
static void NMIR_GPIO_Init(void)
{
    // printf("\r\n[%d][%s]IR GPIO init\r\n", __LINE__, __func__);
    GLB_GPIO_Type gpioPins[2] = {IR_PIN_TX,IR_PIN_RX};
    GLB_GPIO_FUNC_Type gpioFuns[2] = {GPIO_FUN_ANALOG,GPIO_FUN_SWGPIO};

    /* IR LED driver must use gpio11 */
    //GLB_GPIO_Func_Init(gpioFuns[0],&gpioPins[0],1);
    /* IR RX gpio should run in SWGPIO mode */
    GLB_GPIO_Func_Init(gpioFuns[1],&gpioPins[1],1);
}


/****************************************************************************//**
 * @brief  IR TX and RX init
 *
 * @param  None
 *
 * @return None
 *
*******************************************************************************/
void _NMIR_Init(void)
{
    /* IR gpio init */
    NMIR_GPIO_Init();

    /* Enable ir tx driver */
    GLB_IR_LED_Driver_Enable();

    /* Select ir rx gpio */
    GLB_IR_RX_GPIO_Sel(IR_PIN_RX);

    /* Disable ir before config */
    IR_Disable(IR_RX);  //IR_TXRX

    /* IR tx init */
    // IR_TxInit(&txCfg);
    // IR_TxPulseWidthConfig(&txPWCfg);

    /* IR rx init */

    //IR_RxInit(&rxCfg);
	//NamiIRRecv.NamiIR_RxInit(&rxCfg);
	_NamiIR_RxInit(&rxCfg);
}
/******************************************************************************/
/******************************************************************************/
/***                      Processing functions Callback                      **/
/******************************************************************************/
/******************************************************************************/
BL_Sts_Type NMIR_GetIntStatus(IR_INT_Type intType)
{
    uint32_t tmpVal = 0;
    uint32_t ret_old = 0, ret = 0, re_read = 0;
    static uint8_t count = 10;
    
    /* Check the parameters */
    CHECK_PARAM(IS_IR_INT_TYPE(intType));
    
    /* Read tx or rx interrupt status */
    if(IR_INT_TX == intType){
        tmpVal = BL_RD_REG(IR_BASE,IRTX_INT_STS);
        tmpVal = BL_GET_REG_BITS_VAL(tmpVal,IRTX_END_INT);
    }
    else if(IR_INT_RX == intType){
    	tmpVal = BL_RD_REG(IR_BASE,IRRX_INT_STS);
        tmpVal = BL_GET_REG_BITS_VAL(tmpVal,IRRX_END_INT);
    }
    
    if(tmpVal)
    {
        return SET;
    }else{
        return RESET;
    }
}



// void NamiRecvRstBuf(void){
// 		memset(&rxdata, 0x0, sizeof(rxdata));
// 		ret = 0;
// }

// uint32_t _IR_NEC_TR(void){
// 	//static bool ChangeFlag = true;
// 	 uint16_t PulseIndex = 0;
//     //---------------------------------------------------------------------------------------------------------------------
// 	//memset(&LearnBuffer, 0, sizeof(LearnBuffer));


// 	// ret = NMIR_LearnToReceive(IR_RX_SWM, (uint16_t *)&rxdata);
// 	//memcpy(&NamiMQTT.LearnBuffer[0], &rxdata[0], sizeof(uint16_t) * LEN_DATA);
// 	//ret = IR_LearnToInit((uint32_t *)&rxdata, (uint8_t *)&length);

// 	//-------------------------------------------------------------------------------------------------------------------------------
//     if(ret != 0 && rxdata[0] != 0){
//     	printf("Data bit count: %d\r\n", ret);
// 		  for (int i = 0; i < ret; i++) {
// 			//  PulseIndex = (rxdata[i]/2 * 20) / 100;
// 			//  PulseIndex = (rxdata[i]/2) - PulseIndex;
// 			// rxdata_1[i] = PulseIndex;
// 			printf("%u, ", /*PulseIndex*/ rxdata[i]);
// 			if ((i + 1) % 8 == 0) {
// 			  printf("\r\n");
// 			}
// 		}
// 		printf("\r\n");
// 		NamiRecvRstBuf();
// 		// memset(&rxdata, 0x0, sizeof(rxdata));
// 		// ret = 0;
//     }


//     return 0;
// }

BL_Err_Type _NamiIR_RxInit(IR_RxCfg_Type *irRxCfg){
    uint32_t tmpVal;
    
    /* Check the parameters */
    CHECK_PARAM(IS_IR_RXMODE_TYPE(irRxCfg->rxMode));
    
    tmpVal = BL_RD_REG(IR_BASE,IRRX_CONFIG);
    // printf("\r\n[%d][%s] - BEFORE - IRRX_CONFIG_OFFSET = 0x%08lX \r\n", __LINE__, __func__, BL_RD_REG(IR_BASE,IRRX_CONFIG));
    /* Set rx mode */
    switch(irRxCfg->rxMode)
    {
        case IR_RX_NEC:
            tmpVal = BL_SET_REG_BITS_VAL(tmpVal,IR_CR_IRRX_MODE,0x0);
            break;
        case IR_RX_RC5:
            tmpVal = BL_SET_REG_BITS_VAL(tmpVal,IR_CR_IRRX_MODE,0x1);
            break;
        case IR_RX_SWM:
            tmpVal = BL_SET_REG_BITS_VAL(tmpVal,IR_CR_IRRX_MODE,0x2);
            break;
        default:
            break;
    }
    /* Enable or disable input inverse */
    ENABLE == irRxCfg->inputInverse ? (tmpVal=BL_SET_REG_BIT(tmpVal,IR_CR_IRRX_IN_INV)):(tmpVal=BL_CLR_REG_BIT(tmpVal,IR_CR_IRRX_IN_INV));
    /* Enable or disable rx input de-glitch function */
    ENABLE == irRxCfg->rxDeglitch ? (tmpVal=BL_SET_REG_BIT(tmpVal,IR_CR_IRRX_DEG_EN)):(tmpVal=BL_CLR_REG_BIT(tmpVal,IR_CR_IRRX_DEG_EN));
    /* Set de-glitch function cycle count */
    tmpVal = BL_SET_REG_BITS_VAL(tmpVal,IR_CR_IRRX_DEG_CNT,irRxCfg->DeglitchCnt);
    /* Write back */
    BL_WR_REG(IR_BASE,IRRX_CONFIG,tmpVal);
    
    tmpVal = BL_RD_REG(IR_BASE,IRRX_PW_CONFIG);
    // printf("\r\n[%d][%s] - BEFORE - IRRX_PW_CONFIG_OFFSET = 0x%08lX \r\n", __LINE__, __func__, BL_RD_REG(IR_BASE,IRRX_PW_CONFIG));
    /* Set pulse width threshold to trigger end condition */
    tmpVal = BL_SET_REG_BITS_VAL(tmpVal,IR_CR_IRRX_END_TH,irRxCfg->endThreshold-1);
    /* Set pulse width threshold for logic0/1 detection */
    tmpVal = BL_SET_REG_BITS_VAL(tmpVal,IR_CR_IRRX_DATA_TH,irRxCfg->dataThreshold-1);
    /* Write back */
    BL_WR_REG(IR_BASE,IRRX_PW_CONFIG,tmpVal);

    // printf("\r\n[%d][%s] - IRRX_CONFIG_OFFSET = 0x%08lX \r\n", __LINE__, __func__, BL_RD_REG(IR_BASE,IRRX_CONFIG));
    // printf("\r\n[%d][%s] - IRRX_PW_CONFIG_OFFSET = 0x%08lX \r\n", __LINE__, __func__, BL_RD_REG(IR_BASE,IRRX_PW_CONFIG));

    return SUCCESS;
}

uint32_t NMIR_LearnToReceive(IR_RxMode_Type mode, uint16_t* U16Data, bool _ModeTimeoutFlag){
		#define NEC_HEAD_H_MIN          17000
		#define NEC_HEAD_H_MAX          19000
		#define NEC_HEAD_L_MIN           8400
		#define NEC_HEAD_L_MAX           9600
		#define NEC_BIT0_H_MIN            525
		#define NEC_BIT0_H_MAX           1725
		#define RC5_ONE_PLUSE_MIN        1175
		#define RC5_ONE_PLUSE_MAX        2375
		#define RC5_TWO_PLUSE_MIN        2955
		#define RC5_TWO_PLUSE_MAX        4155
	
        uint32_t timeoutCnt;
        if(_ModeTimeoutFlag)
		    timeoutCnt = (PARAMETER_IR_COUNT_TIMEOUT*5);                  ///        (100*160*300);
        else
            timeoutCnt = (PARAMETER_IR_COUNT_TIMEOUT);

	    uint8_t length = 0;
	    // uint32_t timeoutCnt = IR_RX_INT_TIMEOUT_COUNT;
		uint32_t tmpVal;
	    uint32_t rxLen = 0;
		uint32_t U16RxLen = 0;
		uint16_t RealValue = 0;
		uint16_t OfficialValue = 0;


	    /* Check the parameters */
	    CHECK_PARAM(IS_IR_RXMODE_TYPE(mode));

		/* Disable ir rx unit */
		tmpVal = BL_RD_REG(IR_BASE,IRRX_CONFIG);
		BL_WR_REG(IR_BASE,IRRX_CONFIG,BL_CLR_REG_BIT(tmpVal,IR_CR_IRRX_EN));


	    /* Clear and mask rx interrupt */
	    tmpVal = BL_RD_REG(IR_BASE,IRRX_INT_STS);
	    BL_WR_REG(IR_BASE,IRRX_INT_STS,BL_SET_REG_BIT(tmpVal,IR_CR_IRRX_END_CLR));

		tmpVal = BL_RD_REG(IR_BASE,IRRX_INT_STS);
	    BL_WR_REG(IR_BASE,IRRX_INT_STS,BL_SET_REG_BITS_VAL(tmpVal,IR_CR_IRRX_END_MASK, MASK));

	    /* Enable ir rx */
	    tmpVal = BL_RD_REG(IR_BASE,IRRX_CONFIG);
	    BL_WR_REG(IR_BASE,IRRX_CONFIG,BL_SET_REG_BIT(tmpVal,IR_CR_IRRX_EN));

		/* Wait for rx interrupt */
		while(SET != NMIR_GetIntStatus(IR_INT_RX)){
			timeoutCnt--;
			if(IR_GetRxFIFOCount() != 0){
					/* Read data */
				 RealValue = BL_RD_REG(IR_BASE,IRRX_SWM_FIFO_RDATA)&0xffff;
				//OfficialValue = ((RealValue/2) - (((RealValue/2)*19)/100));

				// process LSB = 0x00 => 0x01, MSB keep value
				/*if(((OfficialValue) & 0xFF) == 0x00){
					U8Data[U8RxLen++] = ((OfficialValue >> 8) & 0xFF);
					U8Data[U8RxLen++] = (((OfficialValue) & 0xFF) | 0x01);
				}else if(((OfficialValue >> 8) & 0xFF) == 0x00){                          // process LSB = 0xFF => 0xFF, MSB = 0x00 => 0x00|0x80 => 0x80
					U8Data[U8RxLen++] = (((OfficialValue >> 8) & 0xFF) | 0x80);
					U8Data[U8RxLen++] = ((OfficialValue) & 0xFF);
				}else*/{
					U16Data[U16RxLen++] = RealValue/2;
				}
			}else if(!timeoutCnt){
				// Disable ir rx 
				tmpVal = BL_RD_REG(IR_BASE,IRRX_CONFIG);
				BL_WR_REG(IR_BASE,IRRX_CONFIG,BL_CLR_REG_BIT(tmpVal,IR_CR_IRRX_EN));
                U16Data[U16RxLen++] = 0;
                U16Data[U16RxLen++] = 0;
				return U16RxLen;
			}
		}

	    /* Disable ir rx */
	    tmpVal = BL_RD_REG(IR_BASE,IRRX_CONFIG);
		BL_WR_REG(IR_BASE,IRRX_CONFIG,BL_CLR_REG_BIT(tmpVal,IR_CR_IRRX_EN));


	    /* Clear rx interrupt */
	    tmpVal = BL_RD_REG(IR_BASE,IRRX_INT_STS);
	    BL_WR_REG(IR_BASE,IRRX_INT_STS,BL_SET_REG_BIT(tmpVal,IR_CR_IRRX_END_CLR));

	    return U16RxLen;
}
/******************************************************************************/
/******************************************************************************/
/***                            MainRun functions                            **/
/******************************************************************************/
/*******************************************************b***********************/

/******************************************************************************/
/******************************************************************************/
/***                            Public functions                             **/
/******************************************************************************/
/******************************************************************************/
static void IR_GPIO_Init(void)
{
    LOGA(IR, "GPIO init\r\n");
    GLB_GPIO_Type gpioPins[2] = {IR_PIN_TX,IR_PIN_RX};
    GLB_GPIO_FUNC_Type gpioFuns[2] = {GPIO_FUN_ANALOG,GPIO_FUN_SWGPIO};
    
    /* IR LED driver must use gpio11 */
    GLB_GPIO_Func_Init(gpioFuns[0],&gpioPins[0],1);
    /* IR RX gpio should run in SWGPIO mode */
    GLB_GPIO_Func_Init(gpioFuns[1],&gpioPins[1],1);
}


void NamiIR_Init(uint8_t DataBit)
{
    GLB_Set_IR_CLK(ENABLE, GLB_IR_CLK_SRC_XCLK, PARAMETER_IR_DIV_CLOCK); // Divider = 1

    /* IR gpio init */
    IR_GPIO_Init();
    
    /* Enable ir tx driver */
    GLB_IR_LED_Driver_Enable();
    
    /* Select ir rx gpio */
    GLB_IR_RX_GPIO_Sel(IR_PIN_RX);
    
    /* Disable ir before config */
    IR_Disable(IR_TXRX);

    if(DataBit != 0){
        txCfg.dataBits = DataBit;
        LOGA(IR, "Data bit current is %u \r\n", txCfg.dataBits);
    }


    /* IR tx init */
    IR_TxInit(&txCfg);
    IR_TxPulseWidthConfig(&txPWCfg);
    
    /* IR rx init */
    IR_RxInit(&rxCfg);
}


static uint16_t calculateBatchSize(uint16_t total) {
    if (total == 0) {
        return 0; // Avoid division by zero if total is 0
    }

    // Iterate from 32 down to 2 to find the largest suitable divisor
    for (int16_t i = 32; i >= 2; i--) {
        if (i % 2 == 0) { // Check if i is even
            if (total % i == 0) { // Check if i is a divisor of total
                return (uint16_t)i;
            }
        }
    }

    // If no even divisor <= 32 is found (e.g., total is 1 or an odd prime > 32)
    // return a default safe batch size.
    // For total=1, any batch size > 1 will work, but 2 is a safe default.
    // For odd totals, their divisors are odd, so this fallback will be hit.
    // We need an even batch size. If total is odd, we can't divide it evenly
    // into batches of an even size. The original code handles partial batches
    // with `currentBatchSize = (remainingItems >= batchSize) ? batchSize : remainingItems;`
    // So, returning a default even number is fine.
    if (total >= 2) {
        return 2; // Smallest even number <= 32
    } else {
        return 1; // If total is 1, batch size must be 1
    }
}

static BL_Err_Type NamiIR_TR(void){
    /* IR tx init */
    IR_TxInit(&txCfg);
    IR_TxPulseWidthConfig(&txPWCfg);

    uint32_t wdata = 0xE916FF00;
    uint32_t rdata = 0;
    
    /* Enable ir rx */
    // IR_Enable(IR_RX);

    /* TX send command */   
    IR_TxSWM(ENABLE);
    
    // Create input array by multiplying each item by 2
    uint16_t totalItems = NAMI_IR.NamiSystem.TempCount;
    uint16_t pulseData[totalItems]; // Create array with same size
    memset(&pulseData, 0x00, sizeof(uint16_t) * NAMI_IR.NamiSystem.TempCount);
    
    #define NUMBER_DETECT_ISOLATE_PULSE_LOW            5000
    #define NUMBER_DETECT_ISOLATE_PULSE_HIGH           6000
    uint16_t NumberMin = NAMI_IR.NamiSystem.TempPulse[0]/2;
    uint16_t NumberMax = NAMI_IR.NamiSystem.TempPulse[0]/2;
    uint16_t NumberIndex = 0;
   
    for(uint16_t Index = 0; Index < totalItems; Index++){
        if(NAMI_IR.NamiSystem.TempPulse[Index]/2 < NumberMin){
            NumberMin = NAMI_IR.NamiSystem.TempPulse[Index]/2;
            LOGA(IR, "number min is %u \r\n", NumberMin);
        }

        if(NAMI_IR.NamiSystem.TempPulse[Index]/2 > NumberMax){
            NumberMax = NAMI_IR.NamiSystem.TempPulse[Index]/2;
            NumberIndex = Index;
            LOGA(IR, "number max is %u \r\n", NumberMax);
        }
    }

    if(NAMI_IR.NamiSystem.TempPulse[NumberIndex] > NUMBER_DETECT_ISOLATE_PULSE_LOW*2 && NAMI_IR.NamiSystem.TempPulse[NumberIndex] <= NUMBER_DETECT_ISOLATE_PULSE_HIGH*2){
        NAMI_IR.NamiSystem.TempPulse[NumberIndex] = NAMI_IR.NamiSystem.TempPulse[NumberIndex] + NumberMin;
        LOGA(IR, "Number isolate pulse[%d]=%u \r\n", NumberIndex, NAMI_IR.NamiSystem.TempPulse[NumberIndex]);
    }

    memcpy(&pulseData, &NAMI_IR.NamiSystem.TempPulse, sizeof(uint16_t) * NAMI_IR.NamiSystem.TempCount);

    LOGA(IR, "pulseData[%u]=[", totalItems);
    if((BIT(IR) & FmDebug) == BIT(IR)){
        for(uint32_t Index = 0; Index < totalItems; Index++){
            printf("%u", pulseData[Index]);
            if(Index < totalItems - 1){
                printf(",");
            }
        }
        printf("\033[0;32m]\r\n"); 
    }
    
    uint16_t batchSize = NAMI_IR.NamiSystem.BestDivisor;
    uint16_t currentPos = 0;
    
    LOGA(IR, "Total items to send: %d\r\n", totalItems);
    LOGA(IR, "Batch size: %d\r\n", batchSize);
    
    /* Mask tx interrupt */
    // IR_IntMask(IR_INT_TX,MASK);
    IR_IntMask(IR_INT_TX,MASK);
        
    /* Clear tx interrupt */
    IR_ClrIntStatus(IR_INT_TX);
    
    
    // Send data in batches of 10 items
    while (currentPos < totalItems) {
        bl_gpio_output_set(GPIO_LED_NOTIFY, 0);
        // Calculate remaining items and current batch size
        uint16_t remainingItems = totalItems - currentPos;
        uint16_t currentBatchSize = (remainingItems >= batchSize) ? batchSize : remainingItems;
        LOGA(PULSE, "remainingItems=%u, currentPos=%u, currentBatchSize=%u, batchSize=%u \r\n", remainingItems, currentPos, currentBatchSize, batchSize);

        // Send current batch
        IR_SWMSendData(&pulseData[currentPos], currentBatchSize-1);
        
        /* Enable ir tx */
        IR_Enable(IR_TX);
        
        /* Wait for tx interrupt */
        while(SET != IR_GetIntStatus(IR_INT_TX)){
            // Wait for transmission to complete
        }

        /* Disable ir tx */
        IR_Disable(IR_TX);
        
        /* Clear tx interrupt */
        IR_ClrIntStatus(IR_INT_TX);
        
        // Move to next batch
        currentPos += currentBatchSize;
        LOGA(PULSE, "currentPos = %u, currentBatchSize = %u\r\n", currentPos, currentBatchSize);

        if (currentPos < totalItems) {
            if (!(currentPos % batchSize)){
                LOGA(PULSE, "currentPos div batchSize = %u,  pulseData[%u]=%u\r\n", currentPos % batchSize, currentPos-1, pulseData[currentPos-1]);
                if(pulseData[currentPos-1] >= NamiMQTT.RangeDelayBuffer[LEVEL_HIGH][POSA] && pulseData[currentPos-1] < NamiMQTT.RangeDelayBuffer[LEVEL_HIGH][POSB]){
                    LOGA(PULSE, "Delay 10K, pulseData[%u]=%u\r\n", currentPos-1, pulseData[currentPos-1]);
                    BL602_Delay_US(pulseData[currentPos-1]/2 - NumberMin);                    
                }else if(pulseData[currentPos-1] >= NamiMQTT.RangeDelayBuffer[LEVEL_LOW][POSA] && pulseData[currentPos-1] < NamiMQTT.RangeDelayBuffer[LEVEL_LOW][POSB]){
                    LOGA(PULSE, "Delay 1, pulseData[%u]=%u\r\n", currentPos-1, pulseData[currentPos-1]);
                    BL602_Delay_US(1);
                }else if(pulseData[currentPos-1] >= NamiMQTT.RangeDelayBuffer[LEVEL_MEDIUM][POSA] && pulseData[currentPos-1] < NamiMQTT.RangeDelayBuffer[LEVEL_MEDIUM][POSB]){
                    LOGA(PULSE, "Delay 2, pulseData[%u]=%u\r\n", currentPos-1, pulseData[currentPos-1]);
                    BL602_Delay_US(pulseData[currentPos-1]/2 - NumberMin);
                }
            }
        }
    }
    bl_gpio_output_set(GPIO_LED_NOTIFY, 1);
    /* Disable ir tx */
    IR_Disable(IR_TX);
    IR_ClrIntStatus(IR_INT_TX);

    return SUCCESS;
}

void NamiSuspendTask(void){
    // vTaskSuspend(LedTaskHandle);
    vTaskSuspend(RelayEventTaskHandle);
    vTaskSuspend(MQTTTaskHandle);
    vTaskSuspend(NamiIRTaskHandle);
    vTaskSuspend(wifi_fw_task_handle);
    xTimerStop(NamiSwitch.TimerSwitchHandler, 0);
}

void NamiResumeTask(void){
    // vTaskResume(LedTaskHandle);
    vTaskResume(RelayEventTaskHandle);
    vTaskResume(MQTTTaskHandle);
    vTaskResume(NamiIRTaskHandle);
    vTaskResume(wifi_fw_task_handle);
    xTimerStart(NamiSwitch.TimerSwitchHandler, 0);
}

void NamiInitRecv(void){
	memset(&NamiIRRecv, 0x00, sizeof(NamiIRRecv));
    NamiIRRecv.NMIR_Init           = &NamiInitRecv;
    NamiIRRecv.SuspendTask         = &NamiSuspendTask;
    NamiIRRecv.ResumeTask          = &NamiResumeTask;

	_NMIR_Init();

	uint32_t RecvCounter = 0;

	while (1) {
        #define HEXDUMP_COLS 8
        // if(RecvCounter++ >= 0xFFFFFFFE) RecvCounter = 0;
        if(NAMI_IR.NamiSystem.ReadySend == IR_SEND){
            if(NAMI_IR.NamiSystem.BestDivisor != 0){
                NamiIR_Init(NAMI_IR.NamiSystem.BestDivisor);
                NamiIR_TR();
            }else{
                LOGA(IR, "Err: index init of databit transmit is %u\r\n", NAMI_IR.NamiSystem.BestDivisor);
            }
            NAMI_IR.NamiSystem.ReadySend = IR_IDLE;
            NamiIRRecv.ResumeTask();
            if(gmqtt_connected){
                submit_gpio_relay_status_to_mqtt_server(SEND_OK);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200));  // 1-second delay
    }
	vTaskDelete(NULL);
}
/******************************************************************************/
/******************************************************************************/
/***                            Library callback                             **/
/******************************************************************************/
/******************************************************************************/




