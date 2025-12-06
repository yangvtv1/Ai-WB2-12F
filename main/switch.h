#ifndef _SWITCH_H
#define _SWITCH_H

#include "FreeRTOS.h"
#include "portmacro.h"
#include "event_groups.h"
#include "lwip/err.h"
#include "string.h"
#include <time.h>  // Make sure this header is included
#include <hosal_rtc.h>
#include "ota.h"
#include "stdbool.h"
//#include <storage.h>

#include <stdio.h>
#include <task.h>
#include <queue.h>
#include <bl_gpio.h>
#include "http_client.h"
#include "wifi_execute.h"
// #include "time_rtc.h"
#include "switch.h"
// #include "storage.h"
#include "mqtt.h"
#include "bluetooth.h"
#include "ble_interface.h"
#include "cJSON.h"
#include "hosal_gpio.h"
#include "timers.h"

struct nami_tm {
    uint8_t tm_sec;
    uint8_t tm_min;
    uint8_t tm_hour;
    uint8_t tm_mday;
    uint8_t tm_mon;
    uint8_t tm_year;
    uint8_t tm_wday;
    uint8_t tm_yday;
    uint8_t tm_isdst;
  };


#define RELAY_COUNT 1
#define RELAY_SCHEDULE 1

#define LED_ON 0
#define LED_OFF 1
#define RELAY_ON 1
#define RELAY_OFF 0

#define HIGH 1
#define LOW 0

#define DEBOUNCE_TIME_MS 50*4  // 50ms debounce period


#define GPIO_LED_NOTIFY     14
#define GPIO_RELAY_OUT4     5//4 //Relay2
#define GPIO_KEY_IN0        17//3 //PB1
#define GPIO_KEY_IN12        12//3 //PB1



#define CALCUALATE_TIME(x)                              ((xTaskGetTickCount() * portTICK_PERIOD_MS) - x)




// #define GPIO_LED_OUT12 5
// #define GPIO_LED_OUT12 40    //LED1_STATUS
// #define GPIO_LED_OUT14 50
// #define GPIO_LED_OUT17 60

// #define GPIO_LED1_STATUS 12


extern uint8_t gpio_status_relay3;
extern uint8_t gpio_status_relay4;
extern uint8_t gpio_status_relay5;
extern uint8_t gpio_status_relay6;

// test truct
// Struct to hold on/off times for a schedule
typedef struct {
    // struct nami_tm on_time;  // Time when the relay should turn on
    // struct nami_tm off_time; // Time when the relay should turn off
    struct nami_tm timer; // Time when the relay should operate
    uint8_t action; // 1 for ON, 0 for OFF
    uint8_t repeat; // 1 for repeat, 0 for one-time
    uint8_t dayofweek[7];  // Array to store which days are enabled (0=disabled, 1=enabled)
    uint8_t valid;         // 1 if schedule is valid, 0 if cancelled
} alarm_test_schedule_t;
// Struct to hold the two schedules for a relay
typedef struct {
    alarm_test_schedule_t schedule[RELAY_SCHEDULE]; // schedule
} relay_test_schedule_t;
// end test struct

// test truct
typedef struct {
    uint8_t gpio_status_relay3;   // State for GPIO3 relay
    uint8_t gpio_status_relay4;   // State for GPIO4
    uint8_t gpio_status_relay5;   // State for GPIO5
    uint8_t gpio_status_relay6;   // State for GPIO6
    relay_test_schedule_t schedule_relays[RELAY_COUNT]; // 3 relays, each with 2 schedules
    uint8_t led_conf_status; // configure led status: ON(1)/OFF(0)
} gpio_test_switch_info_t;

//end test struct
typedef struct{
    uint8_t gpio;
    uint8_t lastState;
} gpio_switch;

typedef struct{
    uint8_t *SwitchStatusBroker;
    uint8_t SetCount;
    uint16_t Duration;
    uint8_t StateCurrent;
    uint8_t StateOld;
    uint8_t SaveWifiFlag;
    char    *SwMacStr;

    bool TimerStartFlag;
    bool TimerStopFlag;
    TimerHandle_t TimerSwitchHandler;
    bool ChangeCalibFlag;
    bool ReSetFlag;
    bool ModeCalibFlag;
    bool ExitCalibrateFlag;
    bool BreakOutCalibrateFlag;
}switch_t;

extern switch_t NamiSwitch;

extern char ip_str[16];

void check_wifi_and_update_rtc(uint32_t interval);
int update_led_conf_status(uint8_t led_conf_status);
void update_gpio_led_status(uint8_t gpio_out, uint8_t status);
void update_gpio_out_status(uint8_t gpio_out, uint8_t status);
void update_gpio_relay_status(uint8_t gpio_out, uint8_t status);
void control_gpio_switch(uint8_t gpio_out, uint8_t on_off);
void my_gpio_interrupt_handler(void *arg);
void enable_gpio_interrupt(uint8_t gpio_in);
void initilize_relay_state(uint8_t gpio_relay_out, uint8_t gpio_led_out, uint8_t status);
extern void initialize_switch();
extern void LedNotifyStartPro(void);
int get_alarm_schedule(struct tm current_time, uint8_t relay_index, uint8_t schedule_id);
int set_alarm_schedule(bool store_on_flash, uint8_t relay_index, uint8_t schedule_index, alarm_test_schedule_t *new_schedule);
void check_alarm_for_switch(hosal_rtc_time_t rtc_time);
void check_alarm_for_relay(hosal_rtc_time_t rtc_time, uint8_t gpio_relay_out);
int cancel_alarm_schedule(uint8_t relay_index, uint8_t schedule_index);
void setting_mode_task(void *pvParameters);
void ble_connect_wifi(char *received_data);

extern bool setting_mode_enable;
extern int8_t connecting_to_other_wifi;

//----------function for main-----------
void proc_alarm_for_switch(void *pvParameters);
void proc_interrupt_for_switch(void *pvParameters);
void switch_control(void *pvParameters);
void ble_switch_control(void *pvParameters);
void relay_event_task_ext(void *pvParameters);
void relay_event_task(void *pvParameters);
void duration_trigger_mode(TickType_t press_duration);

void key1_irq_pos(void *arg);
void key1_irq_neg(void *arg);
// void key1_irq(void *arg);
void key2_irq(void *arg);
void key3_irq(void *arg);
void key4_irq(void *arg);
void gpio_irq_init();
extern TaskHandle_t relay_event_task_handle;

#endif // SWITCH_H


