#ifndef WIFI_EXECUTE_H_
#define WIFI_EXECUTE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <FreeRTOS.h>
#include "portmacro.h"
#include <task.h>
#include <timers.h>
#include "event_groups.h"
#include <aos/kernel.h>
#include <aos/yloop.h>
#include <event_device.h>
#include <easyflash.h>
#include <hal_sys.h>
#include <bl_wifi.h>
#include <hal_wifi.h>
#include <bl_timer.h>
#include <bl_sys_time.h>
#include <bl_sys.h>
#include <wifi_mgmr_ext.h>
#include <blog.h>
#include <bl60x_wifi_driver/wifi_mgmr.h>
#include <bl_gpio.h>
#include "mqtt.h"
// #include "time_rtc.h"
// #include "storage.h"
#include "bl_flash.h"
#include "switch.h"
#include "plog.h"


#define AP_SSID "congtac_"
#define AP_PWD "12345678"

#define MAX_SSID_LENGTH 33       // Maximum SSID length (32 + null terminator)
#define MAX_PASSWORD_LENGTH 65   // Maximum Wi-Fi password length (64 + null terminator)
#define MAX_VERSION_LENGTH 15   // Maximum Wi-Fi password length (64 + null terminator)

typedef struct {
    char ssid[MAX_SSID_LENGTH];
    char password[MAX_PASSWORD_LENGTH]; // STA password
    char ap_password[MAX_PASSWORD_LENGTH]; // AP password
} wifi_credentials_t;


extern int g_wifi_sta_is_connected;
extern char g_curr_ssid[MAX_SSID_LENGTH];
extern char g_curr_pass[MAX_PASSWORD_LENGTH];
extern char gssid_ap[50];

// Scan context structure
typedef struct {
    SemaphoreHandle_t scan_sem;
    uint8_t scan_done;
} wifi_scan_context_t;

extern wifi_scan_context_t scan_ctx;

void wifi_execute(void *pvParameters);
void wifi_disconnect();
int wifi_sta_connect(char *ssid, char *password);
void wifi_scanList(void);

#endif /* WIFI_EXECUTE_H_ */
