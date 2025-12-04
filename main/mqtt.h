#ifndef MQTT_H_
#define MQTT_H_

#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <mqtt_client.h>
#include "blog.h"
#include <bl_gpio.h>
#include "nami_ir.h"
// #include "storage.h"
#include <stdint.h>

#include <string.h>
#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/md5.h"
#include "mbedtls/debug.h"
#include "mbedtls/x509_crt.h"
// #include "switch.h"
#include "ota.h"
#include "wifi_execute.h"
// #include "storage.h"
#include <cJSON.h>
#include "bl602_ir.h"
#include "plog.h"


#include "ota_parse.h"
#include "ai_ota.h"
#include "ota_config.h"
#include "ota_hal.h"

#include "nami_ir_recv.h"


// #define DETTECT_IR_PULSE

#define MQTT_BUFFER
#define CHAIN_LATCH 0
#define KEY_NUMBER 1
#define STATUS 1


#define NAMI_ENABLE                     100
#define NAMI_DISABLE                    103
#define NAMI_CONNECTED                  99
#define NAMI_DISCONNECT                 98
#define NAMI_SAVE                       97
#define NAMI_OFF_SAVE                   10  

// typedef enum{

// } mqtt_ir_mode_t;

typedef enum{
	LATCH,
	SETTING,
	GETTING,
	MODIFY,
	LEARNING,
	RANGE_DELAY,
	ERR_NUM_COMPARE,
    SEND_OK,
	SEND_OK_COUNT_FRAME,
    SEND_UPDATE,
    SEND_FAIL,
    NORMAL,
    NEWVER,
	CHAIN_ZERO,
	NO_BEST_DIV,
	NOISE_BUTTON,
	NUMBER_CALIB_COMPARE,
	SETVER,
	UDFSUCCESS,
	UDFCOMPLETE,
	MAC_ADD,
	TEMPHUMID,
	RESET_BUTTON,
	PARA,
	SEND_LEAR = 200
}mqtt_rsp_e;

typedef enum{
	LEVEL_LOW,
	LEVEL_MEDIUM,
	LEVEL_HIGH,
	LEVEL_GET
}mqtt_range_delay_e;

typedef enum{
	POSA,
	POSB,
	POSITION_A,
	POSITION_B,
}mqtt_range_delay_number_e;

typedef enum{
	ELEARN = 1,
	ELISTEN
}mqtt_ir_e;


#define TYPEDEVICE 												"ir"
#define TYPEDEVICEMQTT										    "\"device_type\""

#define FIRMWAREVERSION 										"1.0.74-release"
#define FIRMWAREVERSIONMQTT										"\"firmware_version\""

#define FIRMWARECURRVERSIONMQTT									"\"firmware_curr_version\""

#define LEARNFORMAT												"{\"ir\" :\"IR:0,0,1,"
#define LEARNFORMATEND											"\"}"


/*
// #define TEST_FULL_CERTIFICATE \
// "-----BEGIN CERTIFICATE-----\n" \
// "MIIDojCCAyegAwIBAgISBPiVHuhtRKZHdPYIdJBC4XRGMAoGCCqGSM49BAMDMDIx\n" \
// "CzAJBgNVBAYTAlVTMRYwFAYDVQQKEw1MZXQncyBFbmNyeXB0MQswCQYDVQQDEwJF\n" \
// "NTAeFw0yNDExMzAwNTIzMDlaFw0yNTAyMjgwNTIzMDhaMBwxGjAYBgNVBAMTEW1x\n" \
// "dHQubmFtaXNtYXJ0LnZuMHYwEAYHKoZIzj0CAQYFK4EEACIDYgAEr8AMZgya5sjw\n" \
// "x20Lmtewj0B6DhQPw7acdVlisJFsDbu34EEdz5d5n1xeGLpzQMcLP2xzVUMvGrVp\n" \
// "rH3Dvj5Zval0aNsNYEIpCQNF5ffvevZdCGIu41N/FutxCF67qmKSo4ICFDCCAhAw\n" \
// "DgYDVR0PAQH/BAQDAgeAMB0GA1UdJQQWMBQGCCsGAQUFBwMBBggrBgEFBQcDAjAM\n" \
// "BgNVHRMBAf8EAjAAMB0GA1UdDgQWBBQxa0/iVJdsjIYqg7E1nZvPoDZYhDAfBgNV\n" \
// "HSMEGDAWgBSfK1/PPCFPnQS37SssxMZwi9LXDTBVBggrBgEFBQcBAQRJMEcwIQYI\n" \
// "KwYBBQUHMAGGFWh0dHA6Ly9lNS5vLmxlbmNyLm9yZzAiBggrBgEFBQcwAoYWaHR0\n" \
// "cDovL2U1LmkubGVuY3Iub3JnLzAcBgNVHREEFTATghFtcXR0Lm5hbWlzbWFydC52\n" \
// "bjATBgNVHSAEDDAKMAgGBmeBDAECATCCAQUGCisGAQQB1nkCBAIEgfYEgfMA8QB3\n" \
// "AObSMWNAd4zBEEEG13G5zsHSQPaWhIb7uocyHf0eN45QAAABk3u7ZncAAAQDAEgw\n" \
// "RgIhAKJRAyrqiGu3M/tXY9rq3Ix6CIj2pgaUwR0CRAeAtoGjAiEA8WVSKqGq/Gaq\n" \
// "fXtI0wrreVpq9Tfk2T1ljmt50cw31lkAdgCi4wrkRe+9rZt+OO1HZ3dT14JbhJTX\n" \
// "K14bLMS5UKRH5wAAAZN7u2ZuAAAEAwBHMEUCIQD251talWelyFcz5Y24zXEoIzOp\n" \
// "mzyJGQ8dg2XF/X9UvQIgehhdiuRy06OBlE6N53wjfVveEbuPN6zIdnUS/ZctzOEw\n" \
// "CgYIKoZIzj0EAwMDaQAwZgIxAJnegzHNKDPFUVa+/D+MjLfc0avm4SHktuANggD+\n" \
// "6kW/Lp3qu/6xjmZVjP3p8rDhBAIxAKT9qRS47w6erhdVi53WHnIfQFNb2oA0qmIu\n" \
// "8PjVE5s27vJWU/q+ENJEftzT+JuyTg==\n" \
// "-----END CERTIFICATE-----\n"


// #define TT_FULL_CERTIFICATE \
// "-----BEGIN CERTIFICATE-----\n" \
// "MIIDoTCCAyagAwIBAgISA5GNaEk5FD1N1SrKEVgv1rqYMAoGCCqGSM49BAMDMDIx\n" \
// "CzAJBgNVBAYTAlVTMRYwFAYDVQQKEw1MZXQncyBFbmNyeXB0MQswCQYDVQQDEwJF\n" \
// "NTAeFw0yNDExMzAwNTMxMjhaFw0yNTAyMjgwNTMxMjdaMBwxGjAYBgNVBAMTEW1x\n" \
// "dHQubmFtaXNtYXJ0LnZuMHYwEAYHKoZIzj0CAQYFK4EEACIDYgAEFHF2udD7TBAE\n" \
// "RB+zqoP0PhGZnWfJ7XeOy2viddZ+dxYn26GsnqSphSTMLmiGne1gWzXFiz1xjllJ\n" \
// "YVbieAA3oRdZh/L6bI9Xh+NTBWLw8blKNVGxpOvJyxkGh5tgCH1Eo4ICEzCCAg8w\n" \
// "DgYDVR0PAQH/BAQDAgeAMB0GA1UdJQQWMBQGCCsGAQUFBwMBBggrBgEFBQcDAjAM\n" \
// "BgNVHRMBAf8EAjAAMB0GA1UdDgQWBBQ5rjwRiwnvRO/Ek863LscLY5nodDAfBgNV\n" \
// "HSMEGDAWgBSfK1/PPCFPnQS37SssxMZwi9LXDTBVBggrBgEFBQcBAQRJMEcwIQYI\n" \
// "KwYBBQUHMAGGFWh0dHA6Ly9lNS5vLmxlbmNyLm9yZzAiBggrBgEFBQcwAoYWaHR0\n" \
// "cDovL2U1LmkubGVuY3Iub3JnLzAcBgNVHREEFTATghFtcXR0Lm5hbWlzbWFydC52\n" \
// "bjATBgNVHSAEDDAKMAgGBmeBDAECATCCAQQGCisGAQQB1nkCBAIEgfUEgfIA8AB2\n" \
// "AH1ZHhLheCp7HGFnfF79+NCHXBSgTpWeuQMv2Q6MLnm4AAABk3vDBB4AAAQDAEcw\n" \
// "RQIgRbqdMYr1ILoK5mEQwjbBTA0SIwbTumi26KXige+nACoCIQC0WAE6Whm8Jtaz\n" \
// "r04Ss7wkr/3rKM3MT7NhW8SyzKDzaAB2AM8RVu7VLnyv84db2Wkum+kacWdKsBfs\n" \
// "rAHSW3fOzDsIAAABk3vDBFIAAAQDAEcwRQIhALfFe4t1IYQiJIQa/rXxFTfYFLZV\n" \
// "Mqyk43mzsYk5gSFwAiBbxlBjiiVfZs6tLh+HpdbGXC3zryFhHzYr18C8talRXzAK\n" \
// "BggqhkjOPQQDAwNpADBmAjEA2lL60UdsIpqUt2I8AxeU7CDq0zeYJcYX0tiVStt9\n" \
// "xL6op6nP0Pbvh0gXL6FegzNSAjEArAIueWLCP5jZFpUjBNaor9H4dFltbcRoLsm6\n" \
// "UPjgaiFfoKTbXM7TF8k52cMpac8G\n" \
// "-----END CERTIFICATE-----\n"
*/



#define M_CERTIFICATE "-----BEGIN CERTIFICATE-----\r\n"\
"MIIEAzCCAuugAwIBAgIUBY1hlCGvdj4NhBXkZ/uLUZNILAwwDQYJKoZIhvcNAQEL\r\n"\
"BQAwgZAxCzAJBgNVBAYTAkdCMRcwFQYDVQQIDA5Vbml0ZWQgS2luZ2RvbTEOMAwG\r\n"\
"A1UEBwwFRGVyYnkxEjAQBgNVBAoMCU1vc3F1aXR0bzELMAkGA1UECwwCQ0ExFjAU\r\n"\
"BgNVBAMMDW1vc3F1aXR0by5vcmcxHzAdBgkqhkiG9w0BCQEWEHJvZ2VyQGF0Y2hv\r\n"\
"by5vcmcwHhcNMjAwNjA5MTEwNjM5WhcNMzAwNjA3MTEwNjM5WjCBkDELMAkGA1UE\r\n"\
"BhMCR0IxFzAVBgNVBAgMDlVuaXRlZCBLaW5nZG9tMQ4wDAYDVQQHDAVEZXJieTES\r\n"\
"MBAGA1UECgwJTW9zcXVpdHRvMQswCQYDVQQLDAJDQTEWMBQGA1UEAwwNbW9zcXVp\r\n"\
"dHRvLm9yZzEfMB0GCSqGSIb3DQEJARYQcm9nZXJAYXRjaG9vLm9yZzCCASIwDQYJ\r\n"\
"KoZIhvcNAQEBBQADggEPADCCAQoCggEBAME0HKmIzfTOwkKLT3THHe+ObdizamPg\r\n"\
"UZmD64Tf3zJdNeYGYn4CEXbyP6fy3tWc8S2boW6dzrH8SdFf9uo320GJA9B7U1FW\r\n"\
"Te3xda/Lm3JFfaHjkWw7jBwcauQZjpGINHapHRlpiCZsquAthOgxW9SgDgYlGzEA\r\n"\
"s06pkEFiMw+qDfLo/sxFKB6vQlFekMeCymjLCbNwPJyqyhFmPWwio/PDMruBTzPH\r\n"\
"3cioBnrJWKXc3OjXdLGFJOfj7pP0j/dr2LH72eSvv3PQQFl90CZPFhrCUcRHSSxo\r\n"\
"E6yjGOdnz7f6PveLIB574kQORwt8ePn0yidrTC1ictikED3nHYhMUOUCAwEAAaNT\r\n"\
"MFEwHQYDVR0OBBYEFPVV6xBUFPiGKDyo5V3+Hbh4N9YSMB8GA1UdIwQYMBaAFPVV\r\n"\
"6xBUFPiGKDyo5V3+Hbh4N9YSMA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEL\r\n"\
"BQADggEBAGa9kS21N70ThM6/Hj9D7mbVxKLBjVWe2TPsGfbl3rEDfZ+OKRZ2j6AC\r\n"\
"6r7jb4TZO3dzF2p6dgbrlU71Y/4K0TdzIjRj3cQ3KSm41JvUQ0hZ/c04iGDg/xWf\r\n"\
"+pp58nfPAYwuerruPNWmlStWAXf0UTqRtg4hQDWBuUFDJTuWuuBvEXudz74eh/wK\r\n"\
"sMwfu1HFvjy5Z0iMDU8PUDepjVolOCue9ashlS4EB5IECdSR2TItnAIiIwimx839\r\n"\
"LdUdRudafMu5T5Xma182OC0/u/xRlEm+tvKGGmfFcN0piqVl8OrSPBgIlb+1IKJE\r\n"\
"m/XriWr/Cq4h/JfB7NTsezVslgkBaoU=\r\n"\
"-----END CERTIFICATE-----\r\n"


extern void submit_gpio_relay_status_to_mqtt_server(uint8_t status);
void handle_control_switch(const char *event_data);
void handle_control_switch_ir(const char *event_data);
void handle_switch_status();
void handle_topic(const char *event_topic, const char *event_data);
void mqtt_start(void *pvParameters);
extern void NamiUpdateMQTT(void);
extern uint32_t MQTTIRLearn(uint8_t LearnOrListen);
extern uint16_t NamiMQTTConvertUintToString(uint16_t *Data, uint32_t Counter);


#define MAC_ADDRESS_LENGTH 6
#define DEVICE_ID_LENGTH 33 // Length for the Device ID (32 + null)
#define VERIFICATION_CODE_LENGTH 6 // Length for the Verification Code (5 + null)

extern char gdevice_id[DEVICE_ID_LENGTH];
extern char gverification_code[VERIFICATION_CODE_LENGTH];
extern uint8_t gdevice_status;
extern char gmac[18];

#define LEARNSIZE         4
#define GENSTRINGSIZE     1500

typedef struct{
	uint8_t KeyNumber;
	uint8_t Status;
	bool    SendStatusFlag;
	uint16_t DataIRBuffer[1024];//[1024];
    uint16_t DataIRBufferIndex;
    uint8_t LearnBuffer[2000];//[400];
	uint32_t LearnCount;
	uint16_t GenBuffer[GENSTRINGSIZE];//[1000];
	uint32_t GenCounter;

	bool ModeTimeoutFlag;

	uint16_t NumCalib[2][3];
	uint16_t RangeDelayBuffer[3][2];
	bool RangeDelayFlag;
	bool CalibNumberAFlag;
	bool CalibNumberBFlag;
	bool CalibNumberGetFlag;
	bool ReSendCalibNumberGetFlag;

	//uint16_t *LearnPtr;
	//uint32_t *CountPtr;
	uint8_t UpdateFirmwareFlag;
	uint8_t *StatusLogFlag;
	uint8_t *UpdateFWbuff;
	uint8_t RawPercent;
	uint8_t StatusBroker;
	char mac_str[20];
	char CheckVer[20];

	uint8_t StateUDFFlag;
	bool StateLearnFlag;
	uint8_t CntFrame;
	char Host[50];

	uint8_t StatusBuffer[20];

	// TaskHandle_t MQTTTaskHandle;

	void (*NamiMQTTIRControlSend)(int);
	// void (*NamiMQTTSuspendIR)(void);
	uint8_t (*NamiMQTTMergeIR)(uint16_t *, uint16_t );
	//void (*NamiMQTTLearning)(void);
	uint32_t (*NamiMQTTLearning)(void);  
	//uint32_t (*NamiMQTTLearning)(IR_RxMode_Type , uint32_t *);
	// void (*NamiMQTTControlTask)(IR_SEND_STATUS );
	void (*IR_Init)(uint8_t );
	uint32_t (*IR_LearnToReceive)(IR_RxMode_Type , uint16_t *, bool );
}nami_mqtt_t;

extern nami_mqtt_t NamiMQTT;

extern uint8_t NeedToLearn;
extern uint8_t LearnTimeout;
extern bool gmqtt_connected;
extern TaskHandle_t MQTTTaskHandle;

#endif /* MQTT_H_ */
