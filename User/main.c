/************************************************************************************
*  Copyright (c), 2014, HelTec Automatic Technology co.,LTD.
*            All rights reserved.
*
* Http:    www.heltec.cn
* Email:   cn.heltec@gmail.com
* WebShop: heltec.taobao.com
*
* File name: main.c
* Project  : HelTec.uvprij
* Processor: STM32F103C8T6
* Compiler : MDK fo ARM
* 

* Version: 1.00
* Date   : 2014.4.8

*
* Others: none;
*
* Function List:
*	1. int main(void);//主函数
*
* History: none;
*
*************************************************************************************/
#include "stm32f10x.h"
#include "OLED_I2C.h"
#include "delay.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

typedef struct Point 
{ 

double lon;
double lat;
double presure;
} point_s;    




#define BUF_LEN (2000)
volatile char RX_BUF[BUF_LEN] = {0};
volatile int buf_index = 0;
volatile int idle_flag = 0;

volatile int idle_counter = 0; //
volatile int tim_timeout = 0;
volatile int tim_15s_timeout = 0;
volatile int seconds_cnt = 0;

volatile int key_press = 0;
volatile int key_long_press = 0;

volatile int key_cnt = 0;

point_s his_data[500];

#define R 6371
#define TO_RAD (3.1415926536 / 180)
double dist(double th1, double ph1, double th2, double ph2)
{
	double dx, dy, dz;
	ph1 -= ph2;
	ph1 *= TO_RAD, th1 *= TO_RAD, th2 *= TO_RAD;
 
	dz = sin(th1) - sin(th2);
	dx = cos(ph1) * cos(th1) - cos(th2);
	dy = sin(ph1) * cos(th1);
	return asin(sqrt(dx * dx + dy * dy + dz * dz) / 2) * 2 * R;
}






double cal_distance(int counter)
{
    int i = 0;
    
    double total_distance = 0;
    
    printf("counter points is %d\n", counter);
    if(counter < 2)
        return 0;
    
    for(i = 1; i < counter; i++)
    {
    
        total_distance += dist(his_data[i-1].lat, his_data[i-1].lon,
                               his_data[i].lat, his_data[i].lon);
    
    }

    return total_distance;
}


void TIMx_Configuration(void)
{
	
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
		
   
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	
    
    //TIM_TimeBaseStructure.TIM_Period=1000;
    TIM_TimeBaseStructure.TIM_Period = 9999;//4999;
    //?????,?????????CK_CNT=CK_INT/(71+1)=1M
    TIM_TimeBaseStructure.TIM_Prescaler= 7199;
	
    
    //TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1;
		
    
    TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up; 
		
		
	//TIM_TimeBaseStructure.TIM_RepetitionCounter=0;
	
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
		
    //??????????
    TIM_ClearFlag(TIM3, TIM_FLAG_Update);
	  
		
    //???????
    TIM_ITConfig(TIM3, TIM_IT_Update,ENABLE);
    
    
    

    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; 
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;  
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);   

		
    TIM_Cmd(TIM3, ENABLE);																		    
}


void SysTickConfig(void)
{
    //if(SysTick_Config(SystemCoreClock / 10000))
    if(SysTick_Config(SystemCoreClock / 1000))
    {
        while(1);
    }
      
    NVIC_SetPriority(SysTick_IRQn, 0x0);
}



static void NVIC_Configuration()
{
    NVIC_InitTypeDef NVIC_InitStructure;

    //NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);           //??NVIC??????1

    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;        
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;        
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;   
    NVIC_Init(&NVIC_InitStructure);
}



void KEY_Configuration()
{
    GPIO_InitTypeDef GPIO_InitStructure;                    

    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA | \
                            RCC_APB2Periph_AFIO, ENABLE);     
    //GPIO_DeInit(GPIOB);                                      
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;                 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;             
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    
    GPIO_Init(GPIOA, &GPIO_InitStructure);                    
}



void pb9_set_low()
{
    GPIO_InitTypeDef GPIO_InitStructure;                    

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);     
    //GPIO_DeInit(GPIOB);                                      
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;                 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;             
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    
    GPIO_Init(GPIOA, &GPIO_InitStructure);                    
}



void EXTI_Configuration()
{
    KEY_Configuration();

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource7);

    EXTI_InitTypeDef EXTI_InitStructure;                                                                                            
    EXTI_InitStructure.EXTI_Line = EXTI_Line7;               //?????
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;      //EXTI?????
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  //?????
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;                //????
    EXTI_Init(&EXTI_InitStructure); 

    NVIC_Configuration();                                                                                           
}


point_s parse_data(char *data, int len)
{
    point_s point;
    point.lat = -1000;
    point.lon = -1000;
    
    int i;
    int j = 0;
    
    
    
    int index = 0;
    
    const char *p_data = data;
    const char *p_rmc = NULL;
    const char *p_return = NULL;
    
    
    point_s array[10] = {0};
    int valid_points_cnt = 0;
    valid_points_cnt = 0;
    
    char content[300] = {0};
    
    if(data == NULL || len <= 0)
        return point;
    
    
    while(p_data < data + len)
    {
        p_rmc = strstr(p_data, "GPRM");
        if(p_rmc)    
        {
            memset(content, 0, sizeof(content));
        
            p_return = strchr(p_rmc, 0x0d);
            if(p_return != NULL)
            {
                strncpy(content, p_rmc, p_return - p_rmc);
                
                //index = p_return
                p_data = p_return + 2;
            }
            else
            {
                
                strncpy(content, p_rmc, (data + len - 1) - p_rmc);
                
                p_data = data + len;
            }
            
            
            // if aaaaa
            //  if aaaa 
            //   get data
            if(content[16] == 'A')
            {
                double lon = 0, lat = 0;
                char ch1,ch2;
                ch1 = ch2 = 0;
                char *str = content + 18;
                const char s[2] = ",";
                char *token;
   
                printf("data valid\n");
                /* get the first token */
                token = strtok(str, s);
                while(token != NULL )
                {
                    printf( "token: %s\n", token);
                    j++;
                    if(j == 1)
                    {
                        lat = (token[0]-'0')* 10 + (token[1]-'0');
                        token[0] = token[1] = '0';
                        
                        lat += strtod(token, NULL) / 60.0;
                    }
                    else if(j == 2)
                    {
                        ch1 = token[0];
                        if(ch1 == 'S')
                            lat = -lat;
                    }
                    else if(j == 3)
                    {
                        lon = (token[0]-'0')* 100 + (token[1]-'0')*10 + (token[2]-'0');   //jingdu
                        token[0] = token[1] = token[2] = '0';
                        lon += strtod(token, NULL) / 60;
                        
                    }
                    else if(j == 4)
                    {
                        ch2 = token[0];
                        if(ch2 == 'W')
                            lon = -lon;
                    
                    }
                    if(j == 4) break;
                    
                    token = strtok(NULL, s);
                }
                
                j = 0;
                printf("p_data %p\n", p_data);
                printf("convert gps is lat %f, lon %f\n", lat, lon);
                printf("valid_points_cnt  %d\n", valid_points_cnt);
                //array[valid_points_cnt++].lat = lat;
                //array[valid_points_cnt++].lon = lon;
                array[valid_points_cnt].lat = lat;
                array[valid_points_cnt].lon = lon;
                valid_points_cnt++;
                
            }
        }
        else
        {
            break;
        }
    
    
    }
    
    
    printf("valid_points_cnt %d\n", valid_points_cnt);
    if(valid_points_cnt > 0)
    {
        point.lat = 0;
        point.lon = 0;
        for(i = 0; i < valid_points_cnt; i++)
        {
            point.lat += array[i].lat;
            point.lon += array[i].lon;
        }
    
        point.lat /= valid_points_cnt;
        point.lon /= valid_points_cnt;
    }
    
    
    return point;    
}





int main(void)
{
	int i;
    
    
    //double lon = 0;
    //double lat = 0;
    
    
    point_s pa = {0};
    int point_valid = 0;
    
    int track_index = 0;
    
    
    char lat_buf[20] = {0};
	char lon_buf[20] = {0};
    char distance_buf[20] = {0};
    //SysTickConfig();
    
    
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    memset(his_data, 0, sizeof(his_data));
    /* Enable peripheral clocks for USART1 on GPIOA */
    #if 0
    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_USART1 |
        RCC_APB2Periph_GPIOA |
        RCC_APB2Periph_AFIO, ENABLE);
    #endif
    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_USART1 |
        RCC_APB2Periph_GPIOA, ENABLE);    
    /* Configure PA9 and PA10 as USART1 TX/RX */
    USART_DeInit(USART1); 
    /* PA9 = alternate function push/pull output */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    /* PA10 = floating input */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    
    
    
    /* Configure and initialize usart... */
    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
        
    USART_Init(USART1, &USART_InitStructure);
    
    USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    
    
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;//?????3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1; //????3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ????
    NVIC_Init(&NVIC_InitStructure);
    /* Enable USART1 */
    USART_Cmd(USART1, ENABLE);
    
    USART_ClearITPendingBit(USART1,USART_IT_IDLE);
    USART_ClearITPendingBit(USART1,USART_IT_RXNE);
    
    //printf("test abc\n");
 
 
    EXTI_Configuration();
	
	DelayInit();
	I2C_Configuration();
	OLED_Init();
    OLED_CLS();
    
    TIMx_Configuration();
	
    
    DelayS(5);
    
    //double d = dist(36.12, -86.67, 33.94, -118.4);
    //printf("%.1f km \n", d);
    
    
    #if 0
    char *test = "$GPRMC,133306.00,A,3949.63075,N,11616.48616,E,0.513,,120116,,,A*7A\r\n" 
                  "$GPRMC,133306.00,A,3949.63075,N,11616.48616,E,0.513,,120116,,,A*7A\r\n ";

    printf("test data %s\n", test);
    point_s pa = parse_data(test, strlen(test));
    printf("%f, %f\n", pa.lat, pa.lon);
    #endif
    
	while(1)
	{
		//OLED_Fill(0xFF);//全屏点亮
		//DelayS(2);
		//OLED_Fill(0x00);//全屏灭
		//DelayS(2);
        
        
        
        printf("rx buf index %d\n", buf_index);
        printf("idle_flag %d\n", idle_flag);
        
        
        
        
        if(tim_timeout == 1)
        {
            printf("time out 1s\n");
            
            
            printf("idle counter %d\n", idle_counter);
            
            printf("==========================\n");
            if(idle_flag == 1)
            {
                
                
                for(i = 0; i < buf_index && RX_BUF[i]; i++);
                printf("valid data length %d\n", i);
                    //printf("%02x ", RX_BUF[i]);
                //printf("\n");
                
                
                for(i = 0; i < buf_index; i++)
                    printf("%c", RX_BUF[i]);
                printf("\n");
                
                
                printf("==========================\n");
                if(buf_index > 0 && buf_index < BUF_LEN)
                {
                    //GET ONE DADA LOCATION
                    pa = parse_data((char *)RX_BUF, buf_index);
                    if(pa.lat != -1000)
                    {
                        point_valid = 1;
                        
                        snprintf(lat_buf, sizeof(lat_buf), "LAT %.2f", pa.lat);
                        snprintf(lon_buf, sizeof(lon_buf), "LON %.2f", pa.lon);
                        
                        OLED_ShowStr(0,3, (unsigned char *)lon_buf, 1);
                        OLED_ShowStr(0,12, (unsigned char *)lat_buf, 1);
                    }
                    else
                    {
                        point_valid = 0;
                        snprintf(lat_buf, sizeof(lat_buf), "LAT ---");
                        snprintf(lon_buf, sizeof(lon_buf), "LON ---");
                        
                        OLED_ShowStr(0,3, (unsigned char *)lon_buf, 1);
                        OLED_ShowStr(0,12, (unsigned char *)lat_buf, 1);
                    }
                    
                }
                
                memset((void *)RX_BUF, 0, sizeof(RX_BUF));
                idle_flag = 0;
                buf_index = 0;
            }
            
            
            
            
            
            tim_timeout = 0;
        }
        
        
        if(tim_15s_timeout == 1)
        {
        
            
            printf("15s timeout\n");
            //OLED_ShowStr(0,3,"LONG 89.00",1);
            //OLED_ShowStr(0,12,"LAT 108.44",1);
            
            
            if(track_index > 490)
            {
                printf("track buffer full\n");
            }
            else
            {
                
                if(point_valid)
                {
                    his_data[track_index].lon = pa.lon;
                    his_data[track_index].lat = pa.lat;
                    
                    track_index++;
                }
                    
            }
            
            
            
            
            seconds_cnt = 0;
            tim_15s_timeout = 0;
        }
        
        
        
        //if(key_long_press)
        if(key_press)
        {
            
            printf("button pressed\n");
            
            #if 1
            // record the last point 
            if(point_valid && track_index < 490)
            {
                his_data[track_index].lon = pa.lon;
                his_data[track_index].lat = pa.lat;
                    
                track_index++;
            }
        
            // cal journey
            
            double journey_dist = cal_distance(track_index);
            snprintf(distance_buf, 20, "DIST %.2f km", journey_dist);
            OLED_ShowStr(0, 1, (unsigned char *)distance_buf, 1);
            
            /// lcd show ok
        
            key_cnt = 0;
            #endif
            
            key_press = 0;
        }
    
	}
    
}




/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART */
  USART_SendData(USART1, (uint8_t) ch);

  /* Loop until the end of transmission */
  while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
  {}

  return ch;
}
