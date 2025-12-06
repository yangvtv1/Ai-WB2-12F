
#include "mqtt.h"




nami_mqtt_t NamiMQTT;
TaskHandle_t mqtt_task_handle;
axk_mqtt_client_handle_t client;

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
    "device/state/"
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
    // Handle OTA progress case first
    if (OtaUDF.UpdatePercent) {
        snprintf(json_data, sizeof(json_data), "{\"OTA_progress\":\"%d%%\"}", OtaUDF.UpdatePercent);
    } else {
        // Handle all other status cases
        switch (status) {
            case CHAIN_ZERO:
                snprintf(json_data, sizeof(json_data), "{\"status\":\"Re-check buffer IR, cuz have index = 0\"}");
                break;
            case NO_BEST_DIV:
                snprintf(json_data, sizeof(json_data), "{\"status\":\"No find out best div with chain\"}");
                break;
            case NOISE_BUTTON:
                snprintf(json_data, sizeof(json_data), "{\"status\":\"Err: NOISE - Duration=%d/ count=%d\"}", 
                         NamiSwitch.Duration, NamiSwitch.SetCount);
                break;
            case NORMAL:
                snprintf(json_data, sizeof(json_data), "{\"status\":\"normal_reboot\"}");
                break;
            case NEWVER:
                snprintf(json_data, sizeof(json_data), "{\"status\":\"Update_firmware\", \"version_old\":\"%s\", \"version_current\":\"%s\"}", 
                         NamiMQTT.CheckVer, FIRMWAREVERSION);
                break;
            case UDFSUCCESS:
                snprintf(json_data, sizeof(json_data), "{\"OTA_status\":\"Success\"}");
                break;
            case UDFCOMPLETE:
                snprintf(json_data, sizeof(json_data), "{\"OTA_status\":\"Completed\"}");
                break;
            case SETVER:
                snprintf(json_data, sizeof(json_data), "{\"status\":\"set_firmware_begin\"}");
                break;
            case ERR_NUM_COMPARE:
                snprintf(json_data, sizeof(json_data), "{\"status\":\"Err: No number to compare\"}");
                break;
            case MAC_ADD:
                snprintf(json_data, sizeof(json_data), "{\"MAC_ADDRESS\":\"%s\"}", NamiMQTT.mac_str);
                break;
            case SEND_OK_COUNT_FRAME:
                snprintf(json_data, sizeof(json_data), "{\"status\":\"Send successfully frame %d \"}", NamiMQTT.CntFrame);
                break;
            case PIR_MOTION:
                Gen.PercentLight = ((VOLTAGE_DEFAULT - Gen.ResultData) * 100)/VOLTAGE_DEFAULT;
                snprintf(json_data, sizeof(json_data), "{\"status\":\"1\",\"value\":\"%u\"}", Gen.PercentLight);
                break;
            case PIR_NORMAL:
                Gen.PercentLight = ((VOLTAGE_DEFAULT - Gen.ResultData) * 100)/VOLTAGE_DEFAULT;
                snprintf(json_data, sizeof(json_data), "{\"status\":\" No detect anything with level %u\"}", Gen.PercentLight);
                break;
            case PRESENCE:
                Gen.PercentLight = ((VOLTAGE_DEFAULT - Gen.ResultData) * 100)/VOLTAGE_DEFAULT;
                snprintf(json_data, sizeof(json_data), "{\"status\":\"2\",\"value\":\"%u\"}", Gen.PercentLight);
                break;
            case NO_DETECT_PRESENCE:
                Gen.PercentLight = ((VOLTAGE_DEFAULT - Gen.ResultData) * 100)/VOLTAGE_DEFAULT;
                snprintf(json_data, sizeof(json_data), "{\"status\":\"0\",\"value\":\"%u\"}", Gen.PercentLight);
                break;
            case PRESENCE_TRIGGER_FACTOR_1:
                snprintf(json_data, sizeof(json_data), "{\"messege\":\"TrigF0: %.2f, TrigF1: %.2f\"}", Gen.RawSettingFactorOfficial[0], Gen.RawSettingFactorOfficial[1]);
                break;
            case PRESENCE_TRIGGER_FACTOR_2:
                snprintf(json_data, sizeof(json_data), "{\"messege\":\"TrigF2: %.2f, TrigF3: %.2f, TrigF4: %.2f, TrigF5: %.2f, TrigF6: %.2f\"}", Gen.RawSettingFactorOfficial[2], Gen.RawSettingFactorOfficial[3]
                                                                                                                                               , Gen.RawSettingFactorOfficial[4], Gen.RawSettingFactorOfficial[5]
                                                                                                                                               , Gen.RawSettingFactorOfficial[6]);
                break;
            case PRESENCE_TRIGGER_FACTOR_3:
                snprintf(json_data, sizeof(json_data), "{\"messege\":\"TrigF7: %.2f, TrigF8: %.2f, TrigF9: %.2f, TrigF10: %.2f, TrigF11: %.2f\"}", Gen.RawSettingFactorOfficial[7], Gen.RawSettingFactorOfficial[8]
                                                                                                                                                 , Gen.RawSettingFactorOfficial[9], Gen.RawSettingFactorOfficial[10]
                                                                                                                                                 , Gen.RawSettingFactorOfficial[11]);                
                break;
            case PRESENCE_TRIGGER_FACTOR_4:
                snprintf(json_data, sizeof(json_data), "{\"messege\":\"TrigF12: %.2f, TrigF13: %.2f, TrigF14: %.2f, TrigF15: %.2f, TrigF16: %.2f\"}", Gen.RawSettingFactorOfficial[12], Gen.RawSettingFactorOfficial[13]
                                                                                                                                                    , Gen.RawSettingFactorOfficial[14], Gen.RawSettingFactorOfficial[15]
                                                                                                                                                    , Gen.RawSettingFactorOfficial[16]);
                break;
            case PRESENCE_MOTION_FACTOR_1:
                snprintf(json_data, sizeof(json_data), "{\"messege\":\"MotF0: %.2f, MotF1: %.2f, MotF2: %.2f, MotF3: %.2f, MotF4: %.2f\"}", Gen.RawSettingFactorOfficial[0], Gen.RawSettingFactorOfficial[1]
                                                                                                                                               , Gen.RawSettingFactorOfficial[2], Gen.RawSettingFactorOfficial[3]
                                                                                                                                               , Gen.RawSettingFactorOfficial[4]);
                break;
            case PRESENCE_MOTION_FACTOR_2:
                snprintf(json_data, sizeof(json_data), "{\"messege\":\"MotF5: %.2f, MotF6: %.2f, MotF7: %.2f, MotF8: %.2f, MotF9: %.2f\"}", Gen.RawSettingFactorOfficial[5], Gen.RawSettingFactorOfficial[6]
                                                                                                                                               , Gen.RawSettingFactorOfficial[7], Gen.RawSettingFactorOfficial[8]
                                                                                                                                               , Gen.RawSettingFactorOfficial[9]);
                break;
            case PRESENCE_MOTION_FACTOR_3:
                snprintf(json_data, sizeof(json_data), "{\"messege\":\"MotF10: %.2f, MotF11: %.2f, MotF12: %.2f, MotF13: %.2f, MotF14: %.2f\"}", Gen.RawSettingFactorOfficial[10], Gen.RawSettingFactorOfficial[11]
                                                                                                                                                 , Gen.RawSettingFactorOfficial[12], Gen.RawSettingFactorOfficial[13]
                                                                                                                                                 , Gen.RawSettingFactorOfficial[14]);                
                break;
            case PRESENCE_MOTION_FACTOR_4:
                snprintf(json_data, sizeof(json_data), "{\"messege\":\"MotF15: %.2f\"}", Gen.RawSettingFactorOfficial[15]);
                break;
            case QUERY_PROGRESS:
                snprintf(json_data, sizeof(json_data), "{\"messege\":\"ACK:%u, Percent:%u\"}", Gen.UartReceiveAck, Gen.UartReceivePercent);
                break;
            case OPEN_CLIB:
                snprintf(json_data, sizeof(json_data), "{\"messege\":\"Open mode calibration sensor\"}");
                break;
            case AUTO_THRESHOLD:
                snprintf(json_data, sizeof(json_data), "{\"messege\":\"trigger:%u, Motion:%u, Micro:%u\"}", Gen.UartMotionTrigger, Gen.UartMotionHoldThreshold, Gen.UartMicroMotionHoldThreshold);   
                break;
            case CLOSE_CLIB:
                snprintf(json_data, sizeof(json_data), "{\"messege\":\"Close mode calibration sensor\"}");
                break;
            case PRESENCE_ACK:
                snprintf(json_data, sizeof(json_data), "{\"messege\":\"Setting parameter success!!\"}");
                break;
            case PRESENCE_FAIL:
                snprintf(json_data, sizeof(json_data), "{\"messege\":\"Err: Setting parameter fail!!\"}");
                break;
            case LOG_NAMI:
                snprintf(json_data, sizeof(json_data), "{\"status\":\" log nami interaction\"}");
                break;
            case LOG_IR:
                snprintf(json_data, sizeof(json_data), "{\"status\":\" log ir interaction\"}");
                break;
            case LOG_WF:
                snprintf(json_data, sizeof(json_data), "{\"status\":\" log wifi interaction\"}");
                break;
            case LOG_BLE:
                snprintf(json_data, sizeof(json_data), "{\"status\":\" log Bluetooth interaction\"}");
                break;
            case LOG_PULSE:
                snprintf(json_data, sizeof(json_data), "{\"status\":\" log pulse interaction\"}");
                break;
            case LOG_OTA:
                snprintf(json_data, sizeof(json_data), "{\"status\":\" log ota interaction\"}");
                break;
            case NUMBER_CALIB_COMPARE:
                snprintf(json_data, sizeof(json_data), 
                         "{\"status\":\"(LOW:%u, HIGH:%u) Value:%u, (LOW:%u, HIGH:%u) value:%u\"}", 
                         NamiMQTT.NumCalib[0][0], NamiMQTT.NumCalib[0][1], NamiMQTT.NumCalib[0][2],
                         NamiMQTT.NumCalib[1][0], NamiMQTT.NumCalib[1][1], NamiMQTT.NumCalib[1][2]);
                break;
            case RANGE_DELAY:
                snprintf(json_data, sizeof(json_data), 
                         "{\"status\":\"LOW[%u,%u], MEDIUM[%u,%u], HIGH[%u,%u]\"}", 
                         NamiMQTT.RangeDelayBuffer[LEVEL_LOW][POSA], NamiMQTT.RangeDelayBuffer[LEVEL_LOW][POSB],
                         NamiMQTT.RangeDelayBuffer[LEVEL_MEDIUM][POSA], NamiMQTT.RangeDelayBuffer[LEVEL_MEDIUM][POSB],
                         NamiMQTT.RangeDelayBuffer[LEVEL_HIGH][POSA], NamiMQTT.RangeDelayBuffer[LEVEL_HIGH][POSB]);
                break;
            default:
                snprintf(json_data, sizeof(json_data), "{\"status\":\"%s\"}", 
                         (status == SEND_OK) ? "Send_successfully" : 
                         ((status == SEND_UPDATE) ? "Update_Firmware" : "Error:_send_fail"));
                break;
        }
    }

    // Publish to MQTT server if connected
    if (!gmqtt_connected) {
        LOGA(NAMI, "MQTT client is not connected.\r\n");
        return;
    }

    // Determine topic and payload based on status
    if (OtaUDF.UpdatePercent || status == UDFSUCCESS || status == UDFCOMPLETE || status == NEWVER) {
        LOGA(NAMI, "Published:{%s} \r\n", Topic[4]);
        axk_mqtt_client_publish(gclient_mqtt, Topic[4], json_data, 0, 0, 0);
    }else if(status == NO_DETECT_PRESENCE || status ==  PRESENCE || status == PIR_MOTION) {
        LOGA(NAMI, "%s - %s \r\n", Topic[7], json_data);
        axk_mqtt_client_publish(gclient_mqtt, Topic[7], json_data, 0, 0, 0);
    } else if (status != SEND_LEAR) {
        axk_mqtt_client_publish(gclient_mqtt, Topic[1], json_data, 0, 0, 0);
    } else {
        LOGA(NAMI, "Published:{%s} \r\n", Topic[1]);
        axk_mqtt_client_publish(gclient_mqtt, Topic[1], (char *)&NamiMQTT.LearnBuffer, 0, 0, 0);
    }
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

void handle_control_pir(const char *event_data) {
    uint16_t ValueDouble;
    LOGA(NAMI, "Handle control switch PIR\r\n");
    
    // Tìm vị trí bắt đầu của chuỗi "ir" với các khoảng trắng khác nhau
    const char *patterns[] = {
        PROC_SUB_PROC_1,
        PROC_SUB_PROC_2,
        PROC_SUB_PROC_3,
        PROC_SUB_PROC_4
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
            NamiMQTT.DataIRBuffer[NamiMQTT.DataIRBufferIndex++] = (uint8_t)atoi(start);
            token = strtok(NULL, delimiter);
        }

        if(NamiMQTT.DataIRBuffer[0] == GETTING){
            char tempbuf[20]="";
            if(NamiMQTT.DataIRBuffer[1] == NAMI){
                sprintf(&tempbuf, "p nami %s", (NamiMQTT.DataIRBuffer[2] == 1)?("on"):("off"));
                submit_gpio_relay_status_to_mqtt_server(LOG_NAMI); 
            }else if(NamiMQTT.DataIRBuffer[1] == IR){
                sprintf(&tempbuf, "p ir %s", (NamiMQTT.DataIRBuffer[2] == 1)?("on"):("off"));
                submit_gpio_relay_status_to_mqtt_server(LOG_IR);
            }else if(NamiMQTT.DataIRBuffer[1] == OTA){
                sprintf(&tempbuf, "p ota %s", (NamiMQTT.DataIRBuffer[2] == 1)?("on"):("off"));
                submit_gpio_relay_status_to_mqtt_server(LOG_OTA);
            }else if(NamiMQTT.DataIRBuffer[1] == WF){
                sprintf(&tempbuf, "p wf %s", (NamiMQTT.DataIRBuffer[2] == 1)?("on"):("off"));
                submit_gpio_relay_status_to_mqtt_server(LOG_WF);
            }else if(NamiMQTT.DataIRBuffer[1] == BLE){
                sprintf(&tempbuf, "p ble %s", (NamiMQTT.DataIRBuffer[2] == 1)?("on"):("off"));
                submit_gpio_relay_status_to_mqtt_server(LOG_BLE);
            }else if(NamiMQTT.DataIRBuffer[1] == PULSE){
                sprintf(&tempbuf, "p pulse %s", (NamiMQTT.DataIRBuffer[2] == 1)?("on"):("off"));
                submit_gpio_relay_status_to_mqtt_server(LOG_PULSE);
            }
            PLOG.plog_parser(tempbuf, 0); 
        }else if(NamiMQTT.DataIRBuffer[0] == GET_VAL_PRE){
            // {"pre" :[29,253,252,251,250,12,00,08,00,01,00,04,00,05,00,16,00,17,00,04,03,02,01]}
            NamiMQTT.DataIRBuffer[NamiMQTT.DataIRBufferIndex] = '\0';
            NamiMQTT.DataIRBufferIndex--;
            NamiMQTT.NamiMQTTProcPresenceMQTT2way(NamiMQTT.DataIRBuffer, NamiMQTT.DataIRBufferIndex);
        }

        NamiMQTT.DataIRBufferIndex = 0;
        memset(&NamiMQTT.DataIRBuffer, 0, sizeof(NamiMQTT.DataIRBuffer));
    }
}

void handle_topic(const char *event_topic, const char *event_data) {
    // Compare the topic and take action
    if (strcmp(event_topic, Topic[0]) == 0) { // Control switch
        LOGA(NAMI, "Handle control switch\r\n");
        // handle_control_switch(event_data);
    }else if(strcmp(event_topic, Topic[2]) == 0) {
        LOGA(NAMI, "Handle control switch PIR\r\n");
        handle_control_pir(event_data);
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
}

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        LOGA(NAMI, "Last error %s: 0x%x\r\n", message, error_code);
    }
}

static void mqtt_event_cb(axk_mqtt_event_handle_t event)
{
    // xSemaphoreGive(xIRSemaphore);

	LOGA(NAMI, "Start MQTT mqtt_event_cb\r\n");
    int32_t event_id = event->event_id;
   	gclient_mqtt = event->client;
    LOGA(NAMI, "Event dispatched, event_id=%d\r\n", event_id);
    int msg_id;
    
    switch ((axk_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
    	gmqtt_connected = true;
        NamiMQTT.StatusConnectFlag = true;

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
        NamiMQTT.ConfigBeginFlag = false;
		
        break;
    case MQTT_EVENT_DISCONNECTED:
    	gmqtt_connected = false;
        NamiMQTT.StatusConnectFlag = false;
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

void MQTTDestroy(void)
{
    if (client != NULL) {
        axk_mqtt_client_destroy(client);
        LOGA(NAMI, "MQTT client destroyed\r\n");
    }
}

void MQTTStart(void){
    if (client != NULL) {
        axk_mqtt_client_start(client);
        LOGA(NAMI, "MQTT client start \r\n");
    }
}

void mqtt_start(void *pvParameters)
{
    memset(&NamiMQTT, 0x00, sizeof(NamiMQTT));
    // NamiMQTT.NamiMQTTIRControlSend                   = &NamiIRControlSend;
    // NamiMQTT.NamiMQTTSuspendIR                       = &NamiSuspendIR;
    // NamiMQTT.NamiMQTTMergeIR                         = &NamiMergeIR;
    // NamiMQTT.NamiMQTTControlTask                     = &NamiIRControlTask;
    // NamiMQTT.StatusLogFlag                           = &NAMI_IR.UpdateLogFlag;
    NamiMQTT.UpdateFWbuff                            = &Gen.UpdateFirmwareBuf;
    NamiMQTT.StatusBroker                            = NAMI_DISCONNECT;
    NamiMQTT.NamiMQTTProcPresenceMQTT2way            = &ProcPresenceMQTT2way;
    NamiMQTT.ConfigBeginFlag                         = true;
    // NamiMQTT.IR_Init                                 = &NamiIR_Init;
    // NamiMQTT.IR_LearnToReceive                       = &NMIR_LearnToReceive;
            
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

// static const char *Topic[7] = {
//     "device/command/",
//     "device/command/",
//     "device/command/",
//     "device/ota/",
//     "device/ota/",
//     "device/availability/",
//     "device/config/"
// };
 
    asprintf(&Topic[0], "%s%s/%s/req", Topic[0], TYPEDEVICE, NamiMQTT.mac_str);
    asprintf(&Topic[1], "%s%s/%s/ack", Topic[1], TYPEDEVICE, NamiMQTT.mac_str);
    asprintf(&Topic[2], "%s%s/%s/%s" , Topic[2], TYPEDEVICE, NamiMQTT.mac_str, TYPEDEVICE);
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
        .keepalive = 9
    };
    /*axk_mqtt_client_handle_t*/ client = axk_mqtt_client_init(&mqtt_cfg);
    
    
    // LOGA(NAMI, "Start MQTT task\r\n");
    // bool mqtt_started = false;
    // static uint32_t MQTTCount = 0;

    // while (1) {
    //     ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    //     if (g_wifi_sta_is_connected == 1 && !mqtt_started) {
    //         axk_mqtt_client_start(client);
    //         // Start MQTT client if Wi-Fi is connected and MQTT is not started
    //         // LOGA(NAMI, "start status = %d \r\n", axk_mqtt_client_start(client));
    //         mqtt_started = true;
    //         LOGA(NAMI, "MQTT client started\r\n");

    //         printf("\r\n"
    //                 "\033[0;32m*************************************************** \r\n"
    //                 "\033[0;36m       MAC_ADDRESS: \033[1;31m%s                    \r\n"
    //                 "\033[0;32m*************************************************** \r\n\033[0;37m"
    //                 "\033[38;5;15m \033[0m\n\n", NamiMQTT.mac_str);
    //     }else if (g_wifi_sta_is_connected == 0 && mqtt_started) {
    //         // Stop MQTT client if Wi-Fi is disconnected and MQTT is started
    //         axk_mqtt_client_stop(client);
    //         mqtt_started = false;
    //         LOGA(NAMI, "MQTT client stopped\r\n");
    //     }

    //     // Delay to prevent CPU overuse
    //     vTaskDelay(pdMS_TO_TICKS(800));  // 1-second delay
    // }
    // // Clean up if task is deleted (should rarely reach here)
    // axk_mqtt_client_destroy(client);
	vTaskDelete(NULL);
}
