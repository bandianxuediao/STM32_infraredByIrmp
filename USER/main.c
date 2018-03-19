#include "led.h"
#include "delay.h"
#include "key.h"
#include "sys.h"
#include "lcd.h"
#include "usart.h"	
#include "irmp.h"

IRMP_DATA irmp_data;
uint32_t
SysCtlClockGet(void)
{
    RCC_ClocksTypeDef RCC_ClocksStatus;
    RCC_GetClocksFreq(&RCC_ClocksStatus);
    return RCC_ClocksStatus.SYSCLK_Frequency;
}

void
timer2_init (void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = 7;
    TIM_TimeBaseStructure.TIM_Prescaler = ((F_CPU / F_INTERRUPTS)/8) - 1;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM2, ENABLE);
}

void
TIM2_IRQHandler(void)                                                  // Timer2 Interrupt Handler
{
  TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
  (void) irmp_ISR();                                                        // call irmp ISR
  // call other timer interrupt routines...
}
 
 int main(void)
 {	 
	delay_init();	    	 //延时函数初始化	  
	NVIC_Configuration(); 	 //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	uart_init(9600);	 	//串口初始化为9600
 	LED_Init();			     //LED端口初始化
        
    irmp_init();                                                            // initialize irmp
    timer2_init();                                                          // initialize timer2
    for (;;)
    {
        if (irmp_get_data (&irmp_data))
        {					
//          printf("R: Code: %s",irmp_protocol_names[irmp_data.protocol]);          
          printf(" Address: 0x%.2X",irmp_data.address);
          printf(" Command: 0x%.2X",irmp_data.command);
          printf(" Flags: 0x%.2X\r\n",irmp_data.flags );										
        }
    } 	  		    							  
	while(1)
	{


	}
}



