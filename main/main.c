#include "main.h"

general_t Gen;

hosal_uart_dev_t UARTLOG = {
    .config = {
        .uart_id             = 0,
        .tx_pin              = 16, // TXD GPIO
        .rx_pin              = 7,  // RXD GPIO
        .cts_pin             = 255,
        .rts_pin             = 255,
        .baud_rate           = 115200,
        .data_width          = NAMI_OPERATE_DATA_WIDTH(NAMI_DATA_WIDTH_8BIT),
        .parity              = NAMI_OPERATE_PARITY(NAMI_NO_PARITY),
        .stop_bits           = NAMI_OPERATE_STP_BITS(NAMI_STOP_BITS_1),
        .mode                = NAMI_OPERATE_UART_MODE(NAMI_UART_MODE_POLL),
    },
};



void main()
{
    // Initialize EasyFlash first
    if (easyflash_init() != EF_NO_ERR) {
        printf("[MAIN] Critical Error: Failed to initialize EasyFlash!\n");
        while(1); // Halt if flash init fails
    }

    NAMI_SYSTEM_INIT();
	NAMI_UART_INIT(&UARTLOG);
	PLOG_Init();
	PLOG_Stop(ALL);

	printf("\r\n"
            "\033[0;32m*************************************************** \r\n"
            "\033[1;31m       NAMI TECHNOLOGIES JOINT STOCK COMPANY        \r\n"
            "\033[0;36m       Project: \033[1;31mSmart Switch InfraRed     \r\n"
            "\033[0;36m       Author: \033[1;31mR&D Firmware Team          \r\n"
            "\033[0;36m       Date of time: \033[1;31mApril 09, 2025       \r\n"
            "\033[0;36m       Version: \033[1;31m251203_01_main            \r\n"
            "\033[0;32m*************************************************** \r\n\033[0;37m"
            "\033[38;5;15m \033[0m\n\n");


	initialize_switch();
	LedNotifyStartPro();

	tcpip_init(NULL, NULL);

	xTaskCreate(NamiInitRecv     , "_NamiInitRecv"     , 1024    , NULL, 14, &NamiIRRecvTaskHandle);
	xTaskCreate(InitNamiIR       , "InitNamiIR"        , 2048    , NULL, 14, &NamiIRTaskHandle);  
	xTaskCreate(wifi_execute     , "wifi execute"      , 1024*2  , NULL, 14, NULL);
	xTaskCreate(mqtt_start       , "mqtt task"         , 1024    , NULL, 14, &MQTTTaskHandle);
    xTaskCreate(relay_event_task , "relay_event_task"  , 640     , NULL, 14, &RelayEventTaskHandle); // Increased stack to 2048
}
