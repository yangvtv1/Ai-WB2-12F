
#include "mqtt.h"




nami_mqtt_t NamiMQTT;
TaskHandle_t MQTTTaskHandle;

char gdevice_id[DEVICE_ID_LENGTH];
char gverification_code[VERIFICATION_CODE_LENGTH];
uint8_t gdevice_status;
char gmac[18];



#define cJSON_IsNumber(item) ((item) != NULL && (item)->type == cJSON_Number)
#define cJSON_IsObject(item) ((item) != NULL && (item)->type == cJSON_Object)
#define cJSON_IsString(item) ((item) != NULL && (item)->type == cJSON_String)
#define cJSON_IsArray(item) ((item) != NULL && (item)->type == cJSON_Array)
#define cJSON_ArrayForEach(element, array) for(element = (array != NULL) ? (array)->child : NULL; element != NULL; element = element->next)

static const uint8_t TEST_CERTIFICATE_FILENAME[] = 
    {"-----BEGIN CERTIFICATE-----\r\n"
    "MIIEAzCCAuugAwIBAgIUBY1hlCGvdj4NhBXkZ/uLUZNILAwwDQYJKoZIhvcNAQEL\r\n"
    "BQAwgZAxCzAJBgNVBAYTAkdCMRcwFQYDVQQIDA5Vbml0ZWQgS2luZ2RvbTEOMAwG\r\n"
    "A1UEBwwFRGVyYnkxEjAQBgNVBAoMCU1vc3F1aXR0bzELMAkGA1UECwwCQ0ExFjAU\r\n"
    "BgNVBAMMDW1vc3F1aXR0by5vcmcxHzAdBgkqhkiG9w0BCQEWEHJvZ2VyQGF0Y2hv\r\n"
    "by5vcmcwHhcNMjAwNjA5MTEwNjM5WhcNMzAwNjA3MTEwNjM5WjCBkDELMAkGA1UE\r\n"
    "BhMCR0IxFzAVBgNVBAgMDlVuaXRlZCBLaW5nZG9tMQ4wDAYDVQQHDAVEZXJieTES\r\n"
    "MBAGA1UECgwJTW9zcXVpdHRvMQswCQYDVQQLDAJDQTEWMBQGA1UEAwwNbW9zcXVp\r\n"
    "dHRvLm9yZzEfMB0GCSqGSIb3DQEJARYQcm9nZXJAYXRjaG9vLm9yZzCCASIwDQYJ\r\n"
    "KoZIhvcNAQEBBQADggEPADCCAQoCggEBAME0HKmIzfTOwkKLT3THHe+ObdizamPg\r\n"
    "UZmD64Tf3zJdNeYGYn4CEXbyP6fy3tWc8S2boW6dzrH8SdFf9uo320GJA9B7U1FW\r\n"
    "Te3xda/Lm3JFfaHjkWw7jBwcauQZjpGINHapHRlpiCZsquAthOgxW9SgDgYlGzEA\r\n"
    "s06pkEFiMw+qDfLo/sxFKB6vQlFekMeCymjLCbNwPJyqyhFmPWwio/PDMruBTzPH\r\n"
    "3cioBnrJWKXc3OjXdLGFJOfj7pP0j/dr2LH72eSvv3PQQFl90CZPFhrCUcRHSSxo\r\n"
    "E6yjGOdnz7f6PveLIB574kQORwt8ePn0yidrTC1ictikED3nHYhMUOUCAwEAAaNT\r\n"
    "MFEwHQYDVR0OBBYEFPVV6xBUFPiGKDyo5V3+Hbh4N9YSMB8GA1UdIwQYMBaAFPVV\r\n"
    "6xBUFPiGKDyo5V3+Hbh4N9YSMA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEL\r\n"
    "BQADggEBAGa9kS21N70ThM6/Hj9D7mbVxKLBjVWe2TPsGfbl3rEDfZ+OKRZ2j6AC\r\n"
    "6r7jb4TZO3dzF2p6dgbrlU71Y/4K0TdzIjRj3cQ3KSm41JvUQ0hZ/c04iGDg/xWf\r\n"
    "+pp58nfPAYwuerruPNWmlStWAXf0UTqRtg4hQDWBuUFDJTuWuuBvEXudz74eh/wK\r\n"
    "sMwfu1HFvjy5Z0iMDU8PUDepjVolOCue9ashlS4EB5IECdSR2TItnAIiIwimx839\r\n"
    "LdUdRudafMu5T5Xma182OC0/u/xRlEm+tvKGGmfFcN0piqVl8OrSPBgIlb+1IKJE\r\n"
    "m/XriWr/Cq4h/JfB7NTsezVslgkBaoU=\r\n"
    "-----END CERTIFICATE-----\r\n"};

char json_data[2000];
uint32_t JsonCnt = 0;

static const char *Topic[8] = {
    "device/command/",
    "device/command/",
    "device/command/",
    "device/ota/",
    "device/ota/",
    "device/availability/",
    "device/config/",
    "device/env/"
};

static const char *LastWill[2] = {
    "{\"status\":\"online\"}", 
    "{\"status\":\"offline\"}"
};


bool gmqtt_connected = false;  // Track MQTT client state
axk_mqtt_client_handle_t gclient_mqtt;
bool LearnRspFlag = false;
uint8_t NeedToLearn;


// uint8_t AckUpdateStatus;



void submit_gpio_relay_status_to_mqtt_server(uint8_t status) {
    if(OtaUDF.UpdatePercent){
        snprintf(json_data, sizeof(json_data), "{\"OTA_progress\":\"%d%%\"}", OtaUDF.UpdatePercent);
    }else if(status == CHAIN_ZERO){
        snprintf(json_data, sizeof(json_data), "{\"status\":\"Re-check buffer IR, cuz have index = 0\"}"); 
    }else if(status == NO_BEST_DIV){
        snprintf(json_data, sizeof(json_data), "{\"status\":\"No find out best div with chain\"}"); 
    }else if(status == NOISE_BUTTON){
        snprintf(json_data, sizeof(json_data), "{\"status\":\"Err: NOISE - Duration=%d/ count=%d\"}", NamiSwitch.Duration, NamiSwitch.SetCount);
    }else if(status == NORMAL){
        snprintf(json_data, sizeof(json_data), "{\"status\":\"normal_reboot\"}");  
    }else if(status == NEWVER){
        snprintf(json_data, sizeof(json_data), "{\"status\":\"Update_firmware\", \"version_old\":\"%s\", \"version_current\":\"%s\"}", NamiMQTT.CheckVer, FIRMWAREVERSION);  
    }else if(status == UDFSUCCESS){
        snprintf(json_data, sizeof(json_data), "{\"OTA_status\":\"Success\"}");  
    }else if(status == UDFCOMPLETE){
        snprintf(json_data, sizeof(json_data), "{\"OTA_status\":\"Completed\"}");  
    }else if(status == SETVER){
        snprintf(json_data, sizeof(json_data), "{\"status\":\"set_firmware_begin\"}"); 
    }else if(status == ERR_NUM_COMPARE){
        snprintf(json_data, sizeof(json_data), "{\"status\":\"Err: No number to compare\"}"); 
    }else if(status == MAC_ADD){
        snprintf(json_data, sizeof(json_data), "{\"MAC_ADDRESS\":\"%s\"}", NamiMQTT.mac_str); 
    }else if(status == TEMPHUMID){
        snprintf(json_data, sizeof(json_data), "{\"temperate\":\"%.0f\",\"humidity\":\"%.0f\"}", NAMI_IR.temperature, NAMI_IR.humidity); 
    }else if(status == RESET_BUTTON){
        snprintf(json_data, sizeof(json_data), "{\"message\":\"Reboot firmware by button\"}"); 
    }else if(status == PARA){
        snprintf(json_data, sizeof(json_data), "{\"message\":\"clk: %u, timeout: %u, address: 0x%02X, init: 0x%02X, trig: 0x%02X, rst: 0x%02X, id: %u, speed: %u, timeout: %u\"}"                                                                                                                           , IR_I2C_PARA_TIMEOUT);
    }else if(status == SEND_OK_COUNT_FRAME){
        snprintf(json_data, sizeof(json_data), "{\"status\":\"Send_successfully frame %d \"}", NamiMQTT.CntFrame);
    }else if(status == NUMBER_CALIB_COMPARE){
        snprintf(json_data, sizeof(json_data), "{\"status\":\"(LOW:%u, HIGH:%u) Value:%u, (LOW:%u, HIGH:%u) value:%u\"}", NamiMQTT.NumCalib[0][0], NamiMQTT.NumCalib[0][1], NamiMQTT.NumCalib[0][2]
																		                                                , NamiMQTT.NumCalib[1][0], NamiMQTT.NumCalib[1][1], NamiMQTT.NumCalib[1][2]);
    }else if(status == RANGE_DELAY){
         snprintf(json_data, sizeof(json_data), "{\"status\":\"LOW[%u,%u], MEDIUM[%u,%u], HIGH[%u,%u]\"}", NamiMQTT.RangeDelayBuffer[LEVEL_LOW][POSA], NamiMQTT.RangeDelayBuffer[LEVEL_LOW][POSB]
                                                                                                         , NamiMQTT.RangeDelayBuffer[LEVEL_MEDIUM][POSA], NamiMQTT.RangeDelayBuffer[LEVEL_MEDIUM][POSB]
                                                                                                         , NamiMQTT.RangeDelayBuffer[LEVEL_HIGH][POSA], NamiMQTT.RangeDelayBuffer[LEVEL_HIGH][POSB]);       
    }else if(status == SEND_OK){
        snprintf(json_data, sizeof(json_data), "{\"status\":\"%s\"}", NamiMQTT.StatusBuffer);
    }else{
        snprintf(json_data, sizeof(json_data), "{\"status\":\"%s\"}", (status == SEND_OK)?("Send_successfully"):((status == SEND_UPDATE)?("Update_Firmware"):("Error:_send_fail")));
    }

    // Publish the JSON data to the MQTT server
    if (gmqtt_connected) {
        if(OtaUDF.UpdatePercent || status == UDFSUCCESS || status == UDFCOMPLETE || status == NEWVER){
            LOGA(NAMI, "Published:{%s} \r\n",Topic[4]);
            axk_mqtt_client_publish(gclient_mqtt, Topic[4], json_data, 0, 0, 0);
        }else if(status == TEMPHUMID){
            axk_mqtt_client_publish(gclient_mqtt, Topic[7], json_data, 0, 0, 0);            
        }else if(status != SEND_LEAR){
            LOGA(NAMI, "Published:{%s} \r\n"
                       "{%s}\r\n",Topic[1], json_data);
            axk_mqtt_client_publish(gclient_mqtt, Topic[1], json_data, 0, 0, 0);
        }else{
            LOGA(NAMI, "Published:{%s} \r\n",Topic[1]);
            axk_mqtt_client_publish(gclient_mqtt, Topic[1], (char *)&NamiMQTT.LearnBuffer, 0, 0, 0);
        }
    }
    else {
        LOGA(NAMI, "MQTT client is not connected.\r\n");
    }
}

uint16_t NamiMQTTConvertUintToString(uint16_t *Data, uint32_t Counter){
	char TempBuffer[100] = "";
	uint8_t NumEven = Counter/10;
	uint8_t NumOdd = Counter%10;
    memset(&NamiMQTT.LearnBuffer, 0x00, sizeof(NamiMQTT.LearnBuffer));
	NamiMQTT.LearnCount = 0;

	LOGA(NAMI, "NumEven=%d NumOdd=%d\r\n", NumEven, NumOdd);
    // LOGMQTT("NumEven=%d NumOdd=%d\r\n", NumEven, NumOdd);
    memcpy(&NamiMQTT.LearnBuffer[NamiMQTT.LearnCount], LEARNFORMAT, strlen(LEARNFORMAT));
    NamiMQTT.LearnCount += strlen(LEARNFORMAT);

	for(uint16_t Evenidx = 0; Evenidx < NumEven; Evenidx++) {
		memset(TempBuffer, 0x00, sizeof(TempBuffer));
		sprintf(TempBuffer, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", Data[(Evenidx * 10) + 0], Data[(Evenidx * 10) + 1], Data[(Evenidx * 10) + 2],
                                                             Data[(Evenidx * 10) + 3], Data[(Evenidx * 10) + 4], Data[(Evenidx * 10) + 5],
                                                             Data[(Evenidx * 10) + 6], Data[(Evenidx * 10) + 7], Data[(Evenidx * 10) + 8],
                                                             Data[(Evenidx * 10) + 9]);
		
		//Countidx += strlen(NamiSys.PulseWidth);
		//printf("[%d][%s] - %d: %s\r\n", __LINE__, __func__, idx + 1, TempBuffer);
		memcpy(&NamiMQTT.LearnBuffer[NamiMQTT.LearnCount], TempBuffer, strlen(TempBuffer));
		NamiMQTT.LearnCount += strlen(TempBuffer);
		//printf("[%d][%s] - test \r\n", __LINE__, __func__);
        memcpy(&NamiMQTT.LearnBuffer[NamiMQTT.LearnCount], ",", 1);
		NamiMQTT.LearnCount += 1;
	}

    if(NumOdd == 1 || NumOdd == 2 || NumOdd == 3 || NumOdd == 4|| NumOdd == 5
	               || NumOdd == 6 || NumOdd == 7 || NumOdd == 8 || NumOdd == 9){
        memset(TempBuffer, 0x00, sizeof(TempBuffer));;
		LOGA(NAMI, "reset buffer temp\r\n");
        // LOGMQTT("reset buffer temp\r\n");
	}

	if(NumOdd == 1){
		sprintf(TempBuffer, "%d", Data[(NumEven * 10) + NumOdd - 1]);
	}else if(NumOdd == 2){
		sprintf(TempBuffer, "%d,%d", Data[(NumEven * 10) + NumOdd - 2], Data[(NumEven * 10) + NumOdd - 1]);
	}else if(NumOdd == 3){
		sprintf(TempBuffer, "%d,%d,%d", Data[(NumEven * 10) + NumOdd - 3], Data[(NumEven * 10) + NumOdd - 2], Data[(NumEven * 10) + NumOdd - 1]);
	}else if(NumOdd == 4){
		sprintf(TempBuffer, "%d,%d,%d,%d", Data[(NumEven * 10) + NumOdd - 4], Data[(NumEven * 10) + NumOdd - 3], Data[(NumEven * 10) + NumOdd - 2]
										 , Data[(NumEven * 10) + NumOdd - 1]);
	}else if(NumOdd == 5){
		sprintf(TempBuffer, "%d,%d,%d,%d,%d", Data[(NumEven * 10) + NumOdd - 5], Data[(NumEven * 10) + NumOdd - 4], Data[(NumEven * 10) + NumOdd - 3]
											, Data[(NumEven * 10) + NumOdd - 2], Data[(NumEven * 10) + NumOdd - 1]);
	}else if(NumOdd == 6){
		sprintf(TempBuffer, "%d,%d,%d,%d,%d,%d", Data[(NumEven * 10) + NumOdd - 6], Data[(NumEven * 10) + NumOdd - 5], Data[(NumEven * 10) + NumOdd - 4]
											   , Data[(NumEven * 10) + NumOdd - 3], Data[(NumEven * 10) + NumOdd - 2], Data[(NumEven * 10) + NumOdd - 1]);
	}else if(NumOdd == 7){
		sprintf(TempBuffer, "%d,%d,%d,%d,%d,%d,%d", Data[(NumEven * 10) + NumOdd - 7], Data[(NumEven * 10) + NumOdd - 6], Data[(NumEven * 10) + NumOdd - 5]
												  , Data[(NumEven * 10) + NumOdd - 4], Data[(NumEven * 10) + NumOdd - 3], Data[(NumEven * 10) + NumOdd - 2]
												  , Data[(NumEven * 10) + NumOdd - 1]);
	}else if(NumOdd == 8){
		sprintf(TempBuffer, "%d,%d,%d,%d,%d,%d,%d,%d", Data[(NumEven * 10) + NumOdd - 8], Data[(NumEven * 10) + NumOdd - 7], Data[(NumEven * 10) + NumOdd - 6]
                                                     , Data[(NumEven * 10) + NumOdd - 5], Data[(NumEven * 10) + NumOdd - 4], Data[(NumEven * 10) + NumOdd - 3]
                                                     , Data[(NumEven * 10) + NumOdd - 2], Data[(NumEven * 10) + NumOdd - 1]);
	}else if(NumOdd == 9){
		sprintf(TempBuffer, "%d,%d,%d,%d,%d,%d,%d,%d,%d", Data[(NumEven * 10) + NumOdd - 9], Data[(NumEven * 10) + NumOdd - 8], Data[(NumEven * 10) + NumOdd - 7]
                                                        , Data[(NumEven * 10) + NumOdd - 6], Data[(NumEven * 10) + NumOdd - 5], Data[(NumEven * 10) + NumOdd - 4]
                                                        , Data[(NumEven * 10) + NumOdd - 3], Data[(NumEven * 10) + NumOdd - 2], Data[(NumEven * 10) + NumOdd - 1]);
	}
 
	if(NumOdd == 1 || NumOdd == 2 || NumOdd == 3 || NumOdd == 4|| NumOdd == 5
	               || NumOdd == 6 || NumOdd == 7 || NumOdd == 8 || NumOdd == 9){
		memcpy(&NamiMQTT.LearnBuffer[NamiMQTT.LearnCount], TempBuffer, strlen(TempBuffer));
		NamiMQTT.LearnCount += strlen(TempBuffer);
		// printf("[%d][%s] - %d: {%s}/ Total Counter %d\r\n", __LINE__, __func__, NumOdd, NamiMQTT.LearnBuffer, NamiMQTT.LearnCount);
        // LOGMQTT("%d: {%s}/ Total Counter %d\r\n", NumOdd, TempBuffer, NamiMQTT.GenCounter);
	}

    memcpy(&NamiMQTT.LearnBuffer[NamiMQTT.LearnCount], LEARNFORMATEND, strlen(LEARNFORMATEND));
    NamiMQTT.LearnCount += strlen(LEARNFORMATEND);
	
	LOGA(NAMI, "Buffer[%d]={", __LINE__, __func__, NamiMQTT.LearnCount);
    if((BIT(NAMI) & FmDebug) == BIT(NAMI)){
        for(int j = 0; j < NamiMQTT.LearnCount; j++){
            printf("%c", NamiMQTT.LearnBuffer[j]);
        }
        printf("}\r\n");
    }
    // LOGMQTT("}\r\n");
    return NamiMQTT.LearnCount;
}

uint32_t MQTTIRLearn(uint8_t LearnOrListen){ 
    //---------------------------------------------------------------------------------------------------------------------
	//memset(&LearnBuffer, 0, sizeof(LearnBuffer));

    memset(&NamiMQTT.GenBuffer, 0x00, sizeof(NamiMQTT.GenBuffer));
    NamiMQTT.GenCounter = 0;

    NeedToLearn = 100;
    NamiMQTT.IR_Init(0);
    if(LearnOrListen == ELEARN)
        NamiMQTT.ModeTimeoutFlag = true;
    else if(LearnOrListen == ELISTEN)
        NamiMQTT.ModeTimeoutFlag = false;

	NamiMQTT.GenCounter = NamiMQTT.IR_LearnToReceive(IR_RX_SWM, (uint16_t *)&NamiMQTT.GenBuffer, NamiMQTT.ModeTimeoutFlag);

    return NamiMQTT.GenCounter;
}

void handle_control_ir(const char *event_data) {
    uint16_t ValueDouble;
    LOGA(NAMI, "Handle control switch IR\r\n");
    
    // Tìm vị trí bắt đầu của chuỗi "ir" với các khoảng trắng khác nhau
    const char *patterns[] = {
        "\"ir\":[",
        "\"ir\" :[",
        "\"ir\": [",
        "\"ir\" : ["
    };
    
    char *data_ir_str = NULL;
    for (int i = 0; i < sizeof(patterns)/sizeof(patterns[0]); i++) {
        data_ir_str = strstr(event_data, patterns[i]);
        if (data_ir_str) {
            data_ir_str += strlen(patterns[i]);
            break;
        }
    }
    
    if (data_ir_str) {
        char *token;
        char *delimiter = ",]";

        token = strtok(data_ir_str, delimiter);
        while (token != NULL && NamiMQTT.DataIRBufferIndex < sizeof(NamiMQTT.DataIRBuffer) / sizeof(NamiMQTT.DataIRBuffer[0])) {
            // Remove leading/trailing whitespace
            char *start = token;
            while (isspace((unsigned char)*start)) start++;
            char *end = start + strlen(start) - 1;
            while (end > start && isspace((unsigned char)*end)) end--;
            *(end + 1) = '\0';

            // Convert the token to an integer and store it
            NamiMQTT.DataIRBuffer[NamiMQTT.DataIRBufferIndex++] = (uint16_t)atoi(start);
            token = strtok(NULL, delimiter);
        }

        if(NamiMQTT.DataIRBuffer[0] == SETTING || NamiMQTT.DataIRBuffer[0] == LATCH){
            for(uint16_t Checkid = 3; Checkid < NamiMQTT.DataIRBufferIndex; Checkid++){
                if(!NamiMQTT.NumCalib[0][0] || !NamiMQTT.NumCalib[1][0]){
                    ERR(NAMI, "Err: No number to compare \r\n");
                    NamiMQTT.DataIRBufferIndex = 0;
                    memset(&NamiMQTT.DataIRBuffer, 0, sizeof(NamiMQTT.DataIRBuffer));
                    submit_gpio_relay_status_to_mqtt_server(ERR_NUM_COMPARE);
                    return;
                }
                if(NamiMQTT.DataIRBuffer[Checkid] > NamiMQTT.NumCalib[0][0] && NamiMQTT.DataIRBuffer[Checkid] < NamiMQTT.NumCalib[0][1]){
                    NamiMQTT.DataIRBuffer[Checkid] = NamiMQTT.DataIRBuffer[Checkid] + NamiMQTT.NumCalib[0][2];
                }else if(NamiMQTT.DataIRBuffer[Checkid] >= NamiMQTT.NumCalib[1][0] && NamiMQTT.DataIRBuffer[Checkid] < NamiMQTT.NumCalib[1][1]){
                    NamiMQTT.DataIRBuffer[Checkid] = NamiMQTT.DataIRBuffer[Checkid] + NamiMQTT.NumCalib[1][2];
                }
                NamiMQTT.DataIRBuffer[Checkid] = NamiMQTT.DataIRBuffer[Checkid] * 2;
            }
        }

        uint32_t IRType = NamiMQTT.DataIRBuffer[2]; 
        uint8_t NumDevice = NamiMQTT.DataIRBuffer[3];
        uint8_t ValueTemp = NamiMQTT.DataIRBuffer[1];
        bool UpdateStatusflag = false;

        if(NamiMQTT.DataIRBuffer[0] == LEARNING){
            uint32_t Count = MQTTIRLearn(ELEARN);
            if(Count >= GENSTRINGSIZE){
                LOGA(NAMI, "Error: over size (%d) of buffer fail \r\n", Count);
                NamiMQTT.LearnCount = 0;
                return;
            }
            LOGA(NAMI, "Size (%d) of string\r\n", NamiMQTT.GenCounter);
            NamiMQTTConvertUintToString(NamiMQTT.GenBuffer, NamiMQTT.GenCounter);
            LearnRspFlag = true;          
        }else if(NamiMQTT.DataIRBuffer[0] == MODIFY){
            if(NamiMQTT.DataIRBuffer[1] == 99){
                NamiMQTT.CalibNumberGetFlag = true;
            }else if(NamiMQTT.DataIRBuffer[1] == 0){
                NamiMQTT.NumCalib[0][0] = NamiMQTT.DataIRBuffer[2];
                NamiMQTT.NumCalib[0][1] = NamiMQTT.DataIRBuffer[3];
                NamiMQTT.NumCalib[0][2] = NamiMQTT.DataIRBuffer[4];
                NamiMQTT.CalibNumberAFlag = true;
            }else if(NamiMQTT.DataIRBuffer[1] == 1){
                NamiMQTT.NumCalib[1][0] = NamiMQTT.DataIRBuffer[2];
                NamiMQTT.NumCalib[1][1] = NamiMQTT.DataIRBuffer[3];
                NamiMQTT.NumCalib[1][2] = NamiMQTT.DataIRBuffer[4];
                NamiMQTT.CalibNumberBFlag = true;             
            }
            LOGA(NAMI, "(LOW:%u, HIGH:%u) Value:%u, (LOW:%u, HIGH:%u) value:%u\r\n", 
                 NamiMQTT.NumCalib[0][0], NamiMQTT.NumCalib[0][1], NamiMQTT.NumCalib[0][2],
                 NamiMQTT.NumCalib[1][0], NamiMQTT.NumCalib[1][1], NamiMQTT.NumCalib[1][2]);
        }else if(NamiMQTT.DataIRBuffer[0] == RANGE_DELAY){
            char tempnumposabuf[25] = "";
            char tempnumposbbuf[25] = "";
            bool RetPosaFlag = false;
            bool RetPosbFlag = false;
            if(NamiMQTT.DataIRBuffer[1] == LEVEL_GET){
                NamiMQTT.RangeDelayFlag = true;
            }else if(NamiMQTT.DataIRBuffer[1] == LEVEL_LOW){
                NamiMQTT.RangeDelayBuffer[LEVEL_LOW][POSA] = NamiMQTT.DataIRBuffer[POSITION_A];
                NamiMQTT.RangeDelayBuffer[LEVEL_LOW][POSB] = NamiMQTT.DataIRBuffer[POSITION_B];
                sprintf(tempnumposabuf, "range_delay_low_posa");
                RetPosaFlag = ef_set_u16(tempnumposabuf, NamiMQTT.RangeDelayBuffer[LEVEL_LOW][POSA]);
                sprintf(tempnumposbbuf, "range_delay_low_posb");
                RetPosbFlag = ef_set_u16(tempnumposbbuf, NamiMQTT.RangeDelayBuffer[LEVEL_LOW][POSB]);
                LOGA(NAMI, "Set LOW Range Delay PosA(%s)=%u, PosB(%s)=%u\r\n", (RetPosaFlag)?("success"):("fail"),NamiMQTT.RangeDelayBuffer[LEVEL_LOW][POSA], (RetPosbFlag)?("success"):("fail"), NamiMQTT.RangeDelayBuffer[LEVEL_LOW][POSB]);
            }else if(NamiMQTT.DataIRBuffer[1] == LEVEL_MEDIUM){
                NamiMQTT.RangeDelayBuffer[LEVEL_MEDIUM][POSA] = NamiMQTT.DataIRBuffer[POSITION_A];
                NamiMQTT.RangeDelayBuffer[LEVEL_MEDIUM][POSB] = NamiMQTT.DataIRBuffer[POSITION_B];
                sprintf(tempnumposabuf, "range_delay_med_posa");
                RetPosaFlag = ef_set_u16(tempnumposabuf, NamiMQTT.RangeDelayBuffer[LEVEL_MEDIUM][POSA]);
                sprintf(tempnumposbbuf, "range_delay_med_posb");
                RetPosbFlag = ef_set_u16(tempnumposbbuf, NamiMQTT.RangeDelayBuffer[LEVEL_MEDIUM][POSB]);
                LOGA(NAMI, "Set MEDIUM Range Delay PosA(%s)=%u, PosB(%s)=%u\r\n", (RetPosaFlag)?("success"):("fail"), NamiMQTT.RangeDelayBuffer[LEVEL_MEDIUM][POSA], (RetPosbFlag)?("success"):("fail"), NamiMQTT.RangeDelayBuffer[LEVEL_MEDIUM][POSB]);
            }else if(NamiMQTT.DataIRBuffer[1] == LEVEL_HIGH){
                NamiMQTT.RangeDelayBuffer[LEVEL_HIGH][POSA] = NamiMQTT.DataIRBuffer[POSITION_A];
                NamiMQTT.RangeDelayBuffer[LEVEL_HIGH][POSB] = NamiMQTT.DataIRBuffer[POSITION_B];
                sprintf(tempnumposabuf, "range_delay_hig_posa");
                RetPosaFlag = ef_set_u16(tempnumposabuf, NamiMQTT.RangeDelayBuffer[LEVEL_HIGH][POSA]);
                sprintf(tempnumposbbuf, "range_delay_hig_posb");
                RetPosbFlag = ef_set_u16(tempnumposbbuf, NamiMQTT.RangeDelayBuffer[LEVEL_HIGH][POSB]);
                LOGA(NAMI, "Set HIGH Range Delay PosA(%s)=%u, PosB(%s)=%u\r\n", (RetPosaFlag)?("success"):("fail"), NamiMQTT.RangeDelayBuffer[LEVEL_HIGH][POSA], (RetPosbFlag)?("success"):("fail"), NamiMQTT.RangeDelayBuffer[LEVEL_HIGH][POSB]);
            }
            LOGA(NAMI, "LOW[%u,%u], MEDIUM[%u,%u], HIGH[%u,%u]\r\n", 
                 NamiMQTT.RangeDelayBuffer[LEVEL_LOW][POSITION_A], NamiMQTT.RangeDelayBuffer[LEVEL_LOW][POSITION_B],
                 NamiMQTT.RangeDelayBuffer[LEVEL_MEDIUM][POSITION_A], NamiMQTT.RangeDelayBuffer[LEVEL_MEDIUM][POSITION_B],
                 NamiMQTT.RangeDelayBuffer[LEVEL_HIGH][POSITION_A], NamiMQTT.RangeDelayBuffer[LEVEL_HIGH][POSITION_B]);
            submit_gpio_relay_status_to_mqtt_server(RANGE_DELAY);
            NamiMQTT.DataIRBufferIndex = 0;
            memset(&NamiMQTT.DataIRBuffer, 0, sizeof(NamiMQTT.DataIRBuffer));
            return;
        }else if(NamiMQTT.DataIRBuffer[0] == GETTING){
            char tempbuf[20]="";
            if(NamiMQTT.DataIRBuffer[1] == NAMI){
                sprintf(&tempbuf, "p nami %s", (NamiMQTT.DataIRBuffer[2] == 1)?("on"):("off"));
            }else if(NamiMQTT.DataIRBuffer[1] == IR){
                sprintf(&tempbuf, "p ir %s", (NamiMQTT.DataIRBuffer[2] == 1)?("on"):("off"));
            }else if(NamiMQTT.DataIRBuffer[1] == OTA){
                sprintf(&tempbuf, "p ota %s", (NamiMQTT.DataIRBuffer[2] == 1)?("on"):("off"));
            }else if(NamiMQTT.DataIRBuffer[1] == WF){
                sprintf(&tempbuf, "p wf %s", (NamiMQTT.DataIRBuffer[2] == 1)?("on"):("off"));
            }else if(NamiMQTT.DataIRBuffer[1] == BLE){
                sprintf(&tempbuf, "p ble %s", (NamiMQTT.DataIRBuffer[2] == 1)?("on"):("off"));
            }else if(NamiMQTT.DataIRBuffer[1] == PULSE){
                sprintf(&tempbuf, "p pulse %s", (NamiMQTT.DataIRBuffer[2] == 1)?("on"):("off"));
            }
            PLOG.plog_parser(tempbuf, 0);
        }else if(NamiMQTT.DataIRBuffer[0] == PARA){
            submit_gpio_relay_status_to_mqtt_server(PARA);
        }else if(NamiMQTT.DataIRBuffer[0] == SETTING || NamiMQTT.DataIRBuffer[0] == LATCH){
            if(NAMI_IR.ActiveFlag){
                NamiMQTT.DataIRBuffer[NamiMQTT.DataIRBufferIndex] = '\0';
                NamiMQTT.DataIRBufferIndex--;
                NamiMQTT.CntFrame = NamiMQTT.NamiMQTTMergeIR(NamiMQTT.DataIRBuffer, NamiMQTT.DataIRBufferIndex);    
            }else{
                LOGA(NAMI, "No Active IR Send \r\n");
            }
            submit_gpio_relay_status_to_mqtt_server(SEND_OK_COUNT_FRAME);                                          
        }

        if(LearnRspFlag) {
            LearnRspFlag = false;
            submit_gpio_relay_status_to_mqtt_server(SEND_LEAR);
        }else if(UpdateStatusflag){
            UpdateStatusflag = false;
            submit_gpio_relay_status_to_mqtt_server(SEND_UPDATE);
        }/*else{
            submit_gpio_relay_status_to_mqtt_server(SEND_OK_COUNT_FRAME);
        }*/

        NamiMQTT.DataIRBufferIndex = 0;
        memset(&NamiMQTT.DataIRBuffer, 0, sizeof(NamiMQTT.DataIRBuffer));
    }
}






void handle_control_switch(const char *event_data) {

    LOGA(NAMI, "event_data: %s\r\n", event_data);

    char status_raw[64] = {0};
    char mode[16] = {0};
    char fan[16] = {0};
    char off[10] = "";
    char reboot[15] = "";
    int temp = -1;

    // --- Lấy chuỗi status ---
    char *status_str = strstr(event_data, "\"status\"");
    if (status_str) {
        status_str = strchr(status_str, ':');
        if (status_str) {
            status_str++;
            while (*status_str == ' ' || *status_str == '\"')
                status_str++;

            // copy tới khi gặp dấu "
            int i = 0;
            while (*status_str != '\"' && *status_str != '\0' && i < sizeof(status_raw)-1) {
                status_raw[i++] = *status_str++;
            }
            status_raw[i] = '\0';
        }
    }

    LOGA(NAMI, "Status raw = %s\r\n", status_raw);

    memset(&NamiMQTT.StatusBuffer, 0x00, sizeof(NamiMQTT.StatusBuffer));
    memcpy((uint8_t *)&NamiMQTT.StatusBuffer[0], (uint8_t *)&status_raw[0], strlen(status_raw));
    LOGA(NAMI, "StatusBuffer = [%s]\r\n", NamiMQTT.StatusBuffer);

    // --- Tách chuỗi status thành 3 phần: mode.fan.temp ---
    char temp_buf[64];
    strncpy(temp_buf, status_raw, sizeof(temp_buf));

    if(!memcmp(&NamiMQTT.StatusBuffer, "off", strlen("off"))){
        memcpy((char *)&off[0], (char *)&NamiMQTT.StatusBuffer[0], strlen((char *)&NamiMQTT.StatusBuffer));
        LOGA(NAMI, "Parsed: mode: %s \r\n", off);

        if (strlen(off) == 0) {
            LOGA(NAMI, "ERROR: Invalid status format off\r\n");
            return;
        }
    }else if(!memcmp(&NamiMQTT.StatusBuffer, "reboot", strlen("reboot"))){
        memcpy((char *)&reboot[0], (char *)&NamiMQTT.StatusBuffer[0], strlen((char *)&NamiMQTT.StatusBuffer));
        LOGA(NAMI, "Parsed: mode: %s \r\n", reboot);

        if (strlen(reboot) == 0) {
            LOGA(NAMI, "ERROR: Invalid status format off\r\n");
            return;
        }
    }else{
        char *p = strtok(temp_buf, ".");
        if (p) strncpy(mode, p, sizeof(mode));

        p = strtok(NULL, ".");
        if (p) strncpy(fan, p, sizeof(fan));

        p = strtok(NULL, ".");
        if (p) temp = atoi(p);
            LOGA(NAMI, "Parsed: mode=%s, fan=%s, temp=%d\r\n", mode, fan, temp);

        if (strlen(mode) == 0 || strlen(fan) == 0 || temp < 0) {
            LOGA(NAMI, "ERROR: Invalid status format\r\n");
            return;
        }
    }

    // --- Logic xử lý IR như code cũ ---
    if (NAMI_IR.ActiveFlag) {

        
        int status_code = 0;
        if(strlen(mode) != 0 && strlen(fan) != 0 && temp != 0){
            status_code = temp;
        }else if(strlen(off) != 0) {
            status_code = 200;    // 0ff
        }else if(strlen(reboot) != 0){
            status_code = 254;    // 0ff
        }

        // submit_gpio_relay_status_to_mqtt_server(SEND_OK);
        
        if(status_code){
            NamiIRRecv.SuspendTask();
            NamiMQTT.NamiMQTTIRControlSend(status_code);
        }else{
            submit_gpio_relay_status_to_mqtt_server(SEND_FAIL);
        }
        

    } else {
        LOGA(NAMI, "No Active IR Send\r\n");
    }
}





// void handle_control_switch(const char *event_data) {
// 	/*
// 	    {"status":"cool.max.17"}
// 	*/
// 	//printf("event_data: %s\r\n",event_data);
//     LOGA(NAMI, "event_data \r\n");
    
// 	// int key_number = -1;
//     int status = -1;

//     //  // Parse key_number
//     // char *key_number_str = strstr(event_data, "\"key_number\":");
//     // if (key_number_str) {
//     //     key_number_str += strlen("\"key_number\":");
//     //     // Bỏ qua khoảng trắng và dấu ngoặc kép nếu có
//     //     while (*key_number_str == ' ' || *key_number_str == '\"') {
//     //         key_number_str++;
//     //     }
//     //     key_number = atoi(key_number_str);
//     // }


//     // Parse status
//     char *status_str = strstr(event_data, "\"status\":");
//     if (status_str) {
//         status_str += strlen("\"status\":");
//         while (*status_str == ' ' || *status_str == '\"') {
//             status_str++;
//         }
//         status = atoi(status_str);
//         LOGA(NAMI, "(%d) - Disable task LED, MQTT... \r\n", status);

//         if(NAMI_IR.ActiveFlag /*1*/){
//             if(status != 0xFE){
//                 submit_gpio_relay_status_to_mqtt_server(SEND_OK);
//                 // //Suspend task
//                 NamiIRRecv.SuspendTask();
//                 // vTaskDelay(pdMS_TO_TICKS(100));
//                 NamiMQTT.NamiMQTTIRControlSend(status);
//             }else if(status == 0xFE){
//                 NamiMQTT.NamiMQTTIRControlSend(status);
//             }else{
//                 LOGA(NAMI, "Error status: %d\r\n", status);
//                 submit_gpio_relay_status_to_mqtt_server(SEND_FAIL);
//                 return;
//             }
//         }else{
//             LOGA(NAMI, "No Active IR Send \r\n");
//         }
//     }else{
//         LOGA(NAMI, "Error: status not found in JSON data\r\n");
//         submit_gpio_relay_status_to_mqtt_server(SEND_FAIL);
//         return;
//     }
// }

void handle_topic(const char *event_topic, const char *event_data) {
    if(*NamiMQTT.StatusLogFlag != NAMI_DISABLE){
        // Compare the topic and take action
        if (strcmp(event_topic, Topic[0]) == 0) { // Control switch
            LOGA(NAMI, "Handle control switch\r\n");
            handle_control_switch(event_data);
        }else if(strcmp(event_topic, Topic[2]) == 0) {
            LOGA(NAMI, "Handle control switch IR\r\n");
            handle_control_ir(event_data);
        }else if (strcmp(event_topic, Topic[3]) == 0) {
            LOGA(NAMI, "Handle firmware update\r\n");
            memcpy((char *)&NamiMQTT.UpdateFWbuff[0], (char *)&event_data[0], strlen(event_data));

            NamiMQTT.UpdateFirmwareFlag = NAMI_ENABLE;
            // AckUpdateStatus = NAMI_ENABLE;

            LOGA(NAMI, "Test update firmware, status(%d)\r\n", NamiMQTT.UpdateFirmwareFlag);
        }
        else {
            LOGA(NAMI, "Unknown topic: %s\r\n", event_topic);
        }
    }else{
        LOGA(NAMI, "Error: Firmware updating no service fail code err: %d\r\n", *NamiMQTT.StatusLogFlag);
    }
}

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        LOGA(NAMI, "Last error %s: 0x%x\r\n", message, error_code);
    }
}

static void mqtt_event_cb(axk_mqtt_event_handle_t event)
{
	LOGA(NAMI, "Start MQTT mqtt_event_cb\r\n");
    int32_t event_id = event->event_id;
   	gclient_mqtt = event->client;
    LOGA(NAMI, "Event dispatched, event_id=%d\r\n", event_id);
    int msg_id;
    
    switch ((axk_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
    	gmqtt_connected = true;
        LOGA(NAMI, "MQTT_EVENT_CONNECTED\r\n");
		msg_id = axk_mqtt_client_subscribe(gclient_mqtt, Topic[0], 0);
		LOGA(NAMI, "Sent subscribe dieukhien successful, msg_id=%d\r\n", msg_id);

        msg_id = axk_mqtt_client_subscribe(gclient_mqtt, Topic[2], 0);
        LOGA(NAMI, "Sent subscribe firmware successful, msg_id=%d\r\n", msg_id);

        msg_id = axk_mqtt_client_subscribe(gclient_mqtt, Topic[3], 0);
        LOGA(NAMI, "Sent subscribe firmware successful, msg_id=%d\r\n", msg_id);

        axk_mqtt_client_publish(gclient_mqtt, Topic[5], LastWill[0], 0, 1, 1); // qos1

        memset(&json_data, 0x00, sizeof(json_data));
        JsonCnt = 0;

        //Publish to config topic
        // cJSON_AddStringToObject(json,"device_type", gdevice_type);
        //{"device_type":"device_ir","firmware_version":"1.0.0-beta","firmware_curr_version":"","ip_address":"192.168.250.2"}
        json_data[JsonCnt++] = '{';
        memcpy(&json_data[JsonCnt], TYPEDEVICEMQTT, strlen(TYPEDEVICEMQTT));
        JsonCnt += strlen(TYPEDEVICEMQTT);
        json_data[JsonCnt++] = ':';
        json_data[JsonCnt++] = '"';
        memcpy(&json_data[JsonCnt], TYPEDEVICE, strlen(TYPEDEVICE));
        JsonCnt += strlen(TYPEDEVICE);
        json_data[JsonCnt++] = '"';
        json_data[JsonCnt++] = ',';
        memcpy(&json_data[JsonCnt], FIRMWAREVERSIONMQTT, strlen(FIRMWAREVERSIONMQTT));
        JsonCnt += strlen(FIRMWAREVERSIONMQTT);
        json_data[JsonCnt++] = ':';
        json_data[JsonCnt++] = '"';
        memcpy(&json_data[JsonCnt], FIRMWAREVERSION, strlen(FIRMWAREVERSION));
        JsonCnt += strlen(FIRMWAREVERSION);
        json_data[JsonCnt++] = '"';

        json_data[JsonCnt++] = ',';
        memcpy(&json_data[JsonCnt], "\"ip_address\"", strlen("\"ip_address\""));
        JsonCnt += strlen("\"ip_address\"");  
        json_data[JsonCnt++] = ':';
        json_data[JsonCnt++] = '"';   
        // Get the IP address from the WiFi interface
        if (g_wifi_sta_is_connected) {
            struct netif *netif = netif_find("st1");
            if (netif != NULL) {
                char ip_str[16];
                snprintf(ip_str, sizeof(ip_str), "%s", ip4addr_ntoa(netif_ip4_addr(netif)));
                memcpy(&json_data[JsonCnt], ip_str, strlen(ip_str));
                JsonCnt += strlen(ip_str); 
                json_data[JsonCnt++] = '"';
            } else {
                memcpy(&json_data[JsonCnt], "0.0.0.0", strlen("0.0.0.0"));
                JsonCnt += strlen("0.0.0.0"); 
                json_data[NamiMQTT.GenCounter++] = '"';
            }
        } else {
            memcpy(&NamiMQTT.GenBuffer[JsonCnt], "0.0.0.0", strlen("0.0.0.0"));
            JsonCnt += strlen("0.0.0.0"); 
            json_data[JsonCnt++] = '"';
        }
        json_data[JsonCnt++] = '}';
        LOGA(NAMI, "GenBuffer={%s}\r\n", json_data);
        axk_mqtt_client_publish(gclient_mqtt, Topic[6], (char *)json_data, 0, 1, 1); // qos1
		
        break;
    case MQTT_EVENT_DISCONNECTED:
    	gmqtt_connected = false;
        NamiMQTT.StatusBroker = NAMI_DISCONNECT;
        LOGA(NAMI, "MQTT_EVENT_DISCONNECTED\r\n");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        NamiMQTT.StatusBroker = NAMI_CONNECTED;
        LOGA(NAMI, "MQTT_EVENT_SUBSCRIBED, msg_id=%d\r\n", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        LOGA(NAMI, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d\r\n", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        LOGA(NAMI, "MQTT_EVENT_PUBLISHED, msg_id=%d\r\n", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
    	LOGA(NAMI, "\r\n");
		LOGA(NAMI, "\r\n");
        LOGA(NAMI, "MQTT_EVENT_DATA\r\n");
        LOGA(NAMI, "TOPIC=%.*s\r\n", event->topic_len, event->topic);
        LOGA(NAMI, "DATA=%.*s\r\n", event->data_len, event->data);
        
        // Create buffers to hold the topic and data strings
		char topic_buffer[100] = {0}; // Adjust size as needed
		char data_buffer[500] = {0}; // Adjust size as needed
	
		// Copy topic and data into buffers (ensure null termination)
		if (event->topic_len < sizeof(topic_buffer)) {
			strncpy(topic_buffer, event->topic, event->topic_len);
			topic_buffer[event->topic_len] = '\0'; // Null-terminate the string
		}
		else {
			LOGA(NAMI, "Topic is too long to fit into the buffer.\r\n");
		}
		
		
		if (event->data_len < sizeof(data_buffer)) {
			strncpy(data_buffer, event->data, event->data_len);
			data_buffer[event->data_len] = '\0'; // Null-terminate the string
			handle_topic(topic_buffer, data_buffer);
		}
		else {
			LOGA(NAMI, "Data is too long to fit into the buffer.\r\n");
		}
		
        break;
    case MQTT_EVENT_ERROR:
        LOGA(NAMI, "MQTT_EVENT_ERROR\r\n");
        // LOGMQTT("MQTT_EVENT_ERROR\r\n");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from axk-tls", event->error_handle->axk_tls_last_axk_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->axk_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->axk_transport_sock_errno);
            LOGA(NAMI, "Last errno string (%s)\r\n", strerror(event->error_handle->axk_transport_sock_errno));
        }
        break;
    case MQTT_EVENT_BEFORE_CONNECT:
        LOGA(NAMI, "MQTT_EVENT_BEFORE_CONNECT\r\n");
        break;
    default:
        LOGA(NAMI, "Other event id:%d\r\n", event_id);
        break;
    }
}

void mqtt_start(void *pvParameters)
{
    memset(&NamiMQTT, 0x00, sizeof(NamiMQTT));
    NamiMQTT.NamiMQTTIRControlSend                   = &NamiIRControlSend;
    // NamiMQTT.NamiMQTTSuspendIR                       = &NamiSuspendIR;
    NamiMQTT.NamiMQTTMergeIR                         = &NamiMergeIR;
    // NamiMQTT.NamiMQTTControlTask                     = &NamiIRControlTask;
    NamiMQTT.StatusLogFlag                           = &NAMI_IR.UpdateLogFlag;
    NamiMQTT.UpdateFWbuff                            = &NAMI_IR.NamiSystem.EasyFlashBuff;
    NamiMQTT.StatusBroker                            = NAMI_DISCONNECT;
    NamiMQTT.IR_Init                                 = &NamiIR_Init;
    NamiMQTT.IR_LearnToReceive                       = &NMIR_LearnToReceive;
            
    NamiMQTT.ReSendCalibNumberGetFlag                = true;
    NamiMQTT.RangeDelayBuffer[LEVEL_LOW][POSA]       = 1;
    NamiMQTT.RangeDelayBuffer[LEVEL_LOW][POSB]       = 1000;
    NamiMQTT.RangeDelayBuffer[LEVEL_MEDIUM][POSA]    = 1000;
    NamiMQTT.RangeDelayBuffer[LEVEL_MEDIUM][POSB]    = 3500;
    NamiMQTT.RangeDelayBuffer[LEVEL_HIGH][POSA]      = 10000;
    NamiMQTT.RangeDelayBuffer[LEVEL_HIGH][POSB]      = 14000;

    LOGA(NAMI, "topic gmac \r\n");
    LOGA(NAMI, "gmac: %s\r\n", gmac);
    
    uint8_t mac[6];
    
    bl_wifi_mac_addr_get(mac);
    snprintf(NamiMQTT.mac_str, sizeof(NamiMQTT.mac_str), "%02X%02X%02X%02X%02X%02X", 
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

// static const char *Topic[8] = {
//     "device/command/",
//     "device/command/",
//     "device/command/",
//     "device/ota/",
//     "device/ota/",
//     "device/availability/",
//     "device/config/"
//     "device/env/"
// };
 
    asprintf(&Topic[0], "%s%s/%s/req", Topic[0], TYPEDEVICE, NamiMQTT.mac_str);
    asprintf(&Topic[1], "%s%s/%s/ack", Topic[1], TYPEDEVICE, NamiMQTT.mac_str);
    asprintf(&Topic[2], "%s%s/%s/ir" , Topic[2], TYPEDEVICE, NamiMQTT.mac_str);
    asprintf(&Topic[3], "%s%s/%s/req", Topic[3], TYPEDEVICE, NamiMQTT.mac_str);
    asprintf(&Topic[4], "%s%s/%s/ack", Topic[4], TYPEDEVICE, NamiMQTT.mac_str);
    asprintf(&Topic[5], "%s%s/%s"    , Topic[5], TYPEDEVICE, NamiMQTT.mac_str);
    asprintf(&Topic[6], "%s%s/%s"    , Topic[6], TYPEDEVICE, NamiMQTT.mac_str);
    asprintf(&Topic[7], "%s%s/%s"    , Topic[7], TYPEDEVICE, NamiMQTT.mac_str);
    
    
    snprintf(gdevice_id, sizeof(gdevice_id), "%02X%02X", mac[4], mac[5]);

    for(uint8_t Idx = 0; Idx < sizeof(Topic)/sizeof(Topic[0]); Idx++){
        LOGA(NAMI, "Topic[%d]={%s}\r\n"      , Idx, Topic[Idx]);
    }



    LOGA(NAMI, "gdevice_id: %s\r\n"      , gdevice_id);

    mbedtls_x509_crt cacert;
	int ret;
	
	mbedtls_x509_crt_init(&cacert);
	
    ret = mbedtls_x509_crt_parse(&cacert,
                                 (const unsigned char *)M_CERTIFICATE,
                                 strlen((char *)M_CERTIFICATE) + 1);       
    if (ret < 0)
    {
        LOGA(NAMI, "mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
            // LOGMQTT("mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
        vTaskDelete(NULL);
        return;
    }

    LOGA(NAMI, "Loading the CA root certificate...\r\n");
    // LOGMQTT("Loading the CA root certificate...\r\n");

	axk_mqtt_client_config_t mqtt_cfg = {
        .uri = "mqtt://mqtt.namismart.vn:1883", // tcp
        .lwt_topic = Topic[5],
        .lwt_msg = LastWill[1],
        .lwt_retain = 1,
        .event_handle = mqtt_event_cb,
        .username = "namismart",
        .password = "Q7bjvdqxCzXb7Cy",
        .keepalive = 25
    };
    axk_mqtt_client_handle_t client = axk_mqtt_client_init(&mqtt_cfg);
    
    LOGA(NAMI, "Start MQTT task\r\n");
    bool mqtt_started = false;
    static uint32_t MQTTCount = 0;

    while (1) {
        // ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // if(gmqtt_connected /*&& NamiMQTT.StateLearnFlag*/){
        //     // NamiMQTT.StateLearnFlag = false;

        //     uint32_t Count;
        //     uint16_t CountString;
        //     // {"ir" :[4, 0, 0, 0]}

        //     Count = MQTTIRLearn();
        //     if(Count >= GENSTRINGSIZE){
        //         LOGA(NAMI, "Error: over size (%d) of buffer fail \r\n", Count);
        //         // memset(&NamiMQTT.LearnBuffer, 0x0, sizeof(NamiMQTT.LearnBuffer));
        //         NamiMQTT.LearnCount = 0;
        //         return;
        //     }else{
        //         // LOGA(NAMI, "Size (%d)/(%d) of buffer = [", Count, Count/2);
        //         // for(uint32_t CopyIndex = 0; CopyIndex < Count; CopyIndex++){
        //         //     printf("%04d", NamiMQTT.GenBuffer[CopyIndex]);
        //         //     if(CopyIndex < Count - 1){
        //         //         printf(", ");
        //         //     }
        //         // }
        //         // printf("]\r\n");
        //     }

        //     LOGA(NAMI, "Size (%d) of string\r\n", NamiMQTT.GenCounter);


        //     NamiMQTTConvertUintToString(NamiMQTT.GenBuffer, NamiMQTT.GenCounter);
          
        //     // LearnRspFlag = true; 
        //     // submit_gpio_relay_status_to_mqtt_server(SEND_LEAR);
        // }

        if (g_wifi_sta_is_connected == 1 && !mqtt_started) {
            axk_mqtt_client_start(client);
            // Start MQTT client if Wi-Fi is connected and MQTT is not started
            // LOGA(NAMI, "start status = %d \r\n", axk_mqtt_client_start(client));
            mqtt_started = true;
            LOGA(NAMI, "MQTT client started\r\n");

            printf("\r\n"
                    "\033[0;32m*************************************************** \r\n"
                    "\033[0;36m       MAC_ADDRESS: \033[1;31m%s                    \r\n"
                    "\033[0;32m*************************************************** \r\n\033[0;37m"
                    "\033[38;5;15m \033[0m\n\n", NamiMQTT.mac_str);
        }else if (g_wifi_sta_is_connected == 0 && mqtt_started) {
            // Stop MQTT client if Wi-Fi is disconnected and MQTT is started
            axk_mqtt_client_stop(client);
            mqtt_started = false;
            LOGA(NAMI, "MQTT client stopped\r\n");
        }

        // Delay to prevent CPU overuse
        vTaskDelay(pdMS_TO_TICKS(800));  // 1-second delay
    }
    // Clean up if task is deleted (should rarely reach here)
    axk_mqtt_client_destroy(client);
	vTaskDelete(NULL);
}
