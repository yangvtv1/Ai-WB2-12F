
#include "switch.h"


TaskHandle_t setting_mode_task_handle = NULL;
TaskHandle_t relay_event_task_handle;
bool setting_mode_enable = false;
bool setting_mode_flag = true; // Flag to track if setting mode is enabled

uint8_t gpio_status_relay3;
uint8_t gpio_status_relay4;
uint8_t gpio_status_relay5;
uint8_t gpio_status_relay6;
uint8_t gled_conf_status;
char ip_str[16] = {0};

static hosal_gpio_dev_t isr_key1,isr_key2,isr_key3,isr_key4;


typedef struct {
    uint8_t gpioPin;
    bool interrupt_value;  // You can add more fields as needed
    uint16_t duration;
} gpio_event_t;

 static gpio_event_t gevent_gpio;
xQueueHandle gpio_event_queue;  // Handle for the GPIO event queue


switch_t NamiSwitch;


TickType_t LocalCurrentTime = NULL;
TickType_t LocalLEDTime = NULL;
uint32_t TimerCounter;
bool StartCountFlag = false;
uint8_t DesiredGpioState = 0;
uint8_t PreviousGpioState = 0;
bool ChangeLEDFlag = false;
bool PermisSpamCmdFlag = false;
bool PermisSpamCmdRstFlag = false;
uint32_t LedExitCalibCnt;
bool LedExitCalibFlag;

static void NamiSwitchProc(xTimerHandle pxTimer)
{
    if(NamiSwitch.TimerStartFlag){
        NamiSwitch.TimerStartFlag = false;
        LocalCurrentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
        StartCountFlag = true;
        PermisSpamCmdFlag = true;
        PermisSpamCmdRstFlag = true;
        DesiredGpioState = 1;
        LOGA(NAMI, "start timer switch (%u)\r\n", LocalCurrentTime);
    }

    if(NamiSwitch.TimerStopFlag){
        NamiSwitch.TimerStopFlag = false;
        LOGA(NAMI, "stop timer switch \r\n");
        NamiSwitch.ChangeCalibFlag = false;
        NamiSwitch.ReSetFlag = false;
        StartCountFlag = false;
    }

    if(StartCountFlag){
        if((xTaskGetTickCount() * portTICK_PERIOD_MS) - LocalCurrentTime >= 7500){
            if(PermisSpamCmdRstFlag){
                PermisSpamCmdRstFlag = false;
                LOGA(IR, "Time is current press button is = %u/%u\r\n", (xTaskGetTickCount() * portTICK_PERIOD_MS), (xTaskGetTickCount() * portTICK_PERIOD_MS) - LocalCurrentTime);
                NamiSwitch.ReSetFlag = true;
                NamiSwitch.ChangeCalibFlag = false;
                LocalLEDTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
            } 
        }else if(((xTaskGetTickCount() * portTICK_PERIOD_MS) - LocalCurrentTime >= 3000) && ((xTaskGetTickCount() * portTICK_PERIOD_MS) - LocalCurrentTime < 7499)){     // 5s
            if(PermisSpamCmdFlag){
                PermisSpamCmdFlag = false;
                LOGA(IR, "Time is current press button is = %u/%u\r\n", (xTaskGetTickCount() * portTICK_PERIOD_MS), (xTaskGetTickCount() * portTICK_PERIOD_MS) - LocalCurrentTime);
                NamiSwitch.ChangeCalibFlag = true;
                LocalLEDTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
            }
        }
    }

    if(NamiSwitch.ChangeCalibFlag){
        // LOGA(WF, "NamiSwitch.ChangeCalibFlag=%u\r\n", NamiSwitch.ChangeCalibFlag);

        if(NamiSwitch.BreakOutCalibrateFlag == false){
            if(NamiSwitch.ExitCalibrateFlag){
                if(CALCUALATE_TIME(LocalLEDTime) >= 0 && CALCUALATE_TIME(LocalLEDTime) < 200 && LedExitCalibFlag){
                    LedExitCalibFlag = false;
                    LocalLEDTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
                    NAMI_PRE_GPIO_OUTPUT_SET(GPIO_LED_NOTIFY, LED_ON);
                    LedExitCalibCnt++;
                }else if(CALCUALATE_TIME(LocalLEDTime) >= 0 && CALCUALATE_TIME(LocalLEDTime) < 100 && LedExitCalibFlag == false){
                    LedExitCalibFlag = true;
                    LocalLEDTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
                    NAMI_PRE_GPIO_OUTPUT_SET(GPIO_LED_NOTIFY, LED_OFF);                
                }
                if(LedExitCalibCnt > 3){
                    LedExitCalibCnt = 0;
                    LOGA(IR, "Exit calib wifi \r\n");
                    NamiSwitch.ExitCalibrateFlag = false;
                    NamiSwitch.BreakOutCalibrateFlag = true;
                }
            }else if((xTaskGetTickCount() * portTICK_PERIOD_MS) - LocalLEDTime >= 300){             // 100ms
                if(ChangeLEDFlag){
                    ChangeLEDFlag = false;
                    LocalLEDTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
                    NAMI_PRE_GPIO_OUTPUT_SET(GPIO_LED_NOTIFY, LED_ON);
                }else {
                    ChangeLEDFlag = true;
                    LocalLEDTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
                    NAMI_PRE_GPIO_OUTPUT_SET(GPIO_LED_NOTIFY, LED_OFF);
                }  
            }
        }
    }

    if(NamiSwitch.ReSetFlag){
        // LOGA(WF, "NamiSwitch.ReSetFlag=%u\r\n", NamiSwitch.ReSetFlag);
        if((xTaskGetTickCount() * portTICK_PERIOD_MS) - LocalLEDTime >= 50){             // 100ms
            if(ChangeLEDFlag){
                ChangeLEDFlag = false;
                LocalLEDTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
                NAMI_PRE_GPIO_OUTPUT_SET(GPIO_LED_NOTIFY, LED_ON);
            }else {
                ChangeLEDFlag = true;
                LocalLEDTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
                NAMI_PRE_GPIO_OUTPUT_SET(GPIO_LED_NOTIFY, LED_OFF);
            }  
        }       
    }
}


void initialize_switch() {
    NAMI_PRE_GPIO_ENABLE_OUTPUT(GPIO_LED_NOTIFY ,0, 0);
    NAMI_PRE_GPIO_OUTPUT_SET(GPIO_LED_NOTIFY, 1);
    //enable input and output
    NAMI_PRE_GPIO_ENABLE_INPUT(GPIO_KEY_IN0, 1, 0);
    
    // initialize interrupt event queue
    NAMI_PRE_GPIO_IRQ_MASK(&isr_key1, 1); // Mask the interrupt

    gpio_irq_init(); 
    NAMI_PRE_GPIO_IRQ_MASK(&isr_key1, 0); // Unmask the interrupt

    // In your main or init function
    gpio_event_queue = xQueueCreate(25, sizeof(gpio_event_t));

    NamiSwitch.TimerSwitchHandler = xTimerCreate("NamiSwitchProc", pdMS_TO_TICKS(PDS_WAKEUP_MS), pdTRUE, NULL, NamiSwitchProc);
    xTimerStart(NamiSwitch.TimerSwitchHandler, 0);
}
void LedNotifyStartPro(void){
    NAMI_PRE_GPIO_OUTPUT_SET(GPIO_LED_NOTIFY, LED_ON);
    vTaskDelay(pdMS_TO_TICKS(100));
    NAMI_PRE_GPIO_OUTPUT_SET(GPIO_LED_NOTIFY, LED_OFF);
    vTaskDelay(pdMS_TO_TICKS(100));
    NAMI_PRE_GPIO_OUTPUT_SET(GPIO_LED_NOTIFY, LED_ON);
    vTaskDelay(pdMS_TO_TICKS(100));
    NAMI_PRE_GPIO_OUTPUT_SET(GPIO_LED_NOTIFY, LED_OFF);
    vTaskDelay(pdMS_TO_TICKS(100));
}
void gpio_irq_init()
{
    // // GPIO_KEY_IN0
    isr_key1.port = GPIO_KEY_IN0;
    isr_key1.config = INPUT_PULL_UP;
    NAMI_PRE_GPIO_INIT(&isr_key1);
    NAMI_PRE_GPIO_IRQ_SET(&isr_key1, HOSAL_IRQ_TRIG_NEG_PULSE, key1_irq_neg, NULL);
}


// int8_t connecting_to_other_wifi = false;

void ble_connect_wifi (char *received_data){
        cJSON *json = cJSON_Parse(received_data);
        if (json == NULL) {
            cJSON_GetErrorPtr();
            return;
        }

        if (cJSON_GetObjectItem(json, "ssid") == NULL) {
            LOGA(WF, "ssid key not found in JSON\r\n");
            cJSON_Delete(json);
            return;
        }
        // wifi_disconnect();

        cJSON *ssid_item     = cJSON_GetObjectItem(json, "ssid");
        cJSON *password_item = cJSON_GetObjectItem(json, "password");
        // cJSON *HostBle       = cJSON_GetObjectItem(json, "host");

        if (ssid_item == NULL || password_item == NULL) {
            LOGA(WF, "ssid or password not found\r\n");
            cJSON_Delete(json);
            return;
        }
        wifi_disconnect();
        char *ssid = ssid_item->valuestring;
        char *password = password_item->valuestring;

        LOGA(WF, "Received Wi-Fi credentials via BLE: SSID: %s, Password: %s, Host: %s\r\n", 
            ssid, password, NamiMQTT.Host);
        if (wifi_sta_connect(ssid, password) == 0) {
            LOGA(WF, "Wi-Fi connection attempt initiated...\r\n");
        }
        cJSON_Delete(json);
}

void NamiFlash(void){

	if(ef_set_str("ssid", g_curr_ssid)){
		LOGA(WF, "Save flash ssid of %s success\r\n", g_curr_ssid);
	}else{
		ERR(WF, "Error: Save flash ssid of %s fail\r\n", g_curr_ssid);
	}

	if(ef_set_str("pass", g_curr_pass)){
		LOGA(WF, "Save flash pass of %s success\r\n", g_curr_pass);
	}else{
		LOGA(WF, "Error: Save flash pass of %s fail\r\n", g_curr_pass);
	}

    if(NamiMQTT.Host != NULL){
        if(ef_set_str("host", NamiMQTT.Host)){
            LOGA(WF, "Save flash host of %s success\r\n", NamiMQTT.Host);
        }else{
            LOGA(WF, "Error: Save flash host of %s fail\r\n", NamiMQTT.Host);
        }
    }
}

void setting_mode_task(void *pvParameters) {
    LOGA(IR, "Entering setting mode: Transmitting Bluetooth for connection...\r\n");
    // vTaskSuspend(led_wifi_task_handle); // Suspend LED WiFi task
    setting_mode_enable = true;
    NamiSwitch.ModeCalibFlag = true;

    // Start Bluetooth in setting mode
    apps_ble_start();
    LOGA(IR, "Bluetooth started in setting mode\r\n");

    //Attempt to connect to wifi and bluetooth
    int desired_interval = 60*5; //Time for waiting in setting mode (second) 60s x 5 = 5m
    int delay_time=500;
    int attempt = (desired_interval*1000)/delay_time; //wifi connection attempt for 60s
    int value = LED_OFF;
    uint8_t ble_notification[120] = "";
    uint16_t BleCount = 0;
    uint16_t BleCount_1 = 0;
    
    while(1){

        if(NamiSwitch.BreakOutCalibrateFlag){
            LOGA(IR, "Exit calib wifi in while \r\n");
            NamiSwitch.ModeCalibFlag = false;
            apps_ble_stop();
            break;
        }

        //blink led
        //blink led
        NAMI_PRE_GPIO_OUTPUT_SET(GPIO_LED_NOTIFY, LED_ON);
        vTaskDelay(pdMS_TO_TICKS(500));
        NAMI_PRE_GPIO_OUTPUT_SET(GPIO_LED_NOTIFY, LED_OFF);
        vTaskDelay(pdMS_TO_TICKS(500));

        LOGA(WF, "Waiting countdown: %ds, ble_connect_status=%s, setting_mode_enable=%s\r\n", (attempt*delay_time)/1000
                                                                                            , (ble_connect_status)?"true":"false"
                                                                                            , setting_mode_enable?"true":"false");

        if((ble_connect_status == true && setting_mode_enable == false) || attempt == 0){
            if(attempt == 0){
                LOGA(WF, "Failed to connect to wifi and bluetooth\n");
            }
            else{
                // {"result":"ok","message":"ok","lan_ip":"192.168.250.5","mac_id":"7C:B9:4C:1D:9B:B3","device_id":"9BB3","device_code":"","name":"Infrared","type":"device_ir"}

                ble_notification[BleCount++] = '{';
                memcpy(&ble_notification[BleCount], "\"result\"", strlen("\"result\""));
                BleCount += strlen("\"result\"");
                ble_notification[BleCount++] = ':';
                if(g_wifi_sta_is_connected){
                    memcpy(&ble_notification[BleCount], "\"ok\"", strlen("\"ok\""));
                    BleCount += strlen("\"ok\"");
                }else{
                    memcpy(&ble_notification[BleCount], "\"error\"", strlen("\"error\""));
                    BleCount += strlen("\"error\"");
                }

                ble_notification[BleCount++] = ',';
                memcpy(&ble_notification[BleCount], "\"message\"", strlen("\"message\""));
                BleCount += strlen("\"message\"");
                ble_notification[BleCount++] = ':';
                memcpy(&ble_notification[BleCount], "\"ok\"", strlen("\"ok\""));
                BleCount += strlen("\"ok\"");

                ble_notification[BleCount++] = ',';
                memcpy(&ble_notification[BleCount], "\"lan_ip\"", strlen("\"lan_ip\""));
                BleCount += strlen("\"lan_ip\"");
                ble_notification[BleCount++] = ':';
                if (g_wifi_sta_is_connected) {
                    struct netif *netif = netif_find("st1");
                    if (netif != NULL) {
                        snprintf(ip_str, sizeof(ip_str), "%s", ip4addr_ntoa(netif_ip4_addr(netif)));
                        ble_notification[BleCount++] = '"';
                        memcpy(&ble_notification[BleCount], ip_str, strlen(ip_str));
                        BleCount += strlen(ip_str);
                        ble_notification[BleCount++] = '"';
                    } else {
                        memcpy(&ble_notification[BleCount], "\"0.0.0.0\"", strlen("\"0.0.0.0\""));
                        BleCount += strlen("\"0.0.0.0\"");
                    }
                } else {
                    memcpy(&ble_notification[BleCount], "\"0.0.0.0\"", strlen("\"0.0.0.0\""));
                    BleCount += strlen("\"0.0.0.0\"");
                }

                ble_notification[BleCount++] = ',';
                memcpy(&ble_notification[BleCount], "\"mac_id\"", strlen("\"mac_id\""));
                BleCount += strlen("\"mac_id\"");
                ble_notification[BleCount++] = ':';
                ble_notification[BleCount++] = '"';
                ble_notification[BleCount++] = NamiSwitch.SwMacStr[0];
                ble_notification[BleCount++] = NamiSwitch.SwMacStr[1];
                ble_notification[BleCount++] = ':';
                ble_notification[BleCount++] = NamiSwitch.SwMacStr[2];
                ble_notification[BleCount++] = NamiSwitch.SwMacStr[3];
                ble_notification[BleCount++] = ':';
                ble_notification[BleCount++] = NamiSwitch.SwMacStr[4];
                ble_notification[BleCount++] = NamiSwitch.SwMacStr[5];
                ble_notification[BleCount++] = ':';
                ble_notification[BleCount++] = NamiSwitch.SwMacStr[6];
                ble_notification[BleCount++] = NamiSwitch.SwMacStr[7];
                ble_notification[BleCount++] = ':'; 
                ble_notification[BleCount++] = NamiSwitch.SwMacStr[8];
                ble_notification[BleCount++] = NamiSwitch.SwMacStr[9];
                ble_notification[BleCount++] = ':';
                ble_notification[BleCount++] = NamiSwitch.SwMacStr[10];
                ble_notification[BleCount++] = NamiSwitch.SwMacStr[11];
                ble_notification[BleCount++] = ':';
                ble_notification[BleCount++] = '"';
                ble_notification[BleCount++] = ',';
                memcpy(&ble_notification[BleCount], BLE_NAME, strlen(BLE_NAME));
                BleCount += strlen(BLE_NAME); 
                ble_notification[BleCount++] = ',';
                memcpy(&ble_notification[BleCount], BLE_TYPE, strlen(BLE_TYPE));
                BleCount += strlen(BLE_TYPE);       

                ble_notification[BleCount++] = '}';

                // if(ble_connect_status){
                send_ble_message((char *)ble_notification);
                vTaskDelay(pdMS_TO_TICKS(100)); // Add a small delay after sending
            }

            apps_ble_stop();

            // vTaskResume(led_wifi_task_handle); // Resume LED WiFi task
            setting_mode_flag = true;
            // NamiSwitch.SaveWifiFlag = NAMI_SAVE;
            NamiFlash();
            NAMI_PRE_GPIO_OUTPUT_SET(GPIO_LED_NOTIFY, 1);
            NamiSwitch.ModeCalibFlag = false;
            return;
        }
        attempt--;
    }
    LOGA(WF, "Exiting setting mode...\r\n");
}


TickType_t key0_press_start = 0;
TickType_t press_duration = 0;
void key1_irq_neg(void *arg)
{
    key0_press_start = xTaskGetTickCount();
    NamiSwitch.TimerStartFlag = true;
    NAMI_GPIO_IRQ_SET(&isr_key1, HOSAL_IRQ_TRIG_POS_PULSE, key1_irq_pos, NULL);

}
void key1_irq_pos(void *arg)
{
    static TickType_t last_press_time = 0;

    press_duration = (xTaskGetTickCount() - key0_press_start) * portTICK_PERIOD_MS;
    NamiSwitch.TimerStopFlag = true;
    if (press_duration < 2000) { // Short press
        NAMI_PRE_GPIO_OUTPUT_SET(GPIO_LED_NOTIFY, 0);
        gpio_status_relay4 = 1; // Toggle relay state
        last_press_time = xTaskGetTickCount(); // Update last press time

    }
    NAMI_GPIO_IRQ_SET(&isr_key1, HOSAL_IRQ_TRIG_NEG_PULSE, key1_irq_neg, NULL);
    // Send event to queue (use ISR-safe version)
    gpio_event_t event = {
        GPIO_RELAY_OUT4, 
        gpio_status_relay4,
        press_duration
    };
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(gpio_event_queue, &event, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

uint8_t relay1_state_change_flag = 0;
uint8_t relay2_state_change_flag = 0;
uint8_t relay3_state_change_flag = 0;
uint8_t relay4_state_change_flag = 0;

void relay_event_task(void *pvParameters)
{
    gpio_event_t event;
    uint16_t delay_ms = 4000;
    LOGA(IR, " ->>>>>>>>> [test relay event task] <<<<<<<<<<\r\n", __LINE__, __func__);

    NamiSwitch.SwitchStatusBroker = &NamiMQTT.StatusBroker;
    NamiSwitch.SwMacStr           = &NamiMQTT.mac_str;
    NamiSwitch.StateOld           = 0;
    //NamiSwitch.StateCurrent       = 10;


    while (1) {
        if (xQueueReceive(gpio_event_queue, &event, portMAX_DELAY) == pdPASS) {
            vTaskDelay(pdMS_TO_TICKS(50)); // Delay to allow other tasks to run
            NamiSwitch.SetCount++;
            LOGA(WF, "event.duration=%d/ count=%d\r\n", event.duration, NamiSwitch.SetCount);

            if(event.duration >= 12000){
                NamiSwitch.Duration = event.duration;
                event.duration = 0;
                NamiSwitch.ChangeCalibFlag = false;
                ERR(WF, "Err: NOISE - Duration=%d/ count=%d\r\n", NamiSwitch.Duration, NamiSwitch.SetCount);
                submit_gpio_relay_status_to_mqtt_server(NOISE_BUTTON);
            }else if(event.duration >= 9000){
                LOGA(WF, "Reset Firmware after 3s...\r\n");
                vTaskDelay(pdMS_TO_TICKS(3000));
                NAMI_PRE_SYSTEM_RESET();	
            }else if(event.duration >= 3000){
                 if(NamiSwitch.BreakOutCalibrateFlag){
                    NamiSwitch.SetCount = 0;
                    NamiSwitch.ChangeCalibFlag = false;
                    NAMI_PRE_GPIO_OUTPUT_SET(GPIO_LED_NOTIFY, 1);
                    NamiSwitch.BreakOutCalibrateFlag = false;
                }else if(*NamiSwitch.SwitchStatusBroker != NAMI_CONNECTED || NamiSwitch.SetCount != 0){
                    NAMI_PRE_GPIO_OUTPUT_SET(GPIO_LED_NOTIFY, 0);
                    NamiSwitch.SetCount = 0;
                    NamiSwitch.ExitCalibrateFlag = true;
                    NamiSwitch.ChangeCalibFlag = false;
                    //vTaskDelay(pdMS_TO_TICKS(50)); // Delay to allow other tasks to run
                    
                    setting_mode_task(0);
                }else{
                    NamiSwitch.SetCount = 0;
                    NamiSwitch.ChangeCalibFlag = false;
                    NAMI_PRE_GPIO_OUTPUT_SET(GPIO_LED_NOTIFY, 1);
                }
            }else{
                NAMI_PRE_GPIO_OUTPUT_SET(GPIO_LED_NOTIFY, 1);
            }
        }
    }
}
