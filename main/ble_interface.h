#ifndef __BLE_INTERFACE_H__
#define __BLE_INTERFACE_H__

#include <stdint.h>

#include "bluetooth.h"

// #include "ble_interface.h"

#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include <semphr.h>

#include "bluetooth.h"
#include "hci_driver.h"
#include "hci_core.h"
#include "ble_lib_api.h"
#include "conn.h"
#include "conn_internal.h"
#include "gatt.h"
#include "bl_gpio.h"
#include "switch.h"
// #include "storage.h"
#include "wifi_execute.h"

typedef int (*ble_gatt_conn_cb_t)(struct bt_conn *conn, uint8_t code);

void ble_reverse_byte(uint8_t *arr, uint32_t size);
int ble_server_init();
int ble_server_deinit(void);
void ble_stack_start(void);
int ble_uuid1_notify_data(void *handle, void *data, uint16_t length);
int ble_uuid2_notify_data(void *handle, void *data, uint16_t length);

/************************************ble common*******************************************/
struct bt_conn *ble_get_conn_cur(void);
int ble_regist_conn(ble_gatt_conn_cb_t cb);
int ble_regist_disconn(ble_gatt_conn_cb_t cb);
int ble_slave_init();
int ble_slave_deinit(void);
int UUID1_SendNotify(uint16_t len, uint8_t *data);
int UUID2_SendNotify(uint16_t len, uint8_t *data);
int ble_slave_init();
int ble_slave_deinit(void);
int ble_salve_adv();
void apps_ble_stop();
void apps_ble_start();
uint8_t BleSetMtu();
void led_control(char *command);
extern void send_ble_message(const char *message);
extern bool ble_connect_status;
extern char *Read_BLE(char *data);
extern char *Get_BLE_Data();
void set_slave_name_with_mac(const char *name);
void generate_ble_name();
extern uint8_t ble_start_flag;

#endif
