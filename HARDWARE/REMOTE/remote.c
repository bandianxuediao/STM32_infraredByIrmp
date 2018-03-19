#include "remote.h"
#include "delay.h"
#include "usart.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK战舰STM32开发板
//红外遥控解码驱动 代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/9/12
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
//////////////////////////////////////////////////////////////////////////////////

u8 g_IR_RecFlag = 0;        //红外接收到标志


//红外遥控初始化
//设置IO以及定时器4的输入捕获
void Remote_Init(void)    			  
{  
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_ICInitTypeDef  TIM_ICInitStructure;  
 
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE); //使能PORTB时钟 
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4,ENABLE);	//TIM4 时钟使能 

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;				 //PB9 输入 
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; 		//上拉输入 
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
 	GPIO_SetBits(GPIOB,GPIO_Pin_9);	//初始化GPIOB.9
	
						  
 	TIM_TimeBaseStructure.TIM_Period = 10000; //设定计数器自动重装值 最大10ms溢出  
	TIM_TimeBaseStructure.TIM_Prescaler =(35-1); 	//预分频器,1M的计数频率,1us加1.	   
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式

	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx

  	TIM_ICInitStructure.TIM_Channel = TIM_Channel_4;  // 选择输入端 IC4映射到TI4上
  	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Falling;	//上升沿捕获..改为下降沿捕获
  	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;	 //配置输入分频,不分频 
  	TIM_ICInitStructure.TIM_ICFilter = 0x03;//IC4F=0011 配置输入滤波器 8个定时器时钟周期滤波
  	TIM_ICInit(TIM4, &TIM_ICInitStructure);//初始化定时器输入捕获通道

   	TIM_Cmd(TIM4,ENABLE ); 	//使能定时器4
 
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;  //TIM4中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器	

	TIM_ITConfig( TIM4,TIM_IT_Update|TIM_IT_CC4,ENABLE);//允许更新中断 ,允许CC4IE捕获中断								 
}

//遥控器接收状态
//[7]:收到了引导码标志
//[6]:得到了一个按键的所有信息
//[5]:保留	
//[4]:标记下降沿是否已经被捕获								   
//[3:0]:溢出计时器
u8 RmtSta=0;	  	
u8 nFlag = 0;
u8 nData = 0;
u16 Dval;		//下降沿时计数器的值
u32 RmtRec=0;	//红外接收到的数据	   		    
u8  RmtCnt=0;	//按键按下的次数	  

//定时器4中断服务程序	 
void TIM4_IRQHandler(void)
{ 		    	 
    if(TIM_GetITStatus(TIM4,TIM_IT_Update)!=RESET)      //更新中断
	{
		
        if(RmtSta&0x80)//上次有数据被接收到了
		{	
			RmtSta &= ~0x10;						//取消上升沿已经被捕获标记
			if((RmtSta&0x0F) == 0x00) RmtSta |= 1<<6;//标记已经完成一次按键的键值信息采集
			if((RmtSta&0x0F) < 15) RmtSta++;         //超过按键位数则清零
			else
			{
				RmtSta&=~(1<<7);//清空引导标识
				RmtSta&=0xF0;	//清空计数器	
			}
//             printf("TIMEL=%d\r\n",RmtRec);        
		}							    
	}
 	if(TIM_GetITStatus(TIM4,TIM_IT_CC4)!=RESET)
	{	  
		if(!RDATA)      //低电平，代表下降沿捕获, 则开启上升沿捕获，清TIM4
		{
            Dval = TIM_GetCapture4(TIM4);    //读取前一个高电平的时间
            TIM_SetCounter(TIM4,0);	   	    //清空定时器值，重新开始计数低电平时间	
            TIM_OC4PolarityConfig(TIM4,TIM_ICPolarity_Rising);		//CC4P=1 设置为上升沿捕获	
			
            if(RmtSta&0x80)//接收到了首位
            {
               
                if(Dval>1500 && Dval<2000)	    //1.5ms--2ms（标准：1.688ms）
                {
                    nFlag = 0;
                }                 
            }
            RmtSta|=0x10;				    //标记下降沿已经被捕获
		}
        else //高电平，代表上升沿捕获，开启下降沿捕获，清TIM4
		{			
  			Dval=TIM_GetCapture4(TIM4);    //读取CCR4也可以清CC4IF标志位
			TIM_OC4PolarityConfig(TIM4,TIM_ICPolarity_Falling); //CC4P=0	设置为下降沿捕获.
            TIM_SetCounter(TIM4,0);	   	    //清空定时器值，重新开始计数高电平时间	
/******************************************************************************/					
		//RC-5编码一位数据：“1”接收为：先高电平0.844ms+低电平0.844ms
		//RC-5编码一位数据：“0”接收为：先低电平0.844ms+高电平0.844ms
		//如果接收的低电平为1.688ms，则说明接收了一个高电平和一个低电平，即数据位2
		//只有前次低电平时间为1.688ms(TimL>=75 && TimL<=90)，后面的短的低电平为“0”，其他情况均为“1”
/******************************************************************************/ 			
			if(RmtSta&0x10)					//下降沿已经捕获
			{
 				if(RmtSta&0x80)//接收到了首位
				{										
                    if(Dval>600 && Dval<1200)			//0.7ms--0.98ms（标准：0.844ms）
					{
                        if(nFlag == 1)
                        {
                            nData = 0;          //收到数据0
                            RmtRec <<= 1;	        //左移一位.
                            RmtRec += nData;	//接收到0	
                        }
                        else
                        {
                            nData = 1;          //收到数据1
                            RmtRec <<= 1;         //左移一位.
                            RmtRec += nData;	//接收到1
                        }
                           
					}
                    else if(Dval>1500 && Dval<2000)	//1.5ms--2ms（标准：1.688ms）
					{
						nFlag = 1;
                        nData = 2;              //收到两位数据1和0，即 2
                        RmtRec <<= 2;	        //左移2位.
						RmtRec += nData;	    //接收到
					}
//                     else			//信号异常（可能是干扰）
// 					{
// 						printf("红外编码接收异常，请重按遥控按键 TIMEL=%d\r\n",Dval);
// 						
// 					}		
 				}
                else if(Dval>600 && Dval<1200)		//未接收首位则处理此处为首位接收，低电平标准值为0.844ms
				{
					RmtSta|=1<<7;	//标记成功接收到了首位数据
					RmtRec=1;	    //首位为1
                    RmtCnt=0;		//清除按键次数计数器
				}						 
			}
			RmtSta&=~(1<<4);
		}				 		     	    					   
	}
 TIM_ClearFlag(TIM4,TIM_IT_Update|TIM_IT_CC4);	    
}

//处理红外键盘
//返回值:
//	 0,没有任何按键按下
//其他,按下的按键键值.
u8 Remote_Scan(void)
{        
	u8 sta = 0;       
    u8 t1,t2;  
	if(RmtSta&(1<<6))//得到一个按键的所有信息了
	{ 
	    sta = (RmtRec &= 0x000003F);			//得到地址码
//	    t2=(RmtRec>>16)&0xff;	//得到地址反码 
// 	    if((t1==(u8)~t2)&&t1==REMOTE_ID)//检验遥控识别码(ID)及地址 
//	    { 
//	        t1=RmtRec>>8;
//	        t2=RmtRec; 	
//	        if(t1==(u8)~t2)sta=t1;//键值正确	 
//		}   
//		if((sta==0)||((RmtSta&0X80)==0))//按键数据错误/遥控已经没有按下了
//		{
//		 	RmtSta&=~(1<<6);//清除接收到有效按键标识
//			RmtCnt=0;		//清除按键次数计数器
//		}
        RmtSta&=~(1<<6);//清除接收到有效按键标识
	}  
    return sta;
}
































