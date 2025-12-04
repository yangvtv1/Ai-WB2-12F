/*
 *              Copyright (c)  2025
 *  File header: 		nami_ir.c
 *  Created on : 		April 09, 2025
 *      Author : 		R&D Firmware Team (Firmware Developer)
 *      Contact:        hai.luong@namismart.vn
 */


#include "nami_ir.h"

/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/

/******************************************************************************/
/******************************************************************************/
/***                      Private functions declare                          **/
/******************************************************************************/
/******************************************************************************/
// void NamiIRControlTask(IR_SEND_STATUS status);
/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/
nami_ir_t NAMI_IR;
TaskHandle_t NamiIRTaskHandle;
static nami_sys_t NamiSys;
hosal_timer_dev_t timer0;
BaseType_t xReturned;
TaskHandle_t xHandleTaskAlarm = NULL;
// static bool CheckstatusTaskFlag = false;
static uint32_t SendIRCount = 0;
uint16_t TmpCount=0;
static bool PrintIRFlag = false;
uint8_t Countidx = 0xFF;

uint16_t UpdateTempHumidCount;
hosal_i2c_dev_t i2c_dev = {
	.port = IR_I2C_PARA_ID, // Sử dụng I2C port 0
	.config = {
		.address_width = HOSAL_I2C_ADDRESS_WIDTH_7BIT,
		.freq = IR_I2C_PARA_SPEED,
		.scl = 4,  // GPIO14 cho SCL - HÃY KIỂM TRA LẠI CHÂN NÀY TRÊN BOARD CỦA BẠN
		.sda = 5,  // GPIO11 cho SDA - HÃY KIỂM TRA LẠI CHÂN NÀY TRÊN BOARD CỦA BẠN
		.mode = HOSAL_I2C_MODE_MASTER,
	}
};


// Khai báo các hàm
static int aht20_init(void);
static int aht20_read(float *temperature, float *humidity);


typedef enum{
	NEXT,
	EXTEND
}nextorext_e;

uint8_t NamiMergeIR(uint16_t *ir_data, uint16_t ir_data_len){
	LOGA(IR, "NamiMergeIR...\r\n");
	static bool EraserFirstFlag = false;
	static uint8_t CountFrame = 0;

	if(ir_data != NULL && ir_data_len > 0){
		if(EraserFirstFlag == false){ // Adjusted condition for Countidx
			EraserFirstFlag = true;
			memset(&NAMI_IR.NamiSystem.TempPulse, 0x00, sizeof(uint16_t) * sizeof(NAMI_IR.NamiSystem.TempPulse)/sizeof(NAMI_IR.NamiSystem.TempPulse[0])); // Corrected memset
			NAMI_IR.NamiSystem.TempCount = 0;
			NAMI_IR.NamiSystem.Temperature = ir_data[1];
			LOGA(IR, "Temperature=%d*C\r\n", NAMI_IR.NamiSystem.Temperature);
			CountFrame = 0;
		} else if (ir_data[0] == CHAIN_LATCH) {
			EraserFirstFlag = false;
		}

		if(ir_data[3] != TYPE_NAMI_U16){ // Adjusted condition for Countidx
			uint16_t data_to_copy = ir_data_len - 3;
			uint16_t current_index = NAMI_IR.NamiSystem.TempCount;
			LOGA(IR, "data_to_copy=%d, current_index=%d\r\n", data_to_copy, current_index);
			if (current_index + data_to_copy > MAXSIZE) {
				LOGA(IR, "Warning: Not enough space in TempPulse[%lu]. Skipping some data.\r\n", (unsigned long)Countidx);
				data_to_copy = MAXSIZE - current_index;
			}

			if (data_to_copy > 0) {
				CountFrame++;
				memcpy(&NAMI_IR.NamiSystem.TempPulse[current_index], &ir_data[3], data_to_copy * sizeof(uint16_t));
				NAMI_IR.NamiSystem.TempCount += data_to_copy;
			}
		}

		if(ir_data[0] == CHAIN_LATCH){
			LOGA(IR, "Temperate=%u, TempPulse[%u]=[", NAMI_IR.NamiSystem.Temperature, NAMI_IR.NamiSystem.TempCount);
			if((BIT(IR) & FmDebug) == BIT(IR)){
				for(uint32_t Index = 0; Index < NAMI_IR.NamiSystem.TempCount; Index++){
					printf("%u", NAMI_IR.NamiSystem.TempPulse[Index]);
					if(Index < NAMI_IR.NamiSystem.TempCount - 1){
						printf(",");
					}
				}
				printf("\033[0;32m]\r\n");
			}

			// calcualate bit to init IR send
			NAMI_IR.NamiSystem.TotalLengthPlus = NAMI_IR.NamiSystem.TempCount + 1;
			LOGA(IR, "Total length of array: %d\r\n", NAMI_IR.NamiSystem.TempCount);
			LOGA(IR, "Total length + 1: %d\r\n", NAMI_IR.NamiSystem.TotalLengthPlus);
			NAMI_IR.NamiSystem.BestDivisor = -1;
			for (int i = 30; i >= 2; i -= 2) { // Kiểm tra từ 30 xuống 2 (bước 2)
				if (NAMI_IR.NamiSystem.TotalLengthPlus % i == 0) {
					NAMI_IR.NamiSystem.BestDivisor = i;
					break;
				}
			}

			if (NAMI_IR.NamiSystem.BestDivisor != -1) {
				LOGA(IR, "Best even divisor less than 32: %d\r\n", NAMI_IR.NamiSystem.BestDivisor);
				LOGA(IR, "Result: (%d + 1) / %d = %d (exact division)\r\n", 
					NAMI_IR.NamiSystem.TempCount, NAMI_IR.NamiSystem.BestDivisor, NAMI_IR.NamiSystem.TotalLengthPlus / NAMI_IR.NamiSystem.BestDivisor);
			} else {
				NAMI_IR.NamiSystem.BestDivisor = 0;
				LOGA(IR, "No even divisor less than 32 found.\r\n");
				submit_gpio_relay_status_to_mqtt_server(NO_BEST_DIV);
			}	
		}
		return CountFrame;
	}else{
		return 101;
	}
}


 
void NamiIRControlSend(int status){
	if(status == 0xFE){
        LOGA(IR, "Reset Firmware after 3s...\r\n");
		vTaskDelay(pdMS_TO_TICKS(3000));
		NAMI_SYSTEM_RESET();		
	}else{
		NAMI_IR.NamiSystem.ReadySend = IR_SEND;
	}
}

static int aht20_init(void)
{
    uint8_t cmd[3];
    uint8_t status;
    
    // Soft reset
    cmd[0] = IR_I2C_PARA_RESET;
    if (IR_I2C_MASTER_SEND(&i2c_dev, IR_I2C_PARA_ADDRESS, cmd, 1, IR_I2C_PARA_TIMEOUT) != 0) {
        return -1;
    }
    vTaskDelay(20 / portTICK_PERIOD_MS);
    
    // Kiểm tra trạng thái
    cmd[0] = IR_I2C_PARA_INTI;
    if (IR_I2C_MASTER_SEND(&i2c_dev, IR_I2C_PARA_ADDRESS, cmd, 1, IR_I2C_PARA_TIMEOUT) != 0) {
        return -1;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
    
    // Đọc trạng thái
    if (IR_I2C_MASTER_RECV(&i2c_dev, IR_I2C_PARA_ADDRESS, &status, 1, IR_I2C_PARA_TIMEOUT) != 0) {
        return -1;
    }
    
    // Kiểm tra calibration (Bit3 và Bit4 phải là 1)
    if ((status & 0x18) != 0x18) {
        // Calibrate - Gửi lệnh calibration 0xBE 0x08 0x00
        cmd[0] = 0xBE;
        cmd[1] = 0x08;
        cmd[2] = 0x00;
        if (IR_I2C_MASTER_SEND(&i2c_dev, IR_I2C_PARA_ADDRESS, cmd, 3, IR_I2C_PARA_TIMEOUT) != 0) {
            return -1;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS); // Chờ calibration hoàn tất
    }
    
    return 0;
}

// Đọc dữ liệu từ AHT20
static int aht20_read(float *temperature, float *humidity)
{
    uint8_t cmd[3];
    uint8_t data[6];
    
    // Gửi lệnh trigger measurement (0xAC 0x33 0x00)
    cmd[0] = IR_I2C_PARA_TRIGGER;
    cmd[1] = 0x33;
    cmd[2] = 0x00;
    
    if (IR_I2C_MASTER_SEND(&i2c_dev, IR_I2C_PARA_ADDRESS, cmd, 3, IR_I2C_PARA_TIMEOUT) != 0) {
        return -1;
    }
    
    // Chờ đo xong (ít nhất 80ms)
    vTaskDelay(80 / portTICK_PERIOD_MS);
    
    // Đọc 6 byte dữ liệu
    if (IR_I2C_MASTER_RECV(&i2c_dev, IR_I2C_PARA_ADDRESS, data, 6, IR_I2C_PARA_TIMEOUT) != 0) {
        return -1;
    }
    
    // Kiểm tra bit busy (bit7 của byte đầu tiên)
    if (data[0] & 0x80) {
        return -1; // Cảm biến vẫn đang bận
    }
    
    // Chuyển đổi dữ liệu thô thành giá trị thực
    uint32_t raw_humi = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
    uint32_t raw_temp = ((uint32_t)(data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];
    
    *humidity = (raw_humi * 100.0f) / (1 << 20); // 2^20 = 1048576
    *temperature = (raw_temp * 200.0f) / (1 << 20) - 50.0f;
    
    return 0;
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
void InitNamiIR(void *pvParameters){
	memset(&NAMI_IR, 0x00, sizeof(NAMI_IR));
	NAMI_IR.ActiveFlag                    = true;
	NAMI_IR.init                          = &InitNamiIR;
	NAMI_IR.UpdateFWFlag                  = &NamiMQTT.UpdateFirmwareFlag;

	static bool FirstFlag = false;
	static bool CheckVersionFlag = false;
	static uint32_t NumOld = 0;
	static bool SendFirstFlag = true;
	static bool SendFirstIRFlag = true;
	static bool SendCompletedFlag = false;
	static bool SendMacAddRstFlag = true;
	static uint32_t TimerLedNoInternetCount = 0;
    static bool ChangeStateFlag = false;
	static bool CheckStateLedFlag = false;

	// Khởi tạo I2C
    hosal_i2c_init(&i2c_dev);
    
    // Khởi tạo AHT20
    if (aht20_init() != 0) {
        LOGA(IR, "AHT20 init failed!\r\n");
        return;
    }

	while(1){
        if((gmqtt_connected != true) && (NamiSwitch.ModeCalibFlag == false)){
			if(TimerLedNoInternetCount++ >= 2){
				if(ChangeStateFlag){
					// LOGA(IR, "ON LED \r\n");
					bl_gpio_output_set(GPIO_LED_NOTIFY, LED_ON);
					ChangeStateFlag = false;
					TimerLedNoInternetCount = 0;
					CheckStateLedFlag = true;
				}else if(ChangeStateFlag == false){
					// LOGA(IR, "OFF LED \r\n");
					bl_gpio_output_set(GPIO_LED_NOTIFY, LED_OFF);
					ChangeStateFlag = true;
					TimerLedNoInternetCount = 0;
					CheckStateLedFlag = true;
				}
			}       
        }

		if(CheckStateLedFlag && gmqtt_connected){
			CheckStateLedFlag = false;
			bl_gpio_output_set(GPIO_LED_NOTIFY, LED_OFF);
		}

		if(UpdateTempHumidCount++ >= 100 && gmqtt_connected){
			if (aht20_read(&NAMI_IR.temperature, &NAMI_IR.humidity) == 0) {
				UpdateTempHumidCount= 0;
				submit_gpio_relay_status_to_mqtt_server(TEMPHUMID);
            	LOGA(IR, "Temperature: %.0f °C, Humidity: %.0f %%\r\n", NAMI_IR.temperature, NAMI_IR.humidity);
        	}
		}


		if(NamiMQTT.CalibNumberGetFlag){
			NamiMQTT.CalibNumberGetFlag = false;
			char tempnumbuf[20] = "";
			for(uint8_t NumIdx = 0; NumIdx < 3; NumIdx++){
				sprintf(tempnumbuf, "combo_number_0.%d", NumIdx);
				ef_get_u16(tempnumbuf, &NamiMQTT.NumCalib[0][NumIdx]);
			}
			memset(&tempnumbuf, 0x00, 20);
			for(uint8_t NumIdx = 0; NumIdx < 3; NumIdx++){
				sprintf(tempnumbuf, "combo_number_1.%d", NumIdx);
				ef_get_u16(tempnumbuf, &NamiMQTT.NumCalib[1][NumIdx]);
			}
			LOGA(NAMI, "(LOW:%u, HIGH:%u) Value:%u, (LOW:%u, HIGH:%u) value:%u\r\n", NamiMQTT.NumCalib[0][0], NamiMQTT.NumCalib[0][1], NamiMQTT.NumCalib[0][2]
																		           , NamiMQTT.NumCalib[1][0], NamiMQTT.NumCalib[1][1], NamiMQTT.NumCalib[1][2]);
			submit_gpio_relay_status_to_mqtt_server(NUMBER_CALIB_COMPARE);
		}else if(NamiMQTT.ReSendCalibNumberGetFlag && gmqtt_connected){
			NamiMQTT.ReSendCalibNumberGetFlag = false;
			char tempnumbuf[20] = "";
			for(uint8_t NumIdx = 0; NumIdx < 3; NumIdx++){
				sprintf(tempnumbuf, "combo_number_0.%d", NumIdx);
				ef_get_u16(tempnumbuf, &NamiMQTT.NumCalib[0][NumIdx]);
			}
			memset(&tempnumbuf, 0x00, 20);
			for(uint8_t NumIdx = 0; NumIdx < 3; NumIdx++){
				sprintf(tempnumbuf, "combo_number_1.%d", NumIdx);
				ef_get_u16(tempnumbuf, &NamiMQTT.NumCalib[1][NumIdx]);
			}
			LOGA(NAMI, "(LOW:%u, HIGH:%u) Value:%u, (LOW:%u, HIGH:%u) value:%u\r\n", NamiMQTT.NumCalib[0][0], NamiMQTT.NumCalib[0][1], NamiMQTT.NumCalib[0][2]
																		           , NamiMQTT.NumCalib[1][0], NamiMQTT.NumCalib[1][1], NamiMQTT.NumCalib[1][2]);
			submit_gpio_relay_status_to_mqtt_server(NUMBER_CALIB_COMPARE);			
		}

		if(NamiMQTT.CalibNumberAFlag){
			NamiMQTT.CalibNumberAFlag = false;
			LOGA(NAMI, "(LOW:%u, HIGH:%u) Value:%u, (LOW:%u, HIGH:%u) value:%u\r\n", NamiMQTT.NumCalib[0][0], NamiMQTT.NumCalib[0][1], NamiMQTT.NumCalib[0][2]
																		           , NamiMQTT.NumCalib[1][0], NamiMQTT.NumCalib[1][1], NamiMQTT.NumCalib[1][2]);
			
			char tempnumbuf[20] = "";
			for(uint8_t NumIdx = 0; NumIdx < 3; NumIdx++){
				sprintf(tempnumbuf, "combo_number_0.%d", NumIdx);
				ef_set_u16(tempnumbuf, NamiMQTT.NumCalib[0][NumIdx]);
			}
		}else if(NamiMQTT.CalibNumberBFlag){
			NamiMQTT.CalibNumberBFlag = false;
			LOGA(NAMI, "(LOW:%u, HIGH:%u) Value:%u, (LOW:%u, HIGH:%u) value:%u\r\n", NamiMQTT.NumCalib[0][0], NamiMQTT.NumCalib[0][1], NamiMQTT.NumCalib[0][2]
																		           , NamiMQTT.NumCalib[1][0], NamiMQTT.NumCalib[1][1], NamiMQTT.NumCalib[1][2]);
			
			char tempnumbuf[20] = "";
			for(uint8_t NumIdx = 0; NumIdx < 3; NumIdx++){
				sprintf(tempnumbuf, "combo_number_1.%d", NumIdx);
				ef_set_u16(tempnumbuf, NamiMQTT.NumCalib[1][NumIdx]);
			}
		}

		if(SendMacAddRstFlag && gmqtt_connected){
			SendMacAddRstFlag = false;
			submit_gpio_relay_status_to_mqtt_server(MAC_ADD);
		}

		if(SendFirstFlag){
			SendFirstFlag = false;
			uint16_t TempIR[] = {4372,4348,516,1456,492, 492,492,1480,512,1460,512};
			memcpy(&NAMI_IR.NamiSystem.TempPulse, &TempIR, sizeof(uint16_t) * sizeof(TempIR)/sizeof(TempIR[0]));
			NAMI_IR.NamiSystem.TempCount += sizeof(TempIR)/sizeof(TempIR[0]);

			NAMI_IR.NamiSystem.TotalLengthPlus = NAMI_IR.NamiSystem.TempCount + 1;
			LOGA(IR, "Total length of array: %d\r\n", NAMI_IR.NamiSystem.TempCount);
			LOGA(IR, "Total length + 1: %d\r\n", NAMI_IR.NamiSystem.TotalLengthPlus);
			NAMI_IR.NamiSystem.BestDivisor = -1;
			for (int i = 30; i >= 2; i -= 2) { // Kiểm tra từ 30 xuống 2 (bước 2)
				if (NAMI_IR.NamiSystem.TotalLengthPlus % i == 0) {
					NAMI_IR.NamiSystem.BestDivisor = i;
					break;
				}
			}

			if (NAMI_IR.NamiSystem.BestDivisor != -1) {
				SendFirstIRFlag = true;
				LOGA(IR, "Best even divisor less than 32: %d\r\n", NAMI_IR.NamiSystem.BestDivisor);
				LOGA(IR, "Result: (%d + 1) / %d = %d (exact division)\r\n", 
					NAMI_IR.NamiSystem.TempCount, NAMI_IR.NamiSystem.BestDivisor, NAMI_IR.NamiSystem.TotalLengthPlus / NAMI_IR.NamiSystem.BestDivisor);
			} else {
				NAMI_IR.NamiSystem.BestDivisor = 0;
				LOGA(IR, "No even divisor less than 32 found.\r\n");
				submit_gpio_relay_status_to_mqtt_server(NO_BEST_DIV);
			}
		}

		if(SendFirstIRFlag){
			SendFirstIRFlag = false;
			NAMI_IR.NamiSystem.ReadySend = IR_SEND;
		}

		if (gmqtt_connected && CheckVersionFlag == false){
			if(NamiMQTT.StateUDFFlag == 50){
				ef_set_u8("StateUDF", 100);
				SendCompletedFlag = true;
				submit_gpio_relay_status_to_mqtt_server(NEWVER);
				vTaskDelay(pdMS_TO_TICKS(100));
				memset(&NamiMQTT.CheckVer, 0x00, sizeof(NamiMQTT.CheckVer));
				memcpy(&NamiMQTT.CheckVer[0], FIRMWAREVERSION, strlen(FIRMWAREVERSION));
				ef_set_str("version", NamiMQTT.CheckVer);
				LOGA(IR, "save flash version = %s\r\n", NamiMQTT.CheckVer);	
				vTaskDelay(pdMS_TO_TICKS(100));
				submit_gpio_relay_status_to_mqtt_server(UDFSUCCESS);
			}else{
				memset(&NamiMQTT.CheckVer, 0x00, sizeof(NamiMQTT.CheckVer));
				memcpy(&NamiMQTT.CheckVer[0], "1.0.0-beta", strlen("1.0.0-beta"));
				ef_set_str("version", NamiMQTT.CheckVer);
				submit_gpio_relay_status_to_mqtt_server(SETVER);
				LOGA(IR, "Err: syntax error  (%s)\r\n", NamiMQTT.CheckVer);	
			}
			CheckVersionFlag = true;
		}

		if(SendCompletedFlag){
			SendCompletedFlag = false;
			LOGA(IR, "Update completed\r\n");	
			vTaskDelay(pdMS_TO_TICKS(100));
			submit_gpio_relay_status_to_mqtt_server(UDFCOMPLETE);
		}

		if(OtaUDF.UpdatePercent){
			if(OtaUDF.UpdatePercent != NumOld && OtaUDF.UpdatePercent){
				NumOld = OtaUDF.UpdatePercent;
				submit_gpio_relay_status_to_mqtt_server(OtaUDF.UpdatePercent);
			}
		}

		if(*NAMI_IR.UpdateFWFlag == NAMI_ENABLE){
			LOGA(IR, "Test update firmware, status(%d)\r\n", *NAMI_IR.UpdateFWFlag);
			*NAMI_IR.UpdateFWFlag = 0;
			NAMI_IR.UpdateLogFlag = NAMI_DISABLE;
			vTaskDelay(pdMS_TO_TICKS(200));  
			if(NAMI_IR.NamiSystem.EasyFlashBuff != NULL){
				FirstFlag = true;
				ef_set_u8("StateUDF", 50);
				vTaskDelay(pdMS_TO_TICKS(100)); 
				LOGA(IR, "Test update firmware, Len(%d)\r\n", strlen((char *)&NAMI_IR.NamiSystem.EasyFlashBuff[0]));
				LOGA(IR, "Data(%s)\r\n", NAMI_IR.NamiSystem.EasyFlashBuff);
				axk_hal_handle_ota_json((char *)&NAMI_IR.NamiSystem.EasyFlashBuff[0]);
				memset(&NAMI_IR.NamiSystem.EasyFlashBuff, 0x00, sizeof(NAMI_IR.NamiSystem.EasyFlashBuff));
			}else{
				LOGA(IR, "Error: buffer NO index fail \r\n");
			}
		}
		vTaskDelay(pdMS_TO_TICKS(1*100));
	}
	vTaskDelete(NULL);
	// xReturned = xTaskCreate(NamiIRCallback, "NamiIRCallback", 1024, NULL, 14, &xHandleTaskAlarm);
}

#ifdef DETTECT_IR_PULSE
void NamiListen(void *pvParameters){
	while(1){
		if(gmqtt_connected /*&& NamiMQTT.StateLearnFlag*/){
			// NamiMQTT.StateLearnFlag = false;

			uint32_t Count;
			uint16_t CountString;
			// {"ir" :[4, 0, 0, 0]}

			Count = MQTTIRLearn(ELISTEN);
			if(Count >= GENSTRINGSIZE){
				LOGA(IR, "Error: over size (%d) of buffer fail \r\n", Count);
				// memset(&NamiMQTT.LearnBuffer, 0x0, sizeof(NamiMQTT.LearnBuffer));
				NamiMQTT.LearnCount = 0;
				return;
			}else{
				// LOGA(NAMI, "Size (%d)/(%d) of buffer = [", Count, Count/2);
				// for(uint32_t CopyIndex = 0; CopyIndex < Count; CopyIndex++){
				//     printf("%04d", NamiMQTT.GenBuffer[CopyIndex]);
				//     if(CopyIndex < Count - 1){
				//         printf(", ");
				//     }
				// }
				// printf("]\r\n");
			}

			LOGA(IR, "(nammi_ir.c) - Size (%d) of string\r\n", NamiMQTT.GenCounter);


			NamiMQTTConvertUintToString(NamiMQTT.GenBuffer, NamiMQTT.GenCounter);
		
			// LearnRspFlag = true; 
			// submit_gpio_relay_status_to_mqtt_server(SEND_LEAR);
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	vTaskDelete(NULL);
}
#endif
/******************************************************************************/
/******************************************************************************/
/***                            Library callback                             **/
/******************************************************************************/
/******************************************************************************/




