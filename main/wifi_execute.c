#include "wifi_execute.h"
#include "ble_interface.h"


static wifi_conf_t conf = {
    .country_code = "CN",
};

int g_wifi_sta_is_connected = 0;
char g_curr_ssid[MAX_SSID_LENGTH];
char g_curr_pass[MAX_PASSWORD_LENGTH];
char gssid_ap[50];



char upate_version[MAX_VERSION_LENGTH] = "";

wifi_scan_context_t scan_ctx;

// Scan complete callback
static void wifi_scan_complete_cb(void *data, void *param)
{
    wifi_scan_context_t *ctx = (wifi_scan_context_t *)data;
    int status = *((int *)param);
    
    if (status == 1) {
        ctx->scan_done = 1;
        if (ctx->scan_sem != NULL) {
            xSemaphoreGive(ctx->scan_sem);
        }
    }
}


static void wifi_ap_ip_set(char* ip_addr, char* netmask, char* gw)
{
    struct netif* ap_netif = netif_find("ap1");
    int i = 0;
    int ap_ipaddr[4] = { 0 };
    int ap_netmask[4] = { 255,255,255,0 };
    int ap_gw_arry[4] = { 0,0,0,0 };

    ap_ipaddr[0] = atoi(strtok(ip_addr, "."));

    for (i = 1;i<4;i++) {
        ap_ipaddr[i] = atoi(strtok(NULL, "."));
    }
    if (netmask) {
        ap_netmask[0] = atoi(strtok(netmask, "."));
        for (i = 1;i<4;i++)
            ap_netmask[i] = atoi(strtok(NULL, "."));
    }
    if (gw) {
        ap_gw_arry[0] = atoi(strtok(gw, "."));
        for (i = 1;i<4;i++)
            ap_gw_arry[i] = atoi(strtok(NULL, "."));
    }

    if (ap_netif) {
        ip_addr_t ap_ip;
        ip_addr_t ap_mask;
        ip_addr_t ap_gw;
        IP4_ADDR(&ap_ip, ap_ipaddr[0], ap_ipaddr[1], ap_ipaddr[2], ap_ipaddr[3]);
        IP4_ADDR(&ap_mask, ap_netmask[0], ap_netmask[1], ap_netmask[2], ap_netmask[3]);
        IP4_ADDR(&ap_gw, ap_gw_arry[0], ap_gw_arry[1], ap_gw_arry[2], ap_gw_arry[3]);

        netif_set_down(ap_netif);
        netif_set_ipaddr(ap_netif, &ap_ip);
        netif_set_netmask(ap_netif, &ap_mask);
        netif_set_gw(ap_netif, &ap_gw);
        netif_set_up(ap_netif);
        LOGA(WF, "[softAP]:SSID:%s,PASSWORD:%s,IP addr:%s\r\n", AP_SSID, AP_PWD, ip4addr_ntoa(netif_ip4_addr(ap_netif)));
    }
    else {
        LOGA(WF, "no find netif ap1\r\n");
	}
}

void wifi_ap_start()
{
	char last_3digit[4]; // Buffer to store last 3 digits of gdevice_id
    int len = strlen(gdevice_id);
    
    snprintf(last_3digit, 4, "%s", &gdevice_id[len-3]); // Get last 3 digits of the device_id
    snprintf(gssid_ap, sizeof(gssid_ap), "%s%s", AP_SSID, last_3digit); 

	wifi_interface_t ap_interface = wifi_mgmr_ap_enable();
    wifi_mgmr_conf_max_sta(4);
    wifi_mgmr_ap_start(ap_interface, gssid_ap, 0, "", 6);
    wifi_ap_ip_set("192.168.169.1", "255.255.255.0", "192.168.169.1");
}

void wifi_disconnect()
{
    wifi_mgmr_sta_disconnect();
    bl_os_msleep(WIFI_MGMR_STA_DISCONNECT_DELAY);
    wifi_mgmr_sta_disable(NULL);
    g_wifi_sta_is_connected = 0; // disconnect
}
    uint8_t read_wifi[512] = {0};

int wifi_sta_connect(char *ssid, char *password)
{
    wifi_interface_t wifi_sta_interface = wifi_mgmr_sta_enable();
	LOGA(OTA, "vinitialzing wifi connection\r\n");
	if (strncmp(ssid, "na", 2) == 0)
	{
		char temp_ssid[MAX_SSID_LENGTH] = "";  // Assuming SSID is not longer than 32 bytes
		char temp_pass[MAX_PASSWORD_LENGTH] = "";  // Assuming password is not longer than 64 bytes

		ef_get_str("ssid", temp_ssid, sizeof(temp_ssid));
		if(temp_ssid == NULL){
			LOGA(OTA, "Error: Read flash ssid of %s fail\r\n", temp_ssid);
		}

		ef_get_str("pass", temp_pass, sizeof(temp_pass));
		if(temp_pass == NULL){
			LOGA(OTA, "Error: Read flash pass of %s fail\r\n", temp_pass);
		}

		ef_get_str("version", NamiMQTT.CheckVer, sizeof(NamiMQTT.CheckVer));
		if(NamiMQTT.CheckVer == NULL){
			LOGA(OTA, "Error: Read flash version of %s fail\r\n", NamiMQTT.CheckVer);
		}

		ef_get_u8("StateUDF", &NamiMQTT.StateUDFFlag);
		if(NamiMQTT.StateUDFFlag == NULL){
			LOGA(OTA, "Error: Read flash state UDF %u fail\r\n", NamiMQTT.StateUDFFlag);
		}

		ef_get_str("host", NamiMQTT.Host, sizeof(NamiMQTT.Host));
		if(NamiMQTT.Host == NULL){
			ERR(OTA, "Error: Read flash version of %s fail\r\n", NamiMQTT.Host);
		}else{
			LOGA(OTA, "Read flash version of %s\r\n", NamiMQTT.Host);
		}

		// for(uint8_t index = 0; index < strlen(FIRMWAREVERSION); index++){
		//     if(temp_version[index] == upate_version[])
		// }
		// if(strcmp(current_version, FIRMWAREVERSION) == 0){
		//     LOGA(OTA, "Version Curr=(%s)/ Version Update:(%s)\r\n", FIRMWAREVERSION, current_version);
		// }else if(current_version != NULL){
		//     LOGA(OTA, "Version Curr=(%s)/ Version Update:(%s)\r\n", FIRMWAREVERSION, current_version);
		// }else{
		//     LOGA(OTA, "Err: Version Curr=(%s)/ Version Update:(%s)\r\n", FIRMWAREVERSION, current_version);
		// }

		//connect to previous wifi
    	if (temp_ssid != NULL && temp_pass != NULL) {
			strcpy(g_curr_ssid, temp_ssid);
			strcpy(g_curr_pass, temp_pass);
			LOGA(WF, "SSID: %s, Password: %s\r\n", g_curr_ssid, g_curr_pass);

			// Attempt to connect using the retrieved credentials
			return wifi_mgmr_sta_connect(wifi_sta_interface, temp_ssid, temp_pass, NULL, NULL, 0, 0);       
		}
		else {
			// memset(&temp_ssid, 0x00, sizeof(temp_ssid));
			// memcpy(&temp_ssid, "Cao Cuong 2", strlen("Cao Cuong 2"));
			// memset(&temp_pass, 0x00, sizeof(temp_pass));
			// memcpy(&temp_pass, "lambaohan2000@2", strlen("lambaohan2000@2"));

			// memset(&temp_ssid, 0x00, sizeof(temp_ssid));
			// memcpy(&temp_ssid, "Nami R&D", strlen("Nami R&D"));
			// memset(&temp_pass, 0x00, sizeof(temp_pass));
			// memcpy(&temp_pass, "nami@2025", strlen("nami@2025"));

			LOGA(WF, "WiFi credentials not found (temp using hardcode)\r\n");
			strcpy(g_curr_ssid, temp_ssid);
			strcpy(g_curr_pass, temp_pass);
			LOGA(WF, "ssid={%s}/ pass={%s}\r\n", temp_ssid, temp_pass);
			return wifi_mgmr_sta_connect(wifi_sta_interface, temp_ssid, temp_pass, NULL, NULL, 0, 0);

		}
	}else {
		// Attempt to connect using the provided credentials
        strcpy(g_curr_ssid, ssid);
		strcpy(g_curr_pass, password);
        // LOGA(WF, "wifi connect ssid: %s, pass: %s\r\n", g_curr_ssid, g_curr_pass);
		int connection_result = wifi_mgmr_sta_connect(wifi_sta_interface, ssid, password, NULL, NULL, 0, 0);
		// LOGA(WF, "connection_result: %d\r\n",connection_result);
        // return wifi_mgmr_sta_connect(wifi_sta_interface, ssid, password, NULL, NULL, 0, 0);
		
		// bl_flash_read(WIFI_OFFSET, read_wifi, sizeof(read_wifi));
		// LOGA(WF, "read_wifi\r\n");
		// for(int i = 0; i < sizeof(read_wifi); i++) {
		// 	if(i%16 == 0) {
		// 		printf("\r\n");
		// 	}
		// 	printf("%02x ", read_wifi[i]);
		// }
		// printf("\r\n");
		LOGA(WF, "g_curr_ssid=(%s)/g_curr_pass=(%s)\r\n", g_curr_ssid, g_curr_pass);
		LOGA(WF, "Connection=%s \r\n", (!connection_result)?("Connected"):("Unknown"));

		return connection_result;
    }
	
}

void back_to_led_relay_state(){
    // gpio_switch_info_t gpio_switch_info;
    // Retrieve the GPIO relay states from flash
    // if (retrieve_gpio_relay_state_in_flash(&gpio_switch_info) != 0) {
    //     ERR(WF, "Failed to retrieve GPIO relay states from flash.\r\n");
    //     return;
    // }
    // bl_gpio_output_set(GPIO_LED1_STATUS, gpio_switch_info.gpio_status_relay3);
}
unsigned int ret = 0;
size_t size_wf;
static void wifi_event_cb(input_event_t *event, void *private_data)
{
    LOGA(WF, "[APP] [EVT] event->code %d\r\n", event->code);
	size_wf = xPortGetFreeHeapSize();
    LOGA(WF, "[SYS] Memory left is %d Bytes\r\n", size_wf);

    switch (event->code)
    {
		case CODE_WIFI_ON_AP_STARTED:
		{
			ret = aos_now_ms();
			LOGA(WF, "[APP] [EVT] AP STARTED DONE %lld\r\n", ret);
		}
		break;
		case CODE_WIFI_ON_AP_STOPPED:
		{
			ret = aos_now_ms();
			LOGA(WF, "[APP] [EVT] AP STOP DONE %lld\r\n",ret);
		}
		break;
		case CODE_WIFI_ON_INIT_DONE:
		{
			ret = aos_now_ms();
			LOGA(WF, "[APP] [EVT] INIT DONE %lld\r\n", ret);
			wifi_mgmr_start_background(&conf);
			wifi_sta_connect("na", NULL);
		}
		break;
		case CODE_WIFI_ON_MGMR_DONE:
		{
			ret = aos_now_ms();
			LOGA(WF, "[APP] [EVT] MGMR DONE %lld\r\n", ret);
			wifi_mgmr_scan(NULL, NULL);
			// wifi_ap_start();
		}
		break;
		case CODE_WIFI_ON_SCAN_DONE:
		{
			ret = aos_now_ms();
			LOGA(WF, "[APP] [EVT] SCAN Done %lld\r\n", ret);
		    if (scan_ctx.scan_sem != NULL) {
                xSemaphoreGive(scan_ctx.scan_sem);
            }
		}
		break;
		case CODE_WIFI_ON_DISCONNECT:
		{
			ret = aos_now_ms();
			scan_ctx.ExtMQTTDestroy();
			g_wifi_sta_is_connected = 0;
			LOGA(WF, "[APP] [EVT] disconnect %lld\r\n", ret);
		}
		break;
		case CODE_WIFI_ON_CONNECTING:
		{
			ret = aos_now_ms();
			LOGA(WF, "[APP] [EVT] Connecting %lld\r\n", ret);
		}
		break;
		case CODE_WIFI_CMD_RECONNECT:
		{
			ret = aos_now_ms();
			LOGA(WF, "[APP] [EVT] Reconnect %lld\r\n", ret);
		}
		break;
		case CODE_WIFI_ON_CONNECTED:
		{
			ret = aos_now_ms();
			LOGA(WF, "[APP] [EVT] connected %lld\r\n", ret);
		}
		break;
		case CODE_WIFI_ON_AP_STA_ADD:
		{
			LOGA(WF, "Station connected to AP\r\n");
		}
		break;
		case CODE_WIFI_ON_AP_STA_DEL:
		{
			LOGA(WF, "Station disconnected from AP\r\n");
		}
		break;
		case CODE_WIFI_ON_GOT_IP:
		{
			ret = aos_now_ms();
			LOGA(WF, "[APP] [EVT] WIFI STA GOT IP %lld\r\n", ret);
			g_wifi_sta_is_connected = 1;
			LOGA(WF, "ssid: %s, pass: %s\r\n", g_curr_ssid, g_curr_pass);
			//store_wifi_credentials_to_flash(g_curr_ssid, g_curr_pass);
			// back_to_led_relay_state();
			setting_mode_enable = false;
			// struct netif *netif = netif_find("st1");
			// if (netif != NULL) {
			// 	char ip_str[16];
			// 	snprintf(ip_str, sizeof(ip_str), "%s", ip4addr_ntoa(netif_ip4_addr(netif)));
			// 	LOGA(WF, "Current IP address: %s\r\n", ip_str);
			// } else {
			// 	LOGA(WF, "Network interface not found\r\n");
			// }
			scan_ctx.ExtMQTTStart();
		}
		break;
		case CODE_WIFI_ON_AP_STA_GOT_IP:
		{
			LOGA(WF, "[APP] [EVT] AP/STA GOT IP event triggered\r\n");
		}
		break;
		default:
		{
			ret = aos_now_ms();
			LOGA(WF, "[APP] [EVT] Unknown code %u, %lld\r\n", event->code, ret);
		}
    }
}

void wifi_scanList(void){
    // wifi_mgmr_scan_params_t scan_params = {0};
    char wifi_fail_message[64] ="WiFi scan failed";
    // // Initialize scan parameters (as seen in wifi_mgmr_cli.c)
    // scan_params.channel_num = 0;  // Scan all channels
    // memset(scan_params.bssid, 0xFF, sizeof(scan_params.bssid)); // Broadcast MAC
    // scan_params.ssid.length = 0;  // Scan all SSIDs
    // scan_params.scan_mode = SCAN_ACTIVE;
    // scan_params.duration_scan = 0; // Use default duration

    // Initialize scan context
    scan_ctx.scan_sem = xSemaphoreCreateBinary();
    scan_ctx.scan_done = 0;
    
    if (scan_ctx.scan_sem == NULL) {
        ERR(WF, "Failed to create scan semaphore\r\n");
        send_ble_message(wifi_fail_message);
        return;
    }

    // Send start notification
    send_ble_message("Starting WiFi scan...");

    // Start scan using the correct API (as seen in wifi_mgmr_ext.c)
    int ret = wifi_mgmr_scan(&scan_ctx, wifi_scan_complete_cb);
    if (ret != 0) {
        ERR(WF, "Failed to start WiFi scan (error %d)\r\n", ret);
        send_ble_message(wifi_fail_message);
        vSemaphoreDelete(scan_ctx.scan_sem);
        scan_ctx.scan_sem = NULL;
        return;
    }
	char wifi_entry[128];
    // Wait for scan to complete (with timeout)
    if (xSemaphoreTake(scan_ctx.scan_sem, pdMS_TO_TICKS(10000))) {
        // Process scan results by iterating through wifiMgmr.scan_items
        uint32_t counter = 0;
        for (int i = 0; i < sizeof(wifiMgmr.scan_items)/sizeof(wifiMgmr.scan_items[0]); i++) {
            if (wifiMgmr.scan_items[i].is_used && 
                (!wifi_mgmr_scan_item_is_timeout(&wifiMgmr, &wifiMgmr.scan_items[i]))) {
			    int written = snprintf(
					wifi_entry, sizeof(wifi_entry),
					"i%02d; bssid=%02X:%02X:%02X:%02X:%02X:%02X, rssi=%3d, SSID=%s\r\n",
					counter,
					wifiMgmr.scan_items[i].bssid[0],
					wifiMgmr.scan_items[i].bssid[1],
					wifiMgmr.scan_items[i].bssid[2],
					wifiMgmr.scan_items[i].bssid[3],
					wifiMgmr.scan_items[i].bssid[4],
					wifiMgmr.scan_items[i].bssid[5],
					wifiMgmr.scan_items[i].rssi,
					wifiMgmr.scan_items[i].ssid
				);
				counter++;
				if (written > 0 && written < sizeof(wifi_entry)) {
					send_ble_message(wifi_entry);
				}
        	}
			// Send summary
			// char summary[64];
			// snprintf(summary, sizeof(summary), "Scan complete. Found %d networks\r\n", counter);
			// send_ble_message(summary);
    	}
	}
	else {
        LOGA(WF, "Scan timed out\r\n");
        send_ble_message("WiFi scan timed out");
    }

    vSemaphoreDelete(scan_ctx.scan_sem);
    scan_ctx.scan_sem = NULL;
}

void wifi_execute(void *pvParameters)
{
	memset(&scan_ctx, 0x00, sizeof(scan_ctx));
	scan_ctx.ExtMQTTStart     = &MQTTStart;
	scan_ctx.ExtMQTTDestroy   = &MQTTDestroy;
    aos_register_event_filter(EV_WIFI, wifi_event_cb, NULL);
    hal_wifi_start_firmware_task();
    aos_post_event(EV_WIFI, CODE_WIFI_ON_INIT_DONE, 0);
    
    vTaskDelete(NULL);
}
