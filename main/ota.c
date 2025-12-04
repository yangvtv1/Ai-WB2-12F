/**
 * @file main.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-10-09
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "ota.h"

// Add these macro definitions after the includes
#define cJSON_IsString(item) ((item) != NULL && (item)->type == cJSON_String)
#define cJSON_IsNumber(item) ((item) != NULL && (item)->type == cJSON_Number)
#define cJSON_IsBool(item)   ((item) != NULL && ((item)->type & (cJSON_True | cJSON_False)))

#define ROUTER_SSID "KAI"
#define ROUTER_PWD "P@ssword@test!"

// #define url "https://chencong--test.oss-cn-beijing.aliyuncs.com/test.bin"

// blink test
// #define url "https://cms.namismart.vn/assets/b3fc11b8-f4ed-4d79-92eb-c7f449d4c43a?download="

int axk_hal_user_ota_update(const char* url);

static ota_parame ota_param;
static int ota_type;
static char ota_host[256];
static char ota_resource[256];
static const char* current_url = NULL;

static wifi_conf_t conf =
{
    .country_code = "CN",
};

static void wifi_sta_connect(char* ssid, char* password)
{
    wifi_interface_t wifi_interface;

    wifi_interface = wifi_mgmr_sta_enable();
    wifi_mgmr_sta_connect(wifi_interface, ssid, password, NULL, NULL, 0, 0);
}

static void event_cb_wifi_event(input_event_t* event, void* private_data)
{
    static char* ssid;
    static char* password;

    switch (event->code)
    {
        case CODE_WIFI_ON_INIT_DONE:
        {
            LOGA(OTA, "[APP] [EVT] INIT DONE %lld\r\n", aos_now_ms());
            wifi_mgmr_start_background(&conf);
        }
        break;
        case CODE_WIFI_ON_MGMR_DONE:
        {
            LOGA(OTA, "[APP] [EVT] MGMR DONE %lld\r\n", aos_now_ms());
            wifi_sta_connect(ROUTER_SSID, ROUTER_PWD);
        }
        break;
        case CODE_WIFI_ON_DISCONNECT:
        {
            LOGA(OTA, "[APP] [EVT] disconnect %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_CONNECTING:
        {
            LOGA(OTA, "[APP] [EVT] Connecting %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_CMD_RECONNECT:
        {
            LOGA(OTA, "[APP] [EVT] Reconnect %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_CONNECTED:
        {
            LOGA(OTA, "[APP] [EVT] connected %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_PRE_GOT_IP:
        {
            LOGA(OTA, "[APP] [EVT] connected %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_GOT_IP:
        {
            LOGA(OTA, "[APP] [EVT] GOT IP %lld\r\n", aos_now_ms());
            LOGA(OTA, "[SYS] Memory left is %d Bytes\r\n", xPortGetFreeHeapSize());
            // axk_hal_user_ota_update("https://cms.namismart.vn/assets/b3fc11b8-f4ed-4d79-92eb-c7f449d4c43a?download=");
        }
        break;
        case CODE_WIFI_ON_PROV_CONNECT:
        {
            LOGA(OTA, "[APP] [EVT] [PROV] [CONNECT] %lld\r\n", aos_now_ms());
            LOGA(OTA, "vconnecting to %s:%s...\r\n", ssid, password);
            wifi_sta_connect(ROUTER_SSID, ROUTER_PWD);
        }
        break;
        case CODE_WIFI_ON_PROV_DISCONNECT:
        {
            LOGA(OTA, "[APP] [EVT] [PROV] [DISCONNECT] %lld\r\n", aos_now_ms());
        }
        break;
        default:
        {
            LOGA(OTA, "[APP] [EVT] Unknown code %u, %lld\r\n", event->code, aos_now_ms());
            /*nothing*/
        }
    }
}


void _ota_task(void* pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(500));
    if (ota_type == AT_OTA_MODE_HTTP) {
        ai_http_update_ota(&ota_param);
    } else {
        ai_https_update_ota(&ota_param);
    }

    vTaskDelete(NULL);
}

int axk_hal_user_ota_update(const char* url)
{
    int port;
    char schema[8] = {0};
    struct http_parser_url purl;
    HALPartition_Entry_Config otaEntry;
    uint16_t length;






    if (hal_boot2_get_active_entries(BOOT2_PARTITION_TYPE_FW, &otaEntry)) {
        LOGA(OTA, "[OTA] get otaEntry fail\r\n");
        return -1;
    }

    http_parser_url_init(&purl);
    length=strlen(url);

    int parser_status = http_parser_parse_url(url, length, 0, &purl);

    if (parser_status != 0) {
        LOGA(OTA, "[OTA] Error parse url:%s\r\n", url);
        return -1;
    }

    memset(ota_host, 0, sizeof(ota_host));
    memset(ota_resource, 0, sizeof(ota_resource));

    if (purl.field_data[UF_SCHEMA].len > 8) {
        LOGA(OTA, "[OTA] schema ovfl \r\n");
        return -1;
    }
    memcpy(schema, url + purl.field_data[UF_SCHEMA].off, purl.field_data[UF_SCHEMA].len);
    if (strcasecmp(schema, "http") == 0) {
        ota_type = AT_OTA_MODE_HTTP;
        port = 80;
    } else if (strcasecmp(schema, "https") == 0) {
        ota_type = AT_OTA_MODE_HTTPS;
        port = 443;
    } else {
        LOGA(OTA, "[OTA] undef schema\r\n");
        return -1;
    }
    port = purl.port ? purl.port : port;
    memcpy(ota_host, url + purl.field_data[UF_HOST].off, purl.field_data[UF_HOST].len);
    memcpy(ota_resource, url + purl.field_data[UF_PATH].off, purl.field_data[UF_PATH].len);

    LOGA(OTA, "[OTA] port:%d host:%s path:%s\r\n", port, ota_host, ota_resource);

    ota_param = ai_ota_parame_init(ota_host, port, ota_resource);

    int ret = xTaskCreate(_ota_task, "ota", 4096, NULL, 10, NULL);
    if (ret != pdPASS) {
        LOGA(OTA, "[OTA] task create fail: %d\r\n", ret);
        return -1;
    } else {
        LOGA(OTA, "[OTA] task create success\r\n");
    }

    return 0;
}

int axk_hal_handle_ota_json(const char* json_str)
{
    LOGA(OTA, "[OTA] Processing update request: %s\r\n", json_str);
    
    cJSON *json = cJSON_Parse(json_str);
    if (json == NULL) {
        LOGA(OTA, "[OTA] Error parsing JSON data\r\n");
        LOGA(OTA, "Reset Firmware after 3s...\r\n");
		vTaskDelay(pdMS_TO_TICKS(3000));
		bl_sys_reset_por();
        return -1;
    }

    // Get URL (required)
    cJSON *url_item = cJSON_GetObjectItem(json, "url");
    if (!url_item || !cJSON_IsString(url_item)) {
        LOGA(OTA, "[OTA] Missing or invalid 'url' field\r\n");
        cJSON_Delete(json);
        LOGA(OTA, "Reset Firmware after 3s...\r\n");
		vTaskDelay(pdMS_TO_TICKS(3000));
		bl_sys_reset_por();
        return -1;
    }

    // Get optional fields
    cJSON *version = cJSON_GetObjectItem(json, "version");
    cJSON *force = cJSON_GetObjectItem(json, "force");
    cJSON *md5 = cJSON_GetObjectItem(json, "md5");
    cJSON *size = cJSON_GetObjectItem(json, "size");
    
    // Log update information
    LOGA(OTA, "[OTA] Update requested:\r\n");
    LOGA(OTA, "  URL: %s\r\n", url_item->valuestring);
    if (version && cJSON_IsString(version)) {
        LOGA(OTA, "  Version: %s\r\n", version->valuestring);
    }
    if (force && cJSON_IsBool(force)) {
        LOGA(OTA, "  Force: %s\r\n", force->valueint ? "true" : "false");
    }
    if (md5 && cJSON_IsString(md5)) {
        LOGA(OTA, "  MD5: %s\r\n", md5->valuestring);
    }
    if (size && cJSON_IsNumber(size)) {
        LOGA(OTA, "  Size: %lld\r\n", size->valueint);
    }

    // Store URL globally
    current_url = strdup(url_item->valuestring);
    
    // Start the OTA update
    int result = axk_hal_user_ota_update(current_url);
    
    cJSON_Delete(json);
    return result;
}

void proc_main_entry(void* pvParameters)
{
    aos_register_event_filter(EV_WIFI, event_cb_wifi_event, NULL);
    hal_wifi_start_firmware_task();
    aos_post_event(EV_WIFI, CODE_WIFI_ON_INIT_DONE, 0);
    vTaskDelete(NULL);
}

// void main()
// {
//     xTaskCreate(proc_main_entry, (char*)"main_entry", 1024, NULL, 15, NULL);
//     tcpip_init(NULL, NULL);
// }
