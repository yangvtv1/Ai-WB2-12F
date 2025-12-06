/*----------------------------------------------------
 *    Copyright (c) 2023
 *
 *File 					: plog.c
 *Created on		: Dec 2020
 *Author				: Luong_Thanh_Hai
 *Contact		    : elvis.luong@gmail.com
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/
#include "plog.h"
//#include "drv_common.h"

///***** libraries GIGA ******/
//#include "gd32f30x_rcu.h"
//#include "gd32f30x_adc.h"
//#include "gd32f30x_exti.h"
//#include "gd32f30x_fmc.h"
//#include "gd32f30x_fwdgt.h"
//#include "gd32f30x_gpio.h"
//#include "gd32f30x_i2c.h"
//#include "gd32f30x_pmu.h"
//#include "gd32f30x_bkp.h"
//#include "gd32f30x_rtc.h"
//#include "gd32f30x_spi.h"
//#include "gd32f30x_timer.h"
//#include "gd32f30x_usart.h"
/***************************/
 /******************************************************************************/
/******************************************************************************/
/***                      Private functions declare                          **/
/******************************************************************************/
/******************************************************************************/
#define PLOG_TIMEOUT_RX	(100)//ms
void PLOG_Start(type_debug_t _type);
//void PLOG_Stop(type_debug_t _type);
void Plog_callback(void *arg);
bool bool_dbg_fnc(char *_input);
//void _set_plog (bool _mode,type_debug_t _type);
void _set_plog (type_debug_t _type,void* arg);
void _process_cmd (type_debug_t _type,void* arg);
void _Simulate(type_debug_t _type,void* arg);
void _help(type_debug_t _type,void* arg);
void _ResetHW(type_debug_t _type,void* arg);
void _ATCmdTest(type_debug_t _type,void * arg);
/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/


typedef struct {
	type_debug_t type;
	const char *cmd_txt;
//	void (*Cbk_fnc)(bool,type_debug_t);
	void (*Cbk_fnc)(type_debug_t,void * arg);
}
plog_cmd_t;

volatile uint16_t FmDebug = PL_ALL;	
plog_t PLOG;

const plog_cmd_t plog_cmd[] =  {

	/*NAMI Plog*/
	{NAMI,(const char *)"p nami", _set_plog},
	{IR,(const char *)"p ir", _set_plog},
	{OTA,(const char *)"p ota", _set_plog},
	{WF,(const char *)"p wf", _set_plog},
	{BLE,(const char *)"p ble", _set_plog},
	{PULSE,(const char *)"p pulse", _set_plog},
	{ALL,(const char *)"p all", _set_plog},
	{DEFAULT,(const char *)"p default", _set_plog},
	/**@Cmd for testing. Not save*/
	{HELP,(const char *)"p help",_help},
	{SETCMD,(const char *)"p set",_Simulate},
	{GETCMD,(const char *)"p get",_Simulate},
	{TESTCMD,(const char *)"p test",_Simulate},
	{ATCMD,(const char *)"p atcmd",_ATCmdTest},
	{RSTCMD,(const char *)"p reset",_ResetHW},
//---------------------------------------
	{PLEND,(const char *)"None", NULL},
};

/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/
FncPassing Passing;
uint8_t  plog_buffer[RX_MAX_BUF] = "";
uint32_t plog_count = 0;
bool DirectMode = false;
/******************************************************************************/
/******************************************************************************/
/***                            Private ProtoThreads                         **/
/******************************************************************************/
/******************************************************************************/
const char help[] ={
	"\r\n*************** Personal Framework (v1.0) ********************\r\n"
	// "** p reset : Reset HW\r\n"
	// "** p set speed <value>: simulate speed\r\n"
	// "**   <value> : -1 : end testing\r\n"
	// "** p set utc <value>: set datetime(yy/MM/dd-hh:mm:ss)\r\n"
	// "**   <value> : 0 : end testing\r\n"
	// "** p set bkp 0 : Clear all backup data" 
	// "** p get speed <value>: Get current speed\r\n"
	// "** p get runtime: Get time running\r\n"
	// "** p test <type> <off>\r\n"
	// "**   <type> = histories: Testing history with virtual utc 1s = 1ms in realtime\r\n"
	// "** p atcmd <module> : At command testing\r\n"
	// //"**   <module> = ec,gps,..\r\n"
	// "**   EX: p atcmd ec\r\n"
	// "** p atcmd exit\r\n"
};
void _help(type_debug_t _type,void* arg){
	printf("\r\n%s",help);
	printf("** Plog commands line: <cmd> on/off \r\n");
	uint8_t i=0;
	while(plog_cmd[i].type!=PLEND){
		printf("|-> ");
		printf("%s\r\n",plog_cmd[i].cmd_txt);
		i++;
	}
	printf("******************* (C)HaiLuong-2023 **********************\r\n");
}
void _ResetHW(type_debug_t _type,void* arg){
	printf("Reseting..........\r\n");
//	DRV_DELAY_MS(500);
//	DRV_Reset(0);
}

void _Simulate(type_debug_t _type,void* arg){
	char *_arr = (char*)arg;
	Passing(_type,_arr);
}

void _ATCmdTest(type_debug_t _type,void * arg){
	char *_arr = (char*)arg;
//	printf("Access EC20xx at-cmd testing mode!\r\n");
//	if(_type == ATCMD){
	#if EC200S
		if(strstr(_arr,"ec")){
			printf("Access EC20xx at-cmd testing mode!\r\n");
			printf(">");
			DirectMode = true;
		}
		if(strstr(_arr,"exit")){
			printf("EXIT EC20xx at-cmd testing mode!\r\n");
			DirectMode = false;
		}
		#endif
//	}
}

void _set_plog (type_debug_t _type,void * arg){
	uint32_t buf = 0x00000000U;
	char *_arr = (char*)arg;
	bool _mode = bool_dbg_fnc(_arr);
	if(_mode == true) buf= 0xFFFFFFFFU;
	switch ((uint8_t)_type)
	{
		case ALL: {
			FmDebug = (buf & PL_ALL);
			break;
		}
		case DEFAULT: {
			FmDebug = PL_DEFAULT;
			break;
		}
		
		default: 
		{
			if(_mode == false) FmDebug = FmDebug & (~BIT(_type));
			else FmDebug = FmDebug | BIT(_type);
			break;
		}
	};
	// printf("fmDBG("PRINTF_BINARY_PATTERN_INT16 ")\r\n",PRINTF_BYTE_TO_BINARY_INT16(FmDebug));
}
/* 			p|space|TYPE_DBG|space|MODE|\r\n| */
/*	ex: p drv on\r\n : turn on driver debug
*				p drv off\r\n : turn off driver debug
*/
void _plog_parser(char *_arr,uint8_t _arr_sz)
{
	uint8_t i =0;
	printf("%s\r\n",_arr);
	while(PLEND != plog_cmd[i].type)
	{
		if (strncmp (plog_cmd[i].cmd_txt,_arr,strlen(plog_cmd[i].cmd_txt)) == 0)
		{
			plog_cmd[i].Cbk_fnc(plog_cmd[i].type,&_arr[strlen(plog_cmd[i].cmd_txt)+1]);
		}
		// else{
			// printf("[%d][%s] - PLOG: %s/ (%s)\r\n",__LINE__, __func__,_arr, plog_cmd[i].cmd_txt);
		// }
		i++;
	};

	// if (strncmp (plog_cmd[NAMI].cmd_txt,_arr,strlen(plog_cmd[NAMI].cmd_txt)) == 0)
	// {
	// 	plog_cmd[NAMI].Cbk_fnc(plog_cmd[NAMI].type,&_arr[strlen(plog_cmd[NAMI].cmd_txt)+1]);
	// }
	// else if (strncmp (plog_cmd[ALL].cmd_txt,_arr,strlen(plog_cmd[ALL].cmd_txt)) == 0)
	// {
	// 	plog_cmd[ALL].Cbk_fnc(plog_cmd[ALL].type,&_arr[strlen(plog_cmd[ALL].cmd_txt)+1]);
	// }

}

/******************************************************************************/
/******************************************************************************/
/***                            Private functions                            **/
/******************************************************************************/
/******************************************************************************/

bool bool_dbg_fnc(char *_input){
	if(strncmp(_input,"on",2) == 0) return true;
//	else if(strncmp(_input,"off",3) == 0) 
	return false;
}

/* retarget the C library printf function to the USART */
// int fputc(int ch, FILE *f)
// {
	
// //    usart_data_transmit(PLOG.usart.peri.uart.rcu_uart, (uint8_t)ch);
// //    while(RESET == usart_flag_get(PLOG.usart.peri.uart.rcu_uart, USART_FLAG_TBE));
// //    return ch;
	
// 	if(PLOG.usart.peri.uart.rcu_uart != 0)
// 	{
// 		#ifndef DBG_CHECK_SEND
// 		uint64_t startTime = DRV_GETTICK();
// 		while (!DRV_UART_FLAG_TBE_GET(PLOG.usart.peri.uart.rcu_uart))
// 		{
// 			if(DRV_GETTICK() - startTime > 100) //100 ms
// 			{
// 				DRV_UART_FLAG_TC_CLEAR(PLOG.usart.peri.uart.rcu_uart);
// 				return ch;
// 			}   
// 		}
// 		DRV_UART_TRANSMIT(PLOG.usart.peri.uart.rcu_uart, (uint8_t)ch);
// 		while (!DRV_UART_FLAG_TC_GET(PLOG.usart.peri.uart.rcu_uart))
// 		{
// 			if(DRV_GETTICK() - startTime > 100)
// 			{
// 				DRV_UART_FLAG_TBE_CLEAR(PLOG.usart.peri.uart.rcu_uart);
// 				return ch;
// 			}  
// 		}
// 		#else
// 		DRV_UART_TRANSMIT(PLOG.usart.peri.uart.rcu_uart, (uint8_t)ch);
// 		#endif
// 	}
// 	else //use usb
// 	{
// 		//DRV_USB_SendU8((uint8_t)ch);
// 	}
// 	return ch;
// }
/******************************************************************************/
/***                            Library callback                             **/
/******************************************************************************/
void USBPlog_callback(void){
//	static uint8_t hdr = 0, end_point =0;
//	PLOG.usb.rx.idx++;
//	PLOG.usb.rx.rcv[PLOG.usb.rx.idx] = DRV_USB_ReceiveU8();
//	//printf("(%d)(%d-%d)%c",PLOG.usb.rx.idx,hdr,end_point,PLOG.usb.rx.rcv[PLOG.usb.rx.idx]);
//	if( PLOG.usb.rx.rcv[PLOG.usb.rx.idx] == ' ' && PLOG.usb.rx.rcv[PLOG.usb.rx.idx-1] == 'p' && !hdr){
//		hdr = PLOG.usb.rx.idx - 1;
//	}
//	if(hdr!=0 && PLOG.usb.rx.rcv[PLOG.usb.rx.idx] == 0x0D){
//		end_point = PLOG.usb.rx.idx;
//	}
//	if((hdr!=0 && end_point!=0)){
//		_plog_parser((char*)&PLOG.usb.rx.rcv[hdr],end_point);
//		hdr = end_point = PLOG.usb.rx.idx = 0;
//		memset((char*)PLOG.usb.rx.rcv,0,RX_MAX_BUF);
//	}
//	//printf("%c",PLOG.usb.rx.rcv[PLOG.rx.idx]);
//	DRV_USB_SendU8(PLOG.usb.rx.rcv[PLOG.usb.rx.idx]);
}

void Plog_callback(void *arg){
//	static uint8_t hdr = 0, end_point =0;
//	PLOG.usart.rx.idx++;
//	PLOG.usart.rx.rcv[PLOG.usart.rx.idx] = DRV_UART_RECEIVE(PLOG.usart.peri.uart.rcu_uart);
//	//printf("(%d)(%d-%d)%c",PLOG.usart.rx.idx,hdr,end_point,PLOG.usart.rx.rcv[PLOG.usart.rx.idx]);
//	if( PLOG.usart.rx.rcv[PLOG.usart.rx.idx] == ' ' && PLOG.usart.rx.rcv[PLOG.usart.rx.idx-1] == 'p' && !hdr){
//		hdr = PLOG.usart.rx.idx - 1;
//	}
//	if(hdr!=0 && PLOG.usart.rx.rcv[PLOG.usart.rx.idx] == 0x0D){
//		end_point = PLOG.usart.rx.idx;
//	}
//	if((hdr!=0 && end_point!=0)){
//		_plog_parser((char*)&PLOG.usart.rx.rcv[hdr],end_point);
//		hdr = end_point = PLOG.usart.rx.idx = 0;
//		memset((char*)PLOG.usart.rx.rcv,0,RX_MAX_BUF);
//	}
	//printf("%c",PLOG.usart.rx.rcv[PLOG.rx.idx]);
//	char *ADA_USB ="ADA USB Initilization!!!\r\n";
//	DRV_USB_Send((uint8_t*)ADA_USB,strlen(ADA_USB));	
}


void Plog_timeout_callback(void *arg){
//	if(PLOG.usart.rx.idx <  sizeof(PLOG.usart.rx.rcv)- 1)
//	{
//		PLOG.usart.rx.rcv[PLOG.usart.rx.idx] = DRV_UART_RECEIVE(PLOG.usart.peri.uart.rcu_uart);
//		PLOG.usart.rx.idx++;
//	}
//	else DRV_UART_RECEIVE(PLOG.usart.peri.uart.rcu_uart);
//	PLOG.usart.rx.rxtime = DRV_MILLIS();
}

/******************************************************************************/
/******************************************************************************/
/***                            MainRun functions                            **/
/******************************************************************************/
/******************************************************************************/

void PLOG_Run(void)
{
// 	if(PLOG.usart.handle_flag)
// 	{
// 		PLOG.usart.handle_flag = false;
// 		if(PLOG.usart.rx.rxtime > 0)
// 		{
// 			PLOG.usart.rx.rxtime--;
// 			if(!PLOG.usart.rx.rxtime)
// 			{
// 				LOGA(DRV, "Data: %s\r\n", PLOG.usart.rx.rcv);
// 				LOGA(APP, "Data: %s\r\n", PLOG.usart.rx.rcv);
// 				LOGA(BOOTLOADER, "Data: %s\r\n", PLOG.usart.rx.rcv);
// //				LOGA(UDFUSB, "Data: %s\r\n", PLOG.usart.rx.rcv);
// 				LOGA(INF, "Data: %s\r\n", PLOG.usart.rx.rcv);
// 				LOGA(INF_FILE, "Data: %s\r\n", PLOG.usart.rx.rcv);
// 				LOGA(RFID, "Data: %s\r\n", PLOG.usart.rx.rcv);
// 				memcpy(&plog_buffer, &PLOG.usart.rx.rcv, PLOG.usart.rx.idx);
// 				plog_count = PLOG.usart.rx.idx;
// 				memset(&PLOG.usart.rx.rcv, 0x00, sizeof(PLOG.usart.rx.rcv));
// 				PLOG.usart.rx.idx = 0;
// 			}
// 		}
// 	}
	
// 	if(plog_buffer[0] != NULL && plog_buffer[1] != NULL && plog_buffer[2] != NULL && plog_buffer[3] != NULL)
// 	{
// 		_plog_parser((char*)&plog_buffer[0], plog_count);
// 		memset(&plog_buffer, 0x00, sizeof(plog_buffer));
// 		plog_count = 0;
// 	}
	
	
	
	
// //	if((PLOG.usart.rx.idx > 0) && (abs((int)(DRV_MILLIS() - PLOG.usart.rx.rxtime)) > PLOG_TIMEOUT_RX))
// //	{
// //		//if(strstr((char*)PLOG.usart.rx.rcv,"\r"))
// //			{
// //			_plog_parser((char*)&PLOG.usart.rx.rcv[0], PLOG.usart.rx.idx);
// //			PLOG.usart.rx.idx = 0;
// //			memset((char*)PLOG.usart.rx.rcv,0,sizeof(PLOG.usart.rx.rcv));
// //		}
// //	}
	
// //	#if EC200S
// //	if(PLOG.usart.peri.uart.rcu_uart != 0){
// //		while(DirectMode == true){
// //			if(PLOG.usart.rx.rcv[PLOG.usart.rx.idx] == '\r' ){
// //				//printf("{%d-%s}",PLOG.usart.rx.idx,(char *)&PLOG.usart.rx.rcv[1]);
// //				_ATPassing((char *)&PLOG.usart.rx.rcv[1]);
// //				printf("\r\n>");
// //				memset((char*)PLOG.usart.rx.rcv,0,RX_MAX_BUF);
// //				PLOG.usart.rx.idx = 0;
// //			}
// //		}
// //	}
// //	else
// //	{
// //		USBPlog_callback();
// //	}
// //	#endif
}
/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					                     **/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

void PLOG_Init(void)
{
	 memset(&PLOG, 0x00, sizeof(PLOG));
	 PLOG.Init        = &PLOG_Init;
	 PLOG.plog_parser = &_plog_parser;
	 PLOG.Stop        = &PLOG_Stop;
//	/*USART0*/	
//	PLOG.usart.peri.uart.rcu_uart = USART0;
//	PLOG.usart.peri.uart.tx.port = GPIOA;
//	PLOG.usart.peri.uart.tx.pin = GPIO_PIN_9;
//	PLOG.usart.peri.uart.rx.port = GPIOA;
//	PLOG.usart.peri.uart.rx.pin = GPIO_PIN_10;
//		/*USART1*/	
//	PLOG.usart.peri.uart.rcu_uart = USART1;
//	PLOG.usart.peri.uart.tx.port = GPIOA;
//	PLOG.usart.peri.uart.tx.pin = GPIO_PIN_2;
//	PLOG.usart.peri.uart.rx.port = GPIOA;
//	PLOG.usart.peri.uart.rx.pin = GPIO_PIN_3;
		/*USART2*/	
//	PLOG.usart.peri.uart.rcu_uart = USART2;
//	PLOG.usart.peri.uart.tx.port = GPIOB;
//	PLOG.usart.peri.uart.tx.pin = GPIO_PIN_10;
//	PLOG.usart.peri.uart.rx.port = GPIOB;
//	PLOG.usart.peri.uart.rx.pin = GPIO_PIN_11;
	/*UART3*/
//	PLOG.usart.peri.uart.rcu_uart = UART3;
//	PLOG.usart.peri.uart.tx.pin = GPIO_PIN_10;
//	PLOG.usart.peri.uart.tx.port = GPIOC;
//	PLOG.usart.peri.uart.rx.pin = GPIO_PIN_11;
//	PLOG.usart.peri.uart.rx.port = GPIOC;
			/*UART4*/
//	PLOG.usart.peri.uart.rcu_uart = UART4;
//	PLOG.usart.peri.uart.tx.pin = GPIO_PIN_12;
//	PLOG.usart.peri.uart.tx.port = GPIOC;
//	PLOG.usart.peri.uart.rx.pin = GPIO_PIN_2;
//	PLOG.usart.peri.uart.rx.port = GPIOD;
	
	
//	rcu_periph_clock_enable(RCU_AF);
//
//	/* USART interrupt configuration */
//  nvic_irq_enable(UART4_IRQn, 0, 0);
//	/* enable GPIO clock */
//	rcu_periph_clock_enable(RCU_GPIOC);
//	rcu_periph_clock_enable(RCU_GPIOD);
//
//
//	/* enable USART clock */
//	rcu_periph_clock_enable(RCU_UART4);
//
//
//	/* connect port to USARTx_Tx */
//	gpio_init(PLOG.usart.peri.uart.tx.port, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, PLOG.usart.peri.uart.tx.pin);
//
//	/* connect port to USARTx_Rx */
//	gpio_init(PLOG.usart.peri.uart.rx.port, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, PLOG.usart.peri.uart.rx.pin);
	
	/* USART configure */
//	usart_deinit(UART4);
//	usart_baudrate_set(UART4, 115200U);
//	usart_receive_config(UART4, USART_RECEIVE_ENABLE);
//	usart_transmit_config(UART4, USART_TRANSMIT_ENABLE);
//	usart_enable(UART4);
//
//
//	usart_interrupt_enable(UART4, USART_INT_RBNE);
	
//	PLOG_Start(ALL);
	
	
	//USB
//	PLOG.usart.peri.uart.rcu_uart=0;
//	PLOG.usb.peri.usb_DP_Pin.port = GPIOA;
//	PLOG.usb.peri.usb_DP_Pin.pin = GPIO_PIN_8;
//	DRV_USB_Init(PLOG.usb.peri);
	
//	PLOG.Init=&PLOG_Init;
//	PLOG.Start = &Start;
//	PLOG.Stop = &Stop;
	
//	Passing=_fnc;
//	Cblk_parameter_t plog_cbk;
//	plog_cbk.type = DR_UART;
//	plog_cbk.para =  &PLOG.usart.peri.uart.rcu_uart;
////	plog_cbk.EventCallback_t = &Plog_callback;
//plog_cbk.EventCallback_t = &Plog_timeout_callback;
	
//	Drv_RegisterUART(115200,PLOG.usart.peri.uart.rcu_uart,PLOG.usart.peri.uart.tx,PLOG.usart.peri.uart.rx);
//	Drv_RegisterIRQ_callback(plog_cbk);
}

void PLOG_Stop(type_debug_t _type){
	_set_plog(_type,"off");
}
void PLOG_Start(type_debug_t _type){
	_set_plog(_type,"on");
}

// void PLG_PrintHexBuffer( uint8_t *buffer, uint16_t size )
// {
// //	if((BIT(INF_FILE) &FmDebug) != BIT(INF_FILE)) {
// //		return;
// //	}
// 	uint16_t newline = 0;
// 	for( uint16_t i = 0; i < size; i++ )
// 	{
// 			if( newline != 0 )
// 			{
// 					P_PRINTF(INF_FILE,"\r\n" );
// 					newline = 0;
// 			}

// //			if(!isalnum(buffer[i]))	
// 				P_PRINTFA(INF_FILE, "%02X ", buffer[i] );

// 			if( ( ( i + 1 ) % 16 ) == 0 )newline = 1;
// 	}
// 	P_PRINTF(INF_FILE,"\r\n" );
// }
