#ifndef __OTA_H__
#define __OTA_H__

#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include <string.h>
#include <aos/yloop.h>
#include <aos/kernel.h>
#include <lwip/tcpip.h>
#include <wifi_mgmr_ext.h>
#include <hal_wifi.h>

#include <hal_boot2.h>
#include "http_parser.h"
#include "ota_parse.h"
#include "ai_ota.h"
#include "ota_config.h"
#include "ota_hal.h"
#include "cJSON.h"
#include "bl_sys.h"
#include "plog.h"

// OTA Update Trigger Function
int trigger_ota_update();

// MQTT OTA Update Handler
void handle_mqtt_ota_update(char* payload);

// Periodic OTA Check Task
void periodic_ota_check(void* pvParameters);

// MQTT Subscription Setup
void setup_ota_mqtt_subscription();

// proc_main_entry
void proc_main_entry();

int axk_hal_handle_ota_json(const char* json_str);


#endif // __OTA_H__