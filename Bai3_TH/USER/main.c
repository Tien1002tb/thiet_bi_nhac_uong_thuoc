#include "stm32f10x.h"                  // Device header
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stdio.h"
#include "delay.h"
#include "ds1307.h"
#include "LiquidCrystal_I2C.h"

#define GPIO_PORT GPIOA
#define OK  GPIO_Pin_3
#define COI  GPIO_Pin_2
#define DEN GPIO_Pin_1
GPIO_InitTypeDef GPIO_Structure;
int congtru_tong=0;

char 	vrc_Getc; 			// bien kieu char, dung de nhan du lieu tu PC gui xuong;
int		vri_Stt = 0; 			// bien danh dau trang thai. 
int 	vri_Count = 0; 		// bien diem
char	vrc_Res[100];
char receivedChar;

uint16_t UARTx_Getc(USART_TypeDef* USARTx);
void uart_Init(void);
void USART1_IRQHandler(void);
void UART_SendString(const char *str);

uint8_t day, date, month, year, hour, min, sec;
uint8_t thu, ngay, thang, nam, gio, phut, giay;
u8 hour_set1, min_set1, sec_set1;
u8 hour_set2, min_set2, sec_set2;
char *arr[] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
char bufferdate[50];
char buffertime[50];
char bufferday[50];
char timesetting2[50];
char timesetting1[50];

void Real_time(void );
void Get_time(void);
void alarm(void);

void Display(void *p);
void uart_setTime(void *p);

QueueHandle_t uartQueue;

int main(void){
	uartQueue = xQueueCreate(128, sizeof(char));
	LCDI2C_init(0x27,16,2); 
	ds1307_init();	
	LCDI2C_backlight();
	xTaskCreate(Display, (const char*)"Display",128 , NULL,1, NULL);
	xTaskCreate(uart_setTime, (const char*)"SetTime",128 , NULL,tskIDLE_PRIORITY, NULL);
	vTaskStartScheduler(); 
	while(1){
	}
}

void gpio_Init(void){
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_Structure.GPIO_Pin = COI | DEN; // cau hinh chan gpio den
	GPIO_Structure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Structure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_Structure);	
	
	GPIO_Structure.GPIO_Pin =  OK ; // nut
	GPIO_Structure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Structure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_Structure);	
}

void Display(void *p){
	gpio_Init();
	uint8_t button_state_ok = 1 ;
	uint8_t last_button_state_1 ;
	while(1){
	Get_time();
		last_button_state_1 = button_state_ok;
	  button_state_ok = GPIO_ReadInputDataBit(GPIO_PORT, OK);				
        if (button_state_ok == 0 && last_button_state_1 == 1) {
						congtru_tong ++;			
            if (congtru_tong == 1) {
                congtru_tong = 0;
            }					
							LCDI2C_clear(); // Clear Display												
					}   			
				if(congtru_tong == 0){
					Real_time();							
					}
				else if(congtru_tong == 1){
					alarm();
					}
		if (hour_set1 == gio && min_set1 == phut && sec_set1 == giay){
				GPIO_SetBits(GPIO_PORT, DEN);
				GPIO_SetBits(GPIO_PORT, COI);
				Delay(500);
				GPIO_ResetBits(GPIO_PORT, DEN);
				GPIO_ResetBits(GPIO_PORT, COI);
				Delay(500);			
		}
		else if(hour_set1 == gio && min_set1 == phut && sec_set1 == giay){
				GPIO_SetBits(GPIO_PORT, DEN);
				GPIO_SetBits(GPIO_PORT, COI);
				Delay(500);
				GPIO_ResetBits(GPIO_PORT, DEN);
				GPIO_ResetBits(GPIO_PORT, COI);
				Delay(500);						
		}
	}	
}

void Get_time(void){
		ds1307_get_calendar_date(&thu, &ngay, &thang, &nam);
		ds1307_get_time_24(&gio, &phut, &giay);
		sprintf(bufferday,"%s-",arr[thu-1]);
		sprintf(bufferdate, "%02hhu/%02hhu/20%02hhu",ngay, thang, nam);
		sprintf(buffertime, "%02hhu:%02hhu:%02hhu", gio, phut, giay);
		Delay(50);

	}
void Real_time(void){	
		LCDI2C_setCursor(1, 1);
		LCDI2C_write_String(bufferday);		
		LCDI2C_setCursor(5, 1);
		LCDI2C_write_String(bufferdate);		
		LCDI2C_setCursor(4, 0);				
		LCDI2C_write_String(buffertime);
		Delay(50);	
		}

void uart_setTime(void *p){
	uart_Init();
	while (1) {
		  if(xQueueReceive(uartQueue, (void*)&receivedChar, (portTickType)0xFFFFFFFF)){
				if(receivedChar =='T'){
						vri_Stt = 1;
						receivedChar = NULL;
						vri_Count=0;
				}
				if(receivedChar == 'F'){
						vri_Stt = 2;
						receivedChar = NULL;
						vri_Count = 0;
				}
				if(receivedChar == 'S'){
						vri_Stt = 3;
						receivedChar = NULL;
						vri_Count = 0;
				}
				else{
					vrc_Res[vri_Count]= receivedChar;
					vri_Count++;			
				}
			if(vri_Stt ==1){	
					sscanf(vrc_Res, "%hhu-%02hhu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu", &date, &day,
														&month, &year,&hour, &min, &sec);	
					ds1307_set_calendar_date(date, day, month, year);
					ds1307_set_time_24(hour, min, sec);					
			
					vri_Stt = 0;
          vri_Count = 0;
          memset(vrc_Res, 0, sizeof(vrc_Res));
			}
			if(vri_Stt == 2){	
					sscanf(vrc_Res, " %02hhu:%02hhu:%02hhu" ,&hour_set1 , &min_set1, &sec_set1);							
					vri_Stt = 0;
          vri_Count = 0;
          memset(vrc_Res, 0, sizeof(vrc_Res));
			}
			if(vri_Stt == 3){	
					sscanf(vrc_Res, " %02hhu:%02hhu:%02hhu" ,&hour_set2 , &min_set2, &sec_set2);							
					vri_Stt = 0;
          vri_Count = 0;
          memset(vrc_Res, 0, sizeof(vrc_Res));
			}
			
		}
	
	}
}

void alarm(void){
		sprintf(timesetting1, "%02hhu/%02hhu/20%02hhu",hour_set1 , min_set1, sec_set1);
		LCDI2C_setCursor(1, 0);
		sprintf(timesetting2, "%02hhu/%02hhu/20%02hhu",hour_set2 , min_set2, sec_set2);
		LCDI2C_setCursor(1, 1);
		Delay(50);
}

uint16_t UARTx_Getc(USART_TypeDef* USARTx){
	return USART_ReceiveData(USARTx);
}

void USART1_IRQHandler(void) {
 if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        vrc_Getc = UARTx_Getc(USART1);
			xQueueSend(uartQueue, (void*)&vrc_Getc,(portTickType)0);
		}
 }

void uart_Init(void){
	GPIO_InitTypeDef gpio_typedef;
	USART_InitTypeDef usart_typedef;
	// enable clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	// congifgure pin Tx - A9;
	gpio_typedef.GPIO_Pin = GPIO_Pin_9;
	gpio_typedef.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio_typedef.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&gpio_typedef);	
	// configure pin Rx - A10;
	gpio_typedef.GPIO_Pin = GPIO_Pin_10;
	gpio_typedef.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	gpio_typedef.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&gpio_typedef);
	// usart configure
	usart_typedef.USART_BaudRate = 9600;
	usart_typedef.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	usart_typedef.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; 
	usart_typedef.USART_Parity = USART_Parity_No;
	usart_typedef.USART_StopBits = USART_StopBits_1;
	usart_typedef.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART1, &usart_typedef);
	
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	NVIC_EnableIRQ(USART1_IRQn);
	USART_Cmd(USART1, ENABLE);
}
