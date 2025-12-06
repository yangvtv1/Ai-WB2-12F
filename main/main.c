 #include "main.h"


general_t Gen;


volatile RetentionData *retention_data = (volatile RetentionData *)RETENTION_RAM_BASE;
static bool CmdFrame1Flag = false;
static bool CmdFrame2Flag = false;
static bool CmdFrame3Flag = false;
static bool CmdFrame4Flag = false;

static bool CmdFrameMotion1Flag = false;
static bool CmdFrameMotion2Flag = false;
static bool CmdFrameMotion3Flag = false;
static bool CmdFrameMotion4Flag = false;

static bool CmdOpenSettingFlag = false;
static bool PubCmdOpenSettingFlag = false;
static bool CmndCloseSettingFlag = false;
static bool PubCmdCloseSettingFlag = false;
static bool CmdOpenQueryProgressFlag = false;

static bool CmdSetAutoThreHoldFlag = false;

static uint8_t Detect3Status = 0;
static bool UpdateStateMotionFlag = false;
static bool UpdateStateNoDetectFlag = false;
static bool OnlyUpdateFirmwareFlag  = false;




hosal_uart_dev_t UART_ID_1 = {
    .config = {
        .uart_id = 1,
        .tx_pin = 4, // TXD GPIO
        .rx_pin = 3, // RXD GPIO
        .cts_pin = 255,
        .rts_pin = 255,
        .baud_rate = 115200,
        .data_width = NAMI_PRE_OPERATE_DATA_WIDTH(NAMI_PRE_DATA_WIDTH_8BIT),
        .parity = NAMI_PRE_OPERATE_PARITY(NAMI_PRE_NO_PARITY),
        .stop_bits = NAMI_PRE_OPERATE_STP_BITS(NAMI_PRE_STOP_BITS_1),
        .mode = NAMI_PRE_OPERATE_UART_MODE(NAMI_PRE_UART_MODE_POLL),
    },
};

hosal_uart_dev_t uart_dev_echo = {
    .config = {
        .uart_id = 0,
        .tx_pin = 16, // TXD GPIO
        .rx_pin = 7,  // RXD GPIO
        .cts_pin = 255,
        .rts_pin = 255,
        .baud_rate = 115200,
        .data_width = NAMI_PRE_OPERATE_DATA_WIDTH(NAMI_PRE_DATA_WIDTH_8BIT),
        .parity = NAMI_PRE_OPERATE_PARITY(NAMI_PRE_NO_PARITY),
        .stop_bits = NAMI_PRE_OPERATE_STP_BITS(NAMI_PRE_STOP_BITS_1),
        .mode = NAMI_PRE_OPERATE_UART_MODE(NAMI_PRE_UART_MODE_POLL),
    },
};

static hosal_adc_dev_t adc0 = {
    .cb = NULL,
    .config = {
        .mode = HOSAL_ADC_ONE_SHOT,
        .pin = GPIO_ADC_PIN,
        .sampling_freq = 340,
    },
    .dma_chan = 0,
    .p_arg = NULL,
    .port = 0,
};


typedef struct {
    TickType_t start_time;    // Start time of current level
    TickType_t end_time;      // End time of previous level
    TickType_t duration;      // Duration of previous level (ms)
    uint8_t level;            // Signal level (0 or 1)
    uint32_t count;           // Counter for continuous printing
} SignalState_t;

static SignalState_t signal_state = {0};

void print_signal_state(uint8_t new_level, TickType_t current_time)
{
    // Always update the current level duration
    TickType_t current_duration = current_time - signal_state.start_time;
    
    if (new_level != signal_state.level) {
        // Level changed - print the previous level summary
        signal_state.end_time = current_time;
        signal_state.duration = signal_state.end_time - signal_state.start_time;
        
        // Print with different colors for different levels
        if (signal_state.level == 1) {
            LOGA(NAMI, "\033[0;36mLevel %d: Start=%lums, End=%lums, Duration=%lums - Motion Detected\033[0m\r\n",
                 signal_state.level,
                 signal_state.start_time,
                 signal_state.end_time,
                 signal_state.duration);
        } else {
            LOGA(NAMI, "\033[0;33mLevel %d: Start=%lums, End=%lums, Duration=%lums - No Motion\033[0m\r\n",
                 signal_state.level,
                 signal_state.start_time,
                 signal_state.end_time,
                 signal_state.duration);
        }
        
        // Reset counter for new level
        signal_state.count = 0;
        
        // Update to new level
        signal_state.level = new_level;
        signal_state.start_time = current_time;
    } else {
        // Same level - print continuous status
        signal_state.count++;
        
        // Print every 10 cycles (500ms) to avoid flooding the console
        if (signal_state.count % 10 == 0) {
            if (signal_state.level == 1) {
                LOGA(NAMI, "\033[0;36mCurrent: Level %d | Time active=%lums | Counter=%lu | Motion Detected\033[0m\r\n",
                     signal_state.level,
                     current_duration,
                     signal_state.count);
            } else {
                LOGA(NAMI, "\033[0;33mCurrent: Level %d | Time active=%lums | Counter=%lu | No Motion\033[0m\r\n",
                     signal_state.level,
                     current_duration,
                     signal_state.count);
            }
        }
    }
}

void ProcPresenceMQTT2way(uint8_t *_array, uint16_t Len){
    memcpy(&Gen.PresenBuf, &_array[1], (Len - 1));
    Gen.PresenCnt = Len - 1;
    LOGA(NAMI, "Command getting={");
    if((BIT(NAMI) & FmDebug) == BIT(NAMI)){
        for(uint8_t ParaIndex = 0; ParaIndex < Gen.PresenCnt; ParaIndex++){
            printf("%02X ", Gen.PresenBuf[ParaIndex]);
        }
        printf("}\r\n");
    }

    if(Gen.PresenBuf[0] != NULL && Gen.PresenBuf[1] != NULL){
        if(Gen.PresenBuf[0] == 0xFD && Gen.PresenBuf[1] == 0xFC && Gen.PresenBuf[2] == 0xFB && Gen.PresenBuf[3] == 0xFA){
            if(Gen.PresenBuf[14] == 16 && Gen.PresenBuf[6] == 0x08){
                CmdFrame1Flag = true;
                LOGA(NAMI, "enable status CmdFrame1Flag = %d\r\n", CmdFrame1Flag);
            }else if(Gen.PresenBuf[8] == 18 && Gen.PresenBuf[6] == 0x08){
                CmdFrame2Flag = true;
                LOGA(NAMI, "enable status CmdFrame2Flag = %d\r\n", CmdFrame2Flag);
            }else if(Gen.PresenBuf[8] == 23 && Gen.PresenBuf[6] == 0x08){
                CmdFrame3Flag = true;
                LOGA(NAMI, "enable status CmdFrame3Flag = %d\r\n", CmdFrame3Flag);
            }else if(Gen.PresenBuf[8] == 28 && Gen.PresenBuf[6] == 0x08){
                CmdFrame4Flag = true;
                LOGA(NAMI, "enable status CmdFrame4Flag = %d\r\n", CmdFrame4Flag);
            }else if(Gen.PresenBuf[8] == 0x30 && Gen.PresenBuf[6] == 0x08){
                CmdFrameMotion1Flag = true;
                LOGA(NAMI, "enable status CmdFrameMotion1Flag = %d\r\n", CmdFrameMotion1Flag);
            }else if(Gen.PresenBuf[8] == 0x35 && Gen.PresenBuf[6] == 0x08){
                CmdFrameMotion2Flag = true;
                LOGA(NAMI, "enable status CmdFrameMotion2Flag = %d\r\n", CmdFrameMotion2Flag);
            }else if(Gen.PresenBuf[8] == 0x3A && Gen.PresenBuf[6] == 0x08){
                CmdFrameMotion3Flag = true;
                LOGA(NAMI, "enable status CmdFrameMotion3Flag = %d\r\n", CmdFrameMotion3Flag);
            }else if(Gen.PresenBuf[8] == 0x3F && Gen.PresenBuf[6] == 0x08){
                CmdFrameMotion4Flag = true;
                LOGA(NAMI, "enable status CmdFrameMotion4Flag = %d\r\n", CmdFrameMotion4Flag);
            }else if(Gen.PresenBuf[6] == 255){
                CmdOpenSettingFlag = true;
                PubCmdOpenSettingFlag = true;
                CmndCloseSettingFlag = false;
            }else if(Gen.PresenBuf[6] == 254){
                CmndCloseSettingFlag = true;
                PubCmdCloseSettingFlag = true;
                CmdOpenSettingFlag = false;
            }else if(Gen.PresenBuf[6] == 0x0A){
                CmdOpenQueryProgressFlag = true;
            }else if(Gen.PresenBuf[6] == 0x09){
                CmdSetAutoThreHoldFlag = true;
                Gen.UartMotionTrigger       = (Gen.PresenBuf[9]<<8)|(Gen.PresenBuf[8]);
                Gen.UartMotionHoldThreshold = (Gen.PresenBuf[11]<<8)|(Gen.PresenBuf[10]);
                Gen.UartMicroMotionHoldThreshold = (Gen.PresenBuf[13]<<8)|(Gen.PresenBuf[12]);
            }
            hosal_uart_send(&UART_ID_1, Gen.PresenBuf, Gen.PresenCnt);
        }
    }
    memset(&Gen.PresenBuf, 0x00, sizeof(uint16_t)*(sizeof(Gen.PresenBuf)/sizeof(Gen.PresenBuf[0])));
    Gen.PresenCnt = 0;    
}

/*

static float extract_and_compute_log(uint32_t offset) {
    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[offset + 3] << 24) | 
                            (Gen.UartReceiveBuffer[offset + 2] << 16) | 
                            (Gen.UartReceiveBuffer[offset + 1] << 8) | 
                            Gen.UartReceiveBuffer[offset]);
    Gen.Log10Result = log10f(Gen.TotalCalib);
    return roundf(Gen.Log10Result * 1000.0f) / 100.0f;
}


static void process_frame_values(uint8_t start_index, const uint32_t offsets[], uint8_t num_values) {
    for (uint8_t i = 0; i < num_values; i++) {
        Gen.RawSettingFactorOfficial[start_index + i] = extract_and_compute_log(offsets[i]);
    }
}


} else if(Gen.UartReceiveBuffer[0] == 0xFD && Gen.UartReceiveBuffer[1] == 0xFC && Gen.UartReceiveBuffer[2] == 0xFB && Gen.UartReceiveBuffer[3] == 0xFA){
    LOGA(NAMI, "enable status CmdFrameMotion1Flag = %d\r\n", CmdFrameMotion1Flag);
    
    if(CmdFrame1Flag){
        CmdFrame1Flag = false;
        
        uint32_t offsets[] = {22, 26}; // Offsets for two values
        process_frame_values(0, offsets, 2);
        
        LOGA(NAMI, "Gen.TotalCalib=%f, Logarit=%f, F[0]=%.2f, F[1]=%.2f\r\n", 
            Gen.TotalCalib, Gen.Log10Result, Gen.RawSettingFactorOfficial[0], Gen.RawSettingFactorOfficial[1]);
        submit_gpio_relay_status_to_mqtt_server(PRESENCE_TRIGGER_FACTOR_1);
        
    } else if(CmdFrame2Flag){
        CmdFrame2Flag = false;
        
        uint32_t offsets[] = {10, 14, 18, 22, 26}; // Offsets for five values
        process_frame_values(2, offsets, 5);
        
        LOGA(NAMI, "Gen.TotalCalib=%f, Logarit=%f, F[2]=%.2f, F[3]=%.2f, F[4]=%.2f, F[5]=%.2f, F[6]=%.2f\r\n", 
            Gen.TotalCalib, Gen.Log10Result, Gen.RawSettingFactorOfficial[2], Gen.RawSettingFactorOfficial[3], 
            Gen.RawSettingFactorOfficial[4], Gen.RawSettingFactorOfficial[5], Gen.RawSettingFactorOfficial[6]);
        submit_gpio_relay_status_to_mqtt_server(PRESENCE_TRIGGER_FACTOR_2);
        
    } else if(CmdFrame3Flag){
        CmdFrame3Flag = false;
        
        uint32_t offsets[] = {10, 14, 18, 22, 26}; // Offsets for five values
        process_frame_values(7, offsets, 5);
        
        LOGA(NAMI, "Gen.TotalCalib=%f, Logarit=%f, F[7]=%.2f, F[8]=%.2f, F[9]=%.2f, F[10]=%.2f, F[11]=%.2f\r\n", 
            Gen.TotalCalib, Gen.Log10Result, Gen.RawSettingFactorOfficial[7], Gen.RawSettingFactorOfficial[8], 
            Gen.RawSettingFactorOfficial[9], Gen.RawSettingFactorOfficial[10], Gen.RawSettingFactorOfficial[11]);
        submit_gpio_relay_status_to_mqtt_server(PRESENCE_TRIGGER_FACTOR_3);
        
    } else if(CmdFrame4Flag){
        CmdFrame4Flag = false;
        
        uint32_t offsets[] = {10, 14, 18, 22, 26}; // Offsets for five values
        process_frame_values(12, offsets, 5);
        
        LOGA(NAMI, "Gen.TotalCalib=%f, Logarit=%f, F[12]=%.2f, F[13]=%.2f, F[14]=%.2f, F[15]=%.2f, F[16]=%.2f\r\n", 
            Gen.TotalCalib, Gen.Log10Result, Gen.RawSettingFactorOfficial[12], Gen.RawSettingFactorOfficial[13], 
            Gen.RawSettingFactorOfficial[14], Gen.RawSettingFactorOfficial[15], Gen.RawSettingFactorOfficial[16]);
        submit_gpio_relay_status_to_mqtt_server(PRESENCE_TRIGGER_FACTOR_4);
        
    } else if(CmdFrameMotion1Flag){
        CmdFrameMotion1Flag = false;
        
        uint32_t offsets[] = {10, 14, 18, 22, 26}; // Offsets for five values
        process_frame_values(0, offsets, 5);
        
        LOGA(NAMI, "F[0]=%.2f, F[1]=%.2f, F[2]=%.2f, F[3]=%.2f, F[4]=%.2f\r\n", 
            Gen.RawSettingFactorOfficial[0], Gen.RawSettingFactorOfficial[1], Gen.RawSettingFactorOfficial[2], 
            Gen.RawSettingFactorOfficial[3], Gen.RawSettingFactorOfficial[4]);
        submit_gpio_relay_status_to_mqtt_server(PRESENCE_MOTION_FACTOR_1);
        
    } else if(CmdFrameMotion2Flag){
        CmdFrameMotion2Flag = false;
        
        uint32_t offsets[] = {10, 14, 18, 22, 26}; // Offsets for five values
        process_frame_values(5, offsets, 5);
        
        LOGA(NAMI, "F[5]=%.2f, F[6]=%.2f, F[7]=%.2f, F[8]=%.2f, F[9]=%.2f\r\n", 
            Gen.RawSettingFactorOfficial[5], Gen.RawSettingFactorOfficial[6], Gen.RawSettingFactorOfficial[7], 
            Gen.RawSettingFactorOfficial[8], Gen.RawSettingFactorOfficial[9]);
        submit_gpio_relay_status_to_mqtt_server(PRESENCE_MOTION_FACTOR_2);
        
    } else if(CmdFrameMotion3Flag){
        CmdFrameMotion3Flag = false;
        
        uint32_t offsets[] = {10, 14, 18, 22, 26}; // Offsets for five values
        process_frame_values(10, offsets, 5);
        
        LOGA(NAMI, "F[10]=%.2f, F[11]=%.2f, F[12]=%.2f, F[13]=%.2f, F[14]=%.2f\r\n", 
            Gen.RawSettingFactorOfficial[10], Gen.RawSettingFactorOfficial[11], Gen.RawSettingFactorOfficial[12], 
            Gen.RawSettingFactorOfficial[13], Gen.RawSettingFactorOfficial[14]);
        submit_gpio_relay_status_to_mqtt_server(PRESENCE_MOTION_FACTOR_3);
        
    } else if(CmdFrameMotion4Flag){
        CmdFrameMotion4Flag = false;
        
        // Only one value for this case
        Gen.RawSettingFactorOfficial[15] = extract_and_compute_log(10);
        
        LOGA(NAMI, "F[15]=%.2f\r\n", Gen.RawSettingFactorOfficial[15]);
        submit_gpio_relay_status_to_mqtt_server(PRESENCE_MOTION_FACTOR_4);
        
    } else if(CmdOpenQueryProgressFlag){
        CmdOpenQueryProgressFlag = false;
        Gen.UartReceiveAck = (Gen.UartReceiveBuffer[9]<<8)|(Gen.UartReceiveBuffer[8]);
        Gen.UartReceivePercent = (Gen.UartReceiveBuffer[11]<<8)|(Gen.UartReceiveBuffer[10]);
        LOGA(NAMI, "Gen.UartReceiveAck=%u, Gen.UartReceivePercent=%u\r\n", Gen.UartReceiveAck, Gen.UartReceivePercent);
        submit_gpio_relay_status_to_mqtt_server(QUERY_PROGRESS);
        
    } else if(CmdSetAutoThreHoldFlag){
        CmdSetAutoThreHoldFlag = false;
        Gen.UartReceiveAck = (Gen.UartReceiveBuffer[9]<<8)|(Gen.UartReceiveBuffer[8]);
        LOGA(NAMI, "Gen.UartReceiveAck=%u\r\n", Gen.UartReceiveAck);
        submit_gpio_relay_status_to_mqtt_server(AUTO_THRESHOLD);
    }
}


*/


void UpdateFirmware(void){
    static bool SendCompletedFlag = false;
    static uint32_t UpdatePercentOld = 0;
    if(*Gen.GetStatusConnectFlag == true && NamiMQTT.StateUDFFlag == 50){
        LOGA(NAMI, "check var %u \r\n", NamiMQTT.StateUDFFlag)
        ef_set_u8("StateUDF", 100);
        NamiMQTT.StateUDFFlag = 0;
        SendCompletedFlag = true;

        vTaskDelay(pdMS_TO_TICKS(100));
        submit_gpio_relay_status_to_mqtt_server(UDFSUCCESS);
    }

    if(SendCompletedFlag){
        SendCompletedFlag = false;
        LOGA(IR, "Update completed\r\n");	
        vTaskDelay(pdMS_TO_TICKS(100));
        submit_gpio_relay_status_to_mqtt_server(UDFCOMPLETE);
    }

    if(*Gen.MainUpdateFirmwareFlag == NAMI_ENABLE){
        LOGA(NAMI, "Test update firmware, status(%d)\r\n", *Gen.MainUpdateFirmwareFlag);
        vTaskDelay(pdMS_TO_TICKS(100));  
        if(Gen.UpdateFirmwareBuf != NULL){
            ef_set_u8("StateUDF", 50);
            vTaskDelay(pdMS_TO_TICKS(100)); 
            LOGA(NAMI, "Test update firmware, Len(%d)\r\n", strlen((char *)&Gen.UpdateFirmwareBuf[0]));
            LOGA(NAMI, "Data(%s)\r\n", Gen.UpdateFirmwareBuf);
            axk_hal_handle_ota_json((char *)&Gen.UpdateFirmwareBuf[0]);
            *Gen.MainUpdateFirmwareFlag = 0;
            OnlyUpdateFirmwareFlag = true;
            memset(&Gen.UpdateFirmwareBuf, 0x00, sizeof(Gen.UpdateFirmwareBuf));
        }
    }

    if(OtaUDF.UpdatePercent != UpdatePercentOld && OtaUDF.UpdatePercent != 0){
        submit_gpio_relay_status_to_mqtt_server(OtaUDF.UpdatePercent);
        UpdatePercentOld = OtaUDF.UpdatePercent;
    }else if(!UpdatePercentOld){
        UpdatePercentOld = OtaUDF.UpdatePercent;
    }
}


static void NamiTimerProc(xTimerHandle pxTimer){
    static int ret = 0;
    static TickType_t current_time = 0;
    static TickType_t current_time_old = 0;
    static bool SendCompletedFlag = false;
    static uint32_t TimerLedNoInternetCount = 0;
    static bool ChangeStateFlag = false;
    char *ValRecvBuff = NULL;
    
    // current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;


    UpdateFirmware();

    if((*Gen.GetStatusConnectFlag == false) && (*Gen.MModeCalibFlag == false)){
        if(TimerLedNoInternetCount++ >= 2){
            if(ChangeStateFlag){
                LOGA(NAMI, "ON LED \r\n");
                NAMI_PRE_GPIO_OUTPUT_SET(GPIO_LED_NOTIFY, LED_ON);
                ChangeStateFlag = false;
                TimerLedNoInternetCount = 0;
            }else if(ChangeStateFlag == false){
                LOGA(NAMI, "OFF LED \r\n");
                NAMI_PRE_GPIO_OUTPUT_SET(GPIO_LED_NOTIFY, LED_OFF);
                ChangeStateFlag = true;
                TimerLedNoInternetCount = 0;
            }
        }       
    }
    // LOGA(NAMI, "current=%lu, old=%lu, OnlyUpdateFirmwareFlag=%d, MChangeCalibFlag=%d, MModeCalibFlag=%d\r\n", xTaskGetTickCount() * portTICK_PERIOD_MS
    //                                                                 , current_time_old
    //                                                                 , OnlyUpdateFirmwareFlag
    //                                                                 , *Gen.MChangeCalibFlag
    //                                                                 , *Gen.MModeCalibFlag);
    if(xTaskGetTickCount() * portTICK_PERIOD_MS - current_time_old >= 200
        && current_time_old != 0 
        && OnlyUpdateFirmwareFlag == false 
        && *Gen.MChangeCalibFlag == false
        && *Gen.MModeCalibFlag == false){
        Gen.TimerPublish++;
        Gen.UartReceiveCount = NAMI_PRE_UART_RECEIVE(&UART_ID_1, Gen.UartReceiveBuffer, sizeof(Gen.UartReceiveBuffer));
        Gen.ResultData = NAMI_PRE_ADC_VALUE_GET(&adc0, ADC_CHANNEL, 100);
        //  LOGA(NAMI, "DATA RAW={\033[0;32m%s\033[0;33m}\r\n", Gen.UartReceiveBuffer);

        if (Gen.UartReceiveCount > 0){
            LOGA(NAMI, "[%umS][ADC=%u][%u-%u]data read log:\r\n", xTaskGetTickCount() * portTICK_PERIOD_MS - current_time_old
                                                                , Gen.ResultData
                                                                , UpdateStateMotionFlag
                                                                , UpdateStateNoDetectFlag);
            if((BIT(NAMI) & FmDebug) == BIT(NAMI)){
                printf("\033[0;32m{");
                for(uint8_t PrtIdx = 0; PrtIdx < Gen.UartReceiveCount; PrtIdx++){
                    printf("\033[0;35m%02X ", Gen.UartReceiveBuffer[PrtIdx]);
                }
                printf("\033[0;32m}  ");
                printf("{\033[0;36m%s\033[0;32m}\r\n", Gen.UartReceiveBuffer);
            }

            // Đảm bảo kết thúc chuỗi
            if (Gen.UartReceiveCount < sizeof(Gen.UartReceiveBuffer)) {
                Gen.UartReceiveBuffer[Gen.UartReceiveCount] = '\0';
            } else {
                Gen.UartReceiveBuffer[sizeof(Gen.UartReceiveBuffer) - 1] = '\0';
            }

            if(Gen.UartReceiveBuffer[11] == '\r' && Gen.UartReceiveBuffer[12] == '\n'){
                Gen.UartPostionTail = 11;
                Gen.UartGetLen = 2;
            }else if(Gen.UartReceiveBuffer[12] == '\r' && Gen.UartReceiveBuffer[13] == '\n'){
                Gen.UartPostionTail = 12;
                Gen.UartGetLen = 3;
            }

            if(Gen.UartReceiveBuffer[0] == 'd' && Gen.UartReceiveBuffer[1] == 'i' && Gen.UartReceiveBuffer[2] == 's'
            && Gen.UartReceiveBuffer[3] == 't' && Gen.UartReceiveBuffer[4] == 'a' && Gen.UartReceiveBuffer[5] == 'n'
            && Gen.UartReceiveBuffer[6] == 'c' && Gen.UartReceiveBuffer[7] == 'e' && Gen.UartReceiveBuffer[8] == ':'){
                Gen.UartPostionHead = 9;
                memcpy(&Gen.UartValDistanceBuff[0], &Gen.UartReceiveBuffer[Gen.UartPostionHead], Gen.UartGetLen);
                Gen.UartValDistance = (uint16_t)atoi((char *)&Gen.UartValDistanceBuff);
                // LOGA(NAMI, "Head=%u, Len=%u, Dis=%u, DATA={%s}{%s}\r\n", Gen.UartPostionHead, Gen.UartGetLen, Gen.UartValDistance, Gen.UartValDistanceBuff, Gen.UartReceiveBuffer);
                if(Gen.UartValDistance > 0 && Gen.UartValDistance < 800){
                    Gen.UartValDistanceApprove++;
                }
            }else if(Gen.UartReceiveBuffer[0] == 'O' && Gen.UartReceiveBuffer[1] == 'F' && Gen.UartReceiveBuffer[2] == 'F'){
                Gen.UartNoDetect++;
            }else if(Gen.UartReceiveBuffer[0] == 0xFD && Gen.UartReceiveBuffer[1] == 0xFC && Gen.UartReceiveBuffer[2] == 0xFB && Gen.UartReceiveBuffer[3] == 0xFA){

                if(Gen.UartReceiveBuffer[6] == 0x07 && Gen.UartReceiveBuffer[7] == 0x01){
                    if(Gen.UartReceiveBuffer[8] == 0x00 && Gen.UartReceiveBuffer[9] == 0x00){
                        submit_gpio_relay_status_to_mqtt_server(PRESENCE_ACK);
                    }else{
                        submit_gpio_relay_status_to_mqtt_server(PRESENCE_FAIL);
                    }
                }else if(CmdFrame1Flag){
                    CmdFrame1Flag = false;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[25]<<24)|(Gen.UartReceiveBuffer[24]<<16)|(Gen.UartReceiveBuffer[23]<<8)|(Gen.UartReceiveBuffer[22]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[0] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[29]<<24)|(Gen.UartReceiveBuffer[28]<<16)|(Gen.UartReceiveBuffer[27]<<8)|(Gen.UartReceiveBuffer[26]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[1] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;
                    LOGA(NAMI, "Gen.TotalCalib=%f, Logarit=%f, F[0]=%.2f, F[1]=%.2f\r\n", 
                        Gen.TotalCalib, Gen.Log10Result, Gen.RawSettingFactorOfficial[0], Gen.RawSettingFactorOfficial[1]);
                    submit_gpio_relay_status_to_mqtt_server(PRESENCE_TRIGGER_FACTOR_1);
                }else if(CmdFrame2Flag){
                    CmdFrame2Flag = false;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[13]<<24)|(Gen.UartReceiveBuffer[12]<<16)|(Gen.UartReceiveBuffer[11]<<8)|(Gen.UartReceiveBuffer[10]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[2] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[17]<<24)|(Gen.UartReceiveBuffer[16]<<16)|(Gen.UartReceiveBuffer[15]<<8)|(Gen.UartReceiveBuffer[14]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[3] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[21]<<24)|(Gen.UartReceiveBuffer[20]<<16)|(Gen.UartReceiveBuffer[19]<<8)|(Gen.UartReceiveBuffer[18]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[4] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[25]<<24)|(Gen.UartReceiveBuffer[24]<<16)|(Gen.UartReceiveBuffer[23]<<8)|(Gen.UartReceiveBuffer[22]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[5] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[29]<<24)|(Gen.UartReceiveBuffer[28]<<16)|(Gen.UartReceiveBuffer[27]<<8)|(Gen.UartReceiveBuffer[26]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[6] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;
                    LOGA(NAMI, "Gen.TotalCalib=%f, Logarit=%f, F[2]=%.2f, F[3]=%.2f, F[4]=%.2f, F[5]=%.2f, F[6]=%.2f\r\n", 
                        Gen.TotalCalib, Gen.Log10Result, Gen.RawSettingFactorOfficial[2], Gen.RawSettingFactorOfficial[3], Gen.RawSettingFactorOfficial[4], 
                        Gen.RawSettingFactorOfficial[5], Gen.RawSettingFactorOfficial[6]);
                    submit_gpio_relay_status_to_mqtt_server(PRESENCE_TRIGGER_FACTOR_2);                    
                }else if(CmdFrame3Flag){
                    CmdFrame3Flag = false;
                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[13]<<24)|(Gen.UartReceiveBuffer[12]<<16)|(Gen.UartReceiveBuffer[11]<<8)|(Gen.UartReceiveBuffer[10]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[7] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[17]<<24)|(Gen.UartReceiveBuffer[16]<<16)|(Gen.UartReceiveBuffer[15]<<8)|(Gen.UartReceiveBuffer[14]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[8] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[21]<<24)|(Gen.UartReceiveBuffer[20]<<16)|(Gen.UartReceiveBuffer[19]<<8)|(Gen.UartReceiveBuffer[18]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[9] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[25]<<24)|(Gen.UartReceiveBuffer[24]<<16)|(Gen.UartReceiveBuffer[23]<<8)|(Gen.UartReceiveBuffer[22]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[10] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[29]<<24)|(Gen.UartReceiveBuffer[28]<<16)|(Gen.UartReceiveBuffer[27]<<8)|(Gen.UartReceiveBuffer[26]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[11] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;
                    LOGA(NAMI, "Gen.TotalCalib=%f, Logarit=%f, F[7]=%.2f, F[8]=%.2f, F[9]=%.2f, F[10]=%.2f, F[11]=%.2f\r\n", 
                        Gen.TotalCalib, Gen.Log10Result, Gen.RawSettingFactorOfficial[7], Gen.RawSettingFactorOfficial[8], Gen.RawSettingFactorOfficial[9], 
                        Gen.RawSettingFactorOfficial[10], Gen.RawSettingFactorOfficial[11]);
                    submit_gpio_relay_status_to_mqtt_server(PRESENCE_TRIGGER_FACTOR_3);                    
                }else if(CmdFrame4Flag){
                    CmdFrame4Flag = false;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[13]<<24)|(Gen.UartReceiveBuffer[12]<<16)|(Gen.UartReceiveBuffer[11]<<8)|(Gen.UartReceiveBuffer[10]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[12] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[17]<<24)|(Gen.UartReceiveBuffer[16]<<16)|(Gen.UartReceiveBuffer[15]<<8)|(Gen.UartReceiveBuffer[14]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[13] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[21]<<24)|(Gen.UartReceiveBuffer[20]<<16)|(Gen.UartReceiveBuffer[19]<<8)|(Gen.UartReceiveBuffer[18]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[14] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[25]<<24)|(Gen.UartReceiveBuffer[24]<<16)|(Gen.UartReceiveBuffer[23]<<8)|(Gen.UartReceiveBuffer[22]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[15] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[29]<<24)|(Gen.UartReceiveBuffer[28]<<16)|(Gen.UartReceiveBuffer[27]<<8)|(Gen.UartReceiveBuffer[26]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[16] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;
                    LOGA(NAMI, "Gen.TotalCalib=%f, Logarit=%f, F[12]=%.2f, F[13]=%.2f, F[14]=%.2f, F[15]=%.2f, F[16]=%.2f\r\n", 
                        Gen.TotalCalib, Gen.Log10Result, Gen.RawSettingFactorOfficial[12], Gen.RawSettingFactorOfficial[13], Gen.RawSettingFactorOfficial[14], 
                        Gen.RawSettingFactorOfficial[15], Gen.RawSettingFactorOfficial[16]);
                    submit_gpio_relay_status_to_mqtt_server(PRESENCE_TRIGGER_FACTOR_4);                    
                }else if(CmdFrameMotion1Flag){
                    CmdFrameMotion1Flag = false;
                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[13]<<24)|(Gen.UartReceiveBuffer[12]<<16)|(Gen.UartReceiveBuffer[11]<<8)|(Gen.UartReceiveBuffer[10]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[0] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[17]<<24)|(Gen.UartReceiveBuffer[16]<<16)|(Gen.UartReceiveBuffer[15]<<8)|(Gen.UartReceiveBuffer[14]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[1] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[21]<<24)|(Gen.UartReceiveBuffer[20]<<16)|(Gen.UartReceiveBuffer[19]<<8)|(Gen.UartReceiveBuffer[18]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[2] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[25]<<24)|(Gen.UartReceiveBuffer[24]<<16)|(Gen.UartReceiveBuffer[23]<<8)|(Gen.UartReceiveBuffer[22]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[3] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[29]<<24)|(Gen.UartReceiveBuffer[28]<<16)|(Gen.UartReceiveBuffer[27]<<8)|(Gen.UartReceiveBuffer[26]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[4] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    LOGA(NAMI, "F[0]=%.2f, F[1]=%.2f, F[2]=%.2f, F[3]=%.2f, F[4]=%.2f\r\n", 
                        Gen.RawSettingFactorOfficial[0], Gen.RawSettingFactorOfficial[1], Gen.RawSettingFactorOfficial[2], 
                        Gen.RawSettingFactorOfficial[3], Gen.RawSettingFactorOfficial[4]);
                    submit_gpio_relay_status_to_mqtt_server(PRESENCE_MOTION_FACTOR_1);
                }else if(CmdFrameMotion2Flag){
                    CmdFrameMotion2Flag = false;
                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[13]<<24)|(Gen.UartReceiveBuffer[12]<<16)|(Gen.UartReceiveBuffer[11]<<8)|(Gen.UartReceiveBuffer[10]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[5] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[17]<<24)|(Gen.UartReceiveBuffer[16]<<16)|(Gen.UartReceiveBuffer[15]<<8)|(Gen.UartReceiveBuffer[14]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[6] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[21]<<24)|(Gen.UartReceiveBuffer[20]<<16)|(Gen.UartReceiveBuffer[19]<<8)|(Gen.UartReceiveBuffer[18]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[7] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[25]<<24)|(Gen.UartReceiveBuffer[24]<<16)|(Gen.UartReceiveBuffer[23]<<8)|(Gen.UartReceiveBuffer[22]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[8] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[29]<<24)|(Gen.UartReceiveBuffer[28]<<16)|(Gen.UartReceiveBuffer[27]<<8)|(Gen.UartReceiveBuffer[26]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[9] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    LOGA(NAMI, "F[5]=%.2f, F[6]=%.2f, F[7]=%.2f, F[8]=%.2f, F[9]=%.2f\r\n", 
                        Gen.RawSettingFactorOfficial[5], Gen.RawSettingFactorOfficial[6], Gen.RawSettingFactorOfficial[7], 
                        Gen.RawSettingFactorOfficial[8], Gen.RawSettingFactorOfficial[9]);
                    submit_gpio_relay_status_to_mqtt_server(PRESENCE_MOTION_FACTOR_2);
                }else if(CmdFrameMotion3Flag){
                    CmdFrameMotion3Flag = false;
                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[13]<<24)|(Gen.UartReceiveBuffer[12]<<16)|(Gen.UartReceiveBuffer[11]<<8)|(Gen.UartReceiveBuffer[10]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[10] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[17]<<24)|(Gen.UartReceiveBuffer[16]<<16)|(Gen.UartReceiveBuffer[15]<<8)|(Gen.UartReceiveBuffer[14]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[11] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[21]<<24)|(Gen.UartReceiveBuffer[20]<<16)|(Gen.UartReceiveBuffer[19]<<8)|(Gen.UartReceiveBuffer[18]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[12] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[25]<<24)|(Gen.UartReceiveBuffer[24]<<16)|(Gen.UartReceiveBuffer[23]<<8)|(Gen.UartReceiveBuffer[22]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[13] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[29]<<24)|(Gen.UartReceiveBuffer[28]<<16)|(Gen.UartReceiveBuffer[27]<<8)|(Gen.UartReceiveBuffer[26]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[14] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    LOGA(NAMI, "F[10]=%.2f, F[11]=%.2f, F[12]=%.2f, F[13]=%.2f, F[14]=%.2f\r\n", 
                        Gen.RawSettingFactorOfficial[10], Gen.RawSettingFactorOfficial[11], Gen.RawSettingFactorOfficial[12], 
                        Gen.RawSettingFactorOfficial[13], Gen.RawSettingFactorOfficial[14]);
                    submit_gpio_relay_status_to_mqtt_server(PRESENCE_MOTION_FACTOR_3);
                }else if(CmdFrameMotion4Flag){
                    CmdFrameMotion4Flag = false;
                    Gen.TotalCalib = (float)((Gen.UartReceiveBuffer[13]<<24)|(Gen.UartReceiveBuffer[12]<<16)|(Gen.UartReceiveBuffer[11]<<8)|(Gen.UartReceiveBuffer[10]));
                    Gen.Log10Result = log10f(Gen.TotalCalib);
                    Gen.RawSettingFactorOfficial[15] = roundf(Gen.Log10Result * 1000.0f) / 100.0f;

                    LOGA(NAMI, "F[15]=%.2f\r\n", Gen.RawSettingFactorOfficial[15]);
                    submit_gpio_relay_status_to_mqtt_server(PRESENCE_MOTION_FACTOR_4);
                }else if(CmdOpenQueryProgressFlag){
                    CmdOpenQueryProgressFlag = false;
                    Gen.UartReceiveAck = (Gen.UartReceiveBuffer[9]<<8)|(Gen.UartReceiveBuffer[8]);
                    Gen.UartReceivePercent = (Gen.UartReceiveBuffer[11]<<8)|(Gen.UartReceiveBuffer[10]);
                    LOGA(NAMI, "Gen.UartReceiveAck=%u, Gen.UartReceivePercent=%u\r\n", Gen.UartReceiveAck, Gen.UartReceivePercent);
                    submit_gpio_relay_status_to_mqtt_server(QUERY_PROGRESS);   // PIR_MOTION   PRESENCE
                }else if(CmdSetAutoThreHoldFlag){
                    CmdSetAutoThreHoldFlag = false;
                    Gen.UartReceiveAck = (Gen.UartReceiveBuffer[9]<<8)|(Gen.UartReceiveBuffer[8]);
                    LOGA(NAMI, "Gen.UartReceiveAck=%u\r\n", Gen.UartReceiveAck);
                    submit_gpio_relay_status_to_mqtt_server(AUTO_THRESHOLD);   // PIR_MOTION   PRESENCE
                }
            }
        }

        if(CmdOpenSettingFlag == false){
            if(Gen.TimerPublish >= 1){
                if(Gen.UartValDistanceApprove > 1 && UpdateStateMotionFlag){
                    if(*Gen.GetStatusConnectFlag && *Gen.MainConfigBeginFlag == false){
                        Gen.CountDetectMotion++;
                        LOGA(NAMI, " Gen.TimerPublish=%u/ CountDetectMotion=%u\r\n",  Gen.TimerPublish * PDS_WAKEUP_MS, Gen.CountDetectMotion);
                        submit_gpio_relay_status_to_mqtt_server(PIR_MOTION);   // PIR_MOTION   PRESENCE
                        Gen.TimerPublish = 0;
                        Gen.UartValDistanceApprove = 0;
                        Gen.UartNoDetect = 0;
                        Detect3Status = 1;
                        UpdateStateMotionFlag = false;
                        UpdateStateNoDetectFlag = true;
                    }else{
                        ERR(NAMI, "Err: not yet connect MQTT (%u)\r\n", *Gen.GetStatusConnectFlag);
                    }
                }
                
                if(Gen.UartNoDetect >= 2 && UpdateStateNoDetectFlag){
                    if(*Gen.GetStatusConnectFlag && *Gen.MainConfigBeginFlag == false){
                        LOGA(NAMI, " Gen.TimerPublish=%u\r\n",  Gen.TimerPublish * PDS_WAKEUP_MS);
                        submit_gpio_relay_status_to_mqtt_server(NO_DETECT_PRESENCE);
                        Detect3Status = 0;
                        Gen.CountDetectMotion = 0;
                        Gen.TimerPublish = 0;
                        Gen.UartValDistanceApprove = 0;
                        Gen.UartNoDetect = 0;
                        UpdateStateNoDetectFlag = false;
                        UpdateStateMotionFlag = true;
                    }
                }
            }
        }else if(CmdOpenSettingFlag && PubCmdOpenSettingFlag){
            PubCmdOpenSettingFlag = false;
            LOGA(NAMI, "Open mode calibration sensor\r\n");
            submit_gpio_relay_status_to_mqtt_server(OPEN_CLIB);
        }

        if(CmndCloseSettingFlag && PubCmdCloseSettingFlag){
            PubCmdCloseSettingFlag = false;
            LOGA(NAMI, "Close mode calibration sensor\r\n");
            submit_gpio_relay_status_to_mqtt_server(CLOSE_CLIB);            
        }
        
        // CHỈ XÓA BUFFER UART

        memset(&Gen.RawSettingFactorOfficial, 0x00, sizeof(Gen.RawSettingFactorOfficial));
        memset(&Gen.UartReceiveBuffer, 0x00, sizeof(Gen.UartReceiveBuffer));
        memset(&Gen.UartValDistanceBuff, 0x00, sizeof(Gen.UartValDistanceBuff));
        Gen.UartReceiveCount = 0;
        
        // Reset con trỏ
        ValRecvBuff = NULL;

        current_time_old = xTaskGetTickCount() * portTICK_PERIOD_MS;
    } else if(current_time_old == 0){
        current_time_old = xTaskGetTickCount() * portTICK_PERIOD_MS;
    }
}

void main(){
    // Initialize EasyFlash first
    if (easyflash_init() != EF_NO_ERR) {
        printf("[MAIN] Critical Error: Failed to initialize EasyFlash!\n");
        while(1); // Halt if flash init fails
    }


    NAMI_PRE_SYSTEM_INIT();

    NAMI_PRE_UART_INIT(&uart_dev_echo);
    NAMI_PRE_UART_INIT(&UART_ID_1);

	initialize_switch();
    LedNotifyStartPro();

    PLOG_Init();
    // PLOG.Stop(ALL);
    NAMI_PRE_ADC_INIT(&adc0);
    NAMI_PRE_ADC_ADD_CHANNEL(&adc0, ADC_CHANNEL);

    puts("\033[0;32m*************************************************** \r\n"
        "\033[1;31m       NAMI TECHNOLOGIES JOINT STOCK COMPANY        \r\n"
        "\033[0;36m       Project: \033[1;31mPre Sensor                \r\n"
        "\033[0;36m       Author: \033[1;31mR&D Firmware Team          \r\n"
        "\033[0;36m       Date of time: \033[1;31mAug 21, 2025       \r\n"
        "\033[0;36m       Version: \033[1;31m251206_01_main            \r\n"
        "\033[0;32m*************************************************** \r\n\033[0;37m"
        "\033[38;5;15m \033[0m\n\n");

	tcpip_init(NULL, NULL); 
	xTaskCreate(wifi_execute, "wifi execute", 1024, NULL, 14, NULL);
	xTaskCreate(mqtt_start, "mqtt task", 1024, NULL, 14, &mqtt_task_handle);
    // xTaskCreate(handle_led_wifi, "wifi led", 512, NULL, 15, &led_wifi_task_handle);
	xTaskCreate(relay_event_task, "relay_event_task", 640, NULL, 14, &relay_event_task_handle); // Increased stack to 2048

    memset(&Gen, 0x00, sizeof(Gen));
    Gen.MainStatusBroker                  = &NamiMQTT.StatusBroker;
    Gen.MainSetCount                      = &NamiSwitch.SetCount;
    Gen.MainUpdateFirmwareFlag            = &NamiMQTT.UpdateFirmwareFlag;
    Gen.MainDuration                      = &NamiSwitch.Duration;
    Gen.GetStatusConnectFlag              = &NamiMQTT.StatusConnectFlag;
    Gen.MChangeCalibFlag                  = &NamiSwitch.ChangeCalibFlag;
    Gen.MModeCalibFlag                    = &NamiSwitch.ModeCalibFlag;

    Gen.MainConfigBeginFlag               = &NamiMQTT.ConfigBeginFlag;

    UpdateStateMotionFlag = true;
    UpdateStateNoDetectFlag = true;


    NAMI_PRE_GPIO_ENABLE_INPUT(GPIO_KEY_IN12, 0, 0); // Enable input for sleep trigger pin with pull-down

    TimerHandle_t timer_handler;
    timer_handler = xTimerCreate("NamiTimerProc", pdMS_TO_TICKS(PDS_WAKEUP_MS), pdTRUE, NULL, NamiTimerProc);
    xTimerStart(timer_handler, 0);

    uint32_t ValuePIR;

    // Initialize signal state
    signal_state.level = NAMI_PRE_GPIO_INPUT_GET_VALUE(4);
    signal_state.start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    signal_state.count = 0;

    // Thêm biến để lưu trạng thái trước đó của GPIO 17
    static uint8_t previous_gpio_state = 0xFF; // Khởi tạo với giá trị không hợp lệ để đảm bảo set lần đầu
    static uint8_t previous_pir_state = 0xFF; // Khởi tạo với giá trị không hợp lệ
    static uint32_t Cnt = 0;
    while(1){
        if(*Gen.MChangeCalibFlag == false && *Gen.MModeCalibFlag == false && (*Gen.GetStatusConnectFlag) && OnlyUpdateFirmwareFlag == false){
            if(!Detect3Status){
                NAMI_PRE_GPIO_OUTPUT_SET(GPIO_LED_NOTIFY, LED_OFF);
            }else if(Detect3Status == 1){
                NAMI_PRE_GPIO_OUTPUT_SET(GPIO_LED_NOTIFY, LED_ON); // detect object
            }
        }
         vTaskDelay(pdMS_TO_TICKS(100));
    }
}

