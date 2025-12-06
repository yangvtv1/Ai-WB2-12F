#include "ble_interface.h"

// #include <stdio.h>
// #include <string.h>
// #include <FreeRTOS.h>
// #include <semphr.h>

// #include "bluetooth.h"
// #include "hci_driver.h"
// #include "hci_core.h"
// #include "ble_lib_api.h"
// #include "conn.h"
// #include "conn_internal.h"
// #include "gatt.h"
// #include "bl_gpio.h"
// #include "switch.h"
// // #include "storage.h"
// #include "wifi_execute.h"



/*自定义UUID*/
#define UUID1_USER_SER BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x55535343, 0xfe7d, 0x4ae5, 0x8fa9, 0x9fafd205e455))
#define UUID1_USER_TXD BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x49535343, 0x8841, 0x43f4, 0xa8d4, 0xecbe34729bb3))
#define UUID1_USER_RXD BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x49535343, 0x1e4d, 0x4bd9, 0xba61, 0x23c647249616))

#define UUID2_USER_SER BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x10190d0c, 0x0b0a, 0x0908, 0x0706, 0x050403020100))
#define UUID2_USER_TXD BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x102B0d0c, 0x0b0a, 0x0908, 0x0706, 0x050403020100))
#define UUID2_USER_RXD BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x102B0d0d, 0x0b0a, 0x0908, 0x0706, 0x050403020100))

#define SALVE_CMD_SERVER_TX_INDEX 2

static struct bt_conn *conn_cur;
ble_gatt_conn_cb_t conn_cb;
ble_gatt_conn_cb_t disconn_cb;


//1 Socket
#define BLE_1SK_PREFIX "OCam1"
//3 Socket
#define BLE_3SK_PREFIX "OCam3"

//1 Switch
#define BLE_1SW_PREFIX "CongTac1"
//2 Switch
#define BLE_2SW_PREFIX "CongTac2"
//3 Switch
#define BLE_3SW_PREFIX "CongTac3"
//4 Switch
#define BLE_4SW_PREFIX "CongTac4"

#define BLE_NAME_MAX_LENGTH 16  // "CongTac-XXX" + null terminator
char ble_slave_name[BLE_NAME_MAX_LENGTH];  // Global variable for BLE name
uint8_t ble_start_flag = false;

#define MANUFACTURER_ID 0xFFFF // Replace with your custom manufacturer ID
static const uint8_t custom_data[] = {0x01, 0x02, 0x03}; // Define custom data here
// Define manufacturer data as a static constant array
static const uint8_t manufacturer_data[] = {
    MANUFACTURER_ID & 0xFF,
    (MANUFACTURER_ID >> 8) & 0xFF,
    custom_data[0],
    custom_data[1],
    custom_data[2]
};

// static const struct bt_data salve_adv[] = {
//     BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
//     BT_DATA(BT_DATA_NAME_COMPLETE, BLE_SLAVE_NAME, BLE_SLAVE_NAME_LEN),
//     BT_DATA(BT_DATA_MANUFACTURER_DATA, manufacturer_data, sizeof(manufacturer_data))    
// };

static const struct bt_data salve_adv[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, ble_slave_name, sizeof(ble_slave_name) - 1),
    BT_DATA(BT_DATA_MANUFACTURER_DATA, manufacturer_data, sizeof(manufacturer_data))    

};

static ssize_t ble_uuid1_write_val(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                   const void *buf, u16_t len, u16_t offset, u8_t flags);
static ssize_t ble_uuid2_write_val(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                   const void *buf, u16_t len, u16_t offset, u8_t flags);
static void ble_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                                u16_t value);

static struct bt_gatt_attr salve_uuid1_server[] = {
    /* Primary Service */
    BT_GATT_PRIMARY_SERVICE(UUID1_USER_SER),

    /* Characteristic && Characteristic User Declaration */
    BT_GATT_CHARACTERISTIC(UUID1_USER_TXD,
                           BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ, NULL, NULL,
                           NULL),
    BT_GATT_CCC(ble_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

    /* Characteristic && Characteristic User Declaration */
    BT_GATT_CHARACTERISTIC(UUID1_USER_RXD,
                           BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_WRITE, NULL, ble_uuid1_write_val,
                           NULL),
};

static struct bt_gatt_attr salve_uuid2_server[] = {
    /* Primary Service */
    BT_GATT_PRIMARY_SERVICE(UUID2_USER_SER),

    /* Characteristic && Characteristic User Declaration */
    BT_GATT_CHARACTERISTIC(UUID2_USER_TXD,
                           BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ, NULL, NULL,
                           NULL),
    BT_GATT_CCC(ble_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

    /* Characteristic && Characteristic User Declaration */
    BT_GATT_CHARACTERISTIC(UUID2_USER_RXD,
                           BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_WRITE, NULL, ble_uuid2_write_val,
                           NULL),
};


static struct bt_gatt_service ble_uuid1_server = BT_GATT_SERVICE(salve_uuid1_server);
static struct bt_gatt_service ble_uuid2_server = BT_GATT_SERVICE(salve_uuid2_server);

// static ssize_t ble_uuid1_write_val(struct bt_conn *conn, const struct bt_gatt_attr *attr,
//                                    const void *buf, u16_t len, u16_t offset,
//                                    u8_t flags)
// {
//     uint8_t *recv_buffer;
//     recv_buffer = pvPortMalloc(sizeof(uint8_t) * len);
//     memcpy(recv_buffer, buf, len);
//     printf("recv ble data len: %d\r\n", len);
//     for (size_t i = 0; i < len; i++)
//     {
//         printf("0x%x ", recv_buffer[i]);
//     }
//     printf("\r\n");
//     vPortFree(recv_buffer);

//     return len;
// }

static ssize_t ble_uuid1_write_val(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                   const void *buf, u16_t len, u16_t offset,
                                   u8_t flags)
{
    uint8_t *recv_buffer;
    recv_buffer = pvPortMalloc(sizeof(uint8_t) * len);
    memcpy(recv_buffer, buf, len);
    LOGA(BLE, "recv ble data len: %d\r\n", len);
    for (size_t i = 0; i < len; i++)
    {
        printf("0x%x ", recv_buffer[i]);
    }
    printf("\r\n");
    vPortFree(recv_buffer);

    // Convert hex data to string
    char *str_buffer = pvPortMalloc(len + 1);
    memcpy(str_buffer, recv_buffer, len);
    str_buffer[len] = '\0'; // Null-terminate the string
    LOGA(BLE, "recv ble string data: %s\r\n", str_buffer);
    Read_BLE(str_buffer);

    if(strcmp(str_buffer,"wifi_list_req")==0){
        LOGA(BLE, "Requesting WiFi scan list...\r\n");
        wifi_scanList();
    }
    else{
       ble_connect_wifi(str_buffer);
    }

    vPortFree(str_buffer);    
    // led_control(str_buffer);
    return len;
}

static ssize_t ble_uuid2_write_val(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                   const void *buf, u16_t len, u16_t offset,
                                   u8_t flags)
{
    uint8_t *recv_buffer;
    recv_buffer = pvPortMalloc(sizeof(uint8_t) * len);
    memcpy(recv_buffer, buf, len);
    LOGA(BLE, "recv ble data len: %d\r\n", len);
    for (size_t i = 0; i < len; i++)
    {
        printf("0x%x ", recv_buffer[i]);
    }
    printf("\r\n");
    vPortFree(recv_buffer);
    return len;
}

static void ble_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                                u16_t value)
{
    char *str = "disabled";

    if (value == BT_GATT_CCC_NOTIFY)
    {
        str = "notify";
    }
    else if (value == BT_GATT_CCC_INDICATE)
    {
        str = "indicate";
    }

    LOGA(BLE, "[BLE] ccc change %s\r\n", str);
}

static void _connected(struct bt_conn *conn, u8_t err)
{
    if (conn_cb)
    {
        if (conn_cb(conn, err) != 0)
        {
            return;
        }
    }

    if (conn->type != BT_CONN_TYPE_LE)
    {
        return;
    }

    conn_cur = conn;
    ble_connect_status = true;
    LOGA(BLE, "[BLE] connected \r\n");
    BleSetMtu();
    return;
}

static void _disconnected(struct bt_conn *conn, u8_t reason)
{
    if (disconn_cb)
    {
        if (disconn_cb(conn, reason) != 0)
        {
            return;
        }
    }

    if (conn->type != BT_CONN_TYPE_LE)
    {
        return;
    }

    conn_cur = NULL;
    ble_connect_status = false;
    LOGA(BLE, "[BLE] disconnected, reason:%d \r\n", reason);
}

static bool _le_param_req(struct bt_conn *conn,
                          struct bt_le_conn_param *param)
{
    LOGA(BLE, "[BLE] conn param request: int 0x%04x-0x%04x lat %d to %d \r\n",
           param->interval_min,
           param->interval_max,
           param->latency,
           param->timeout);

    return true;
}

static void _le_param_updated(struct bt_conn *conn, u16_t interval,
                              u16_t latency, u16_t timeout)
{
    LOGA(BLE, "[BLE] conn param updated: int 0x%04x lat %d to %d \r\n", interval, latency, timeout);
}

static void _le_phy_updated(struct bt_conn *conn, u8_t tx_phy, u8_t rx_phy)
{
    LOGA(BLE, "[BLE] phy updated: rx_phy %d, rx_phy %d \r\n", tx_phy, rx_phy);
}

static struct bt_conn_cb conn_callbacks = {
    .connected = _connected,
    .disconnected = _disconnected,
    .le_param_req = _le_param_req,
    .le_param_updated = _le_param_updated,
    .le_phy_updated = _le_phy_updated,
};

static void ble_disconnect_all(struct bt_conn *conn, void *data)
{
    if (conn->state == BT_CONN_CONNECTED)
    {
        LOGA(BLE, "[BLE] disconn id:%d \r\n", conn->id);
        bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    }
}

static void _ble_mtu_changed_cb(struct bt_conn *conn, int mtu)
{
    LOGA(BLE, "[BLE] mtu updated:%d \r\n", mtu);
}

struct bt_conn *ble_get_conn_cur(void)
{
    return conn_cur;
}

int ble_regist_conn(ble_gatt_conn_cb_t cb)
{
    conn_cb = cb;

    return 0;
}

int ble_regist_disconn(ble_gatt_conn_cb_t cb)
{
    disconn_cb = cb;

    return 0;
}

static int ble_salve_conn_cb(struct bt_conn *conn, uint8_t code)
{
    int err;

    struct bt_le_conn_param param;
    param.interval_max = 24;
    param.interval_min = 24;
    param.latency = 0;
    param.timeout = 600;
    err = bt_conn_le_param_update(conn, &param);

    return 0;
}

static int ble_salve_disconn_cb(struct bt_conn *conn, uint8_t code)
{
    if (set_adv_enable(true))
    {
        LOGA(BLE, "[BLE] Restart adv fail. \r\n");
    }
    else
    {
        LOGA(BLE, "[BLE] Restart adv success. \r\n");
    }

    return 0;
}

int ble_salve_adv()
{
    int err = -1;
    err = bt_le_adv_start(BT_LE_ADV_CONN, salve_adv, ARRAY_SIZE(salve_adv), NULL, 0);
    if (err)
    {
        LOGA(BLE, "[BLE] adv fail(err %d) \r\n", err);
        return -1;
    }

    return 0;
}

static void bt_enable_cb(int err)
{
    if (!err)
    {
        bt_addr_le_t bt_addr;
        bt_get_local_public_address(&bt_addr);
        bt_addr.a.val[5] = 0x88;
        bt_addr.a.val[4] = 0x88;
        bt_addr.a.val[3] = 0x88;
        bt_addr.a.val[2] = 0x88;
        bt_addr.a.val[1] = 0x88;
        bt_addr.a.val[0] = 0x88;
        LOGA(BLE, , "BD_ADDR:(MSB)%02x:%02x:%02x:%02x:%02x:%02x(LSB) \r\n",
               bt_addr.a.val[5], bt_addr.a.val[4], bt_addr.a.val[3], bt_addr.a.val[2], bt_addr.a.val[1], bt_addr.a.val[0]);
    }
}

void ble_reverse_byte(uint8_t *arr, uint32_t size)
{
    uint8_t i, tmp;

    for (i = 0; i < size / 2; i++)
    {
        tmp = arr[i];
        arr[i] = arr[size - 1 - i];
        arr[size - 1 - i] = tmp;
    }
}

int ble_uuid1_notify_data(void *handle, void *data, uint16_t length)
{
    int ret;
    uint16_t mtu;
    uint16_t offset;
    uint16_t send_len;

    offset = 0;
    mtu = bt_gatt_get_mtu(handle) - 3;
    // LOGA(BLE, "[BLE] Current MTU: %d\r\n", mtu + 3); // Print actual MTU size
    while (length > 0)
    {
        /* calculate send_len */
        send_len = length > mtu ? mtu : length;
        /* send data */
        ret = bt_gatt_notify(handle, &salve_uuid1_server[SALVE_CMD_SERVER_TX_INDEX], data + offset, send_len);
        /* set offset */
        offset += send_len;
        length -= send_len;

        LOGA(BLE, "[BLE] notify len:%d \r\n", send_len);
        // LOGA(BLE, "[BLE] Notified data: %.*s\r\n", send_len, (char*)(data + offset - send_len)); // Print the chunk being sent

        if (ret != 0)
        {
            break;
        }
    }

    return ret;
}

int ble_uuid2_notify_data(void *handle, void *data, uint16_t length)
{
    int ret;
    uint16_t mtu;
    uint16_t offset;
    uint16_t send_len;

    offset = 0;
    mtu = bt_gatt_get_mtu(handle) - 3;
    while (length > 0)
    {
        /* calculate send_len */
        send_len = length > mtu ? mtu : length;
        /* send data */
        ret = bt_gatt_notify(handle, &salve_uuid2_server[SALVE_CMD_SERVER_TX_INDEX], data + offset, send_len);
        /* set offset */
        offset += send_len;
        length -= send_len;

        LOGA(BLE, "[BLE] notify len:%d \r\n", send_len);

        if (ret != 0)
        {
            break;
        }
    }

    return ret;
}

// Send data to Bluetooth UUID1 service in slave mode
// Parameters
//     len: Length of the data to be sent
//     data: Data to be sent
// Return value
//     >=0: Length of data successfully sent
//     -1: Bluetooth status error
//     -2: Data length error
//     -3: data is NULL
//     -4: Sending failed
int UUID1_SendNotify(uint16_t len, uint8_t *data)
{
    int ret;
    struct bt_conn *conn;

    conn = ble_get_conn_cur();
    if (conn == NULL)
    {
        return -1;
    }
    // LOGA(BLE, "UUID1_SendNotify: %s\r\n",data);
    ret = ble_uuid1_notify_data(conn, (void *)data, len);
    if (ret != 0)
    {
        return -4;
    }

    return len;
}

// Send data to Bluetooth UUID2 service in slave mode
// Parameters
//     len: Length of the data to be sent
//     data: Data to be sent
// Return value
//     >=0: Length of data successfully sent
//     -1: Bluetooth status error
//     -2: Data length error
//     -3: data is NULL
//     -4: Sending failed
int UUID2_SendNotify(uint16_t len, uint8_t *data)
{
    int ret;
    struct bt_conn *conn;

    conn = ble_get_conn_cur();
    if (conn == NULL)
    {
        return -1;
    }

    ret = ble_uuid2_notify_data(conn, (void *)data, len);
    if (ret != 0)
    {
        return -4;
    }

    return len;
}

static void exchange_func(struct bt_conn *conn, u8_t err,
                          struct bt_gatt_exchange_params *params)
{
    uint16_t ret_bt;
    if (conn)
    {
        ret_bt = bt_gatt_get_mtu(conn);
        // LOGA(BLE, "[BLE] Exchange %s MTU Size =%d \r\n", err == 0U ? "successful" : "failed", ret_bt);
    }
}

static struct bt_gatt_exchange_params exchange_params;

uint8_t BleSetMtu()
{
    int ret = -1;
    if (conn_cur == NULL)
    {
        return 1;
    }

    exchange_params.func = exchange_func;
    ret = bt_gatt_exchange_mtu(conn_cur, &exchange_params);
    if (ret != 0)
    {
        return 1;
    }

    return 0;
}

int ble_slave_init()
{

    ble_regist_conn(ble_salve_conn_cb);
    ble_regist_disconn(ble_salve_disconn_cb);

    ble_server_init();
    ble_salve_adv();

    return 0;
}

int ble_slave_deinit(void)
{
    bt_le_adv_stop();
    // ble_server_deinit();
    ble_regist_conn(NULL);
    ble_regist_disconn(NULL);

    return 0;
}

int ble_server_init()
{
    int ret = 0;

    ret = bt_gatt_service_register(&ble_uuid1_server);
    ret |= bt_gatt_service_register(&ble_uuid2_server);

    return ret;
}

int ble_server_deinit(void)
{
    int ret = 0;

    ret = bt_gatt_service_unregister(&ble_uuid1_server);
    ret |= bt_gatt_service_unregister(&ble_uuid2_server);

    return ret;
}

void ble_stack_start(void)
{
    // Initialize BLE controller
    // LOGA(BLE, "Check Initialize BLE controller\r\n");
    ble_controller_init(configMAX_PRIORITIES - 1);
    // Initialize BLE Host stack
    hci_driver_init();
    bt_enable(bt_enable_cb);
}

// Turn on Bluetooth
// start ble
void apps_ble_start()
{
    LOGA(BLE, "debug ble start \r\n");
    ble_start_flag = true;
    //initialize name for switch
    generate_ble_name();

    ble_stack_start();
    ble_slave_init();
    bt_gatt_register_mtu_callback(_ble_mtu_changed_cb);
    bt_conn_cb_register(&conn_callbacks);
    /* avoid callback infinite loop */
    conn_callbacks._next = NULL;
}

// Turn off Bluetooth 
// stop ble
void apps_ble_stop()
{
    ble_start_flag = false;
    ble_slave_deinit();

    bt_conn_foreach(BT_CONN_TYPE_ALL, ble_disconnect_all, NULL);

    int disconn_cnt = 0;
    while (le_check_valid_conn() && disconn_cnt++ < 10)
    {
        LOGA(BLE, "[BLE] wait for ble_disconnect_all\r\n");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    bt_disable();
}

extern char ble_data[256] = {0};

char *Read_BLE(char *data)
{
    strncpy(ble_data, data, sizeof(ble_data) - 1);
    ble_data[sizeof(ble_data) - 1] = '\0';
    return ble_data;
}

char *Get_BLE_Data()
{
    // if(strlen(ble_data) == 0)
    // {
    //     return strdup("");
    // }
    char *temp = strdup(ble_data);
    // char *temp = ble_data;
    memset(ble_data, 0, sizeof(ble_data));
    return temp;
}



bool ble_connect_status = false;

void led_control(char *command)
{
    int Led_Pin = 17;
    bl_gpio_enable_output(Led_Pin, 0, 0);

    if(strcmp(command, "On")==0 || strcmp(command, "1")==0)
    {
        bl_gpio_output_set(Led_Pin, 1);
        LOGA(BLE, "Led On\r\n");
    }
    else if (strcmp(command, "Off")==0 || strcmp(command, "0")==0)
    {
        bl_gpio_output_set(Led_Pin, 0);
        LOGA(BLE, "Led Off\r\n");
    }
    // if (sizeof(com and) >= 1)
    // {
    // }
}
void send_ble_message(const char *message){
    // printf("Sending BLE message: %s\n", message);  // Debug print
    // UUID1_SendNotify(strlen(message), (uint8_t *)message);  // Ensure null-terminated text
    if(UUID1_SendNotify(strlen(message), (uint8_t *)message) >= 0){
        LOGA(BLE, "Send feedback to BLE successfully\r\n");
    }
    else{
        LOGA(BLE, "Send feedback to BLE failed\r\n");
    }
}

void generate_ble_name() {
    char last_3digit[4]; // Buffer to store last 3 digits of gdevice_id
    int len = strlen(gdevice_id);
    
    snprintf(last_3digit, 4, "%s", &gdevice_id[len-3]); // Get last 3 digits of the device_id
    snprintf(ble_slave_name, sizeof(ble_slave_name), "%s-%s", TYPEDEVICE, last_3digit);
    LOGA(OTA, "gdevice_id[%d]=(%s)/last_3digit=(%s)/ble_slave_name=(%s)\r\n", len, gdevice_id, last_3digit, ble_slave_name);
    // printf("Generated BLE Name: %s\n", ble_slave_name);  // Debug output
}
