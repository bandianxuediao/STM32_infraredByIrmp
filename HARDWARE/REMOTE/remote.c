#include "remote.h"
#include "delay.h"
#include "usart.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEKս��STM32������
//����ң�ؽ������� ����	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/9/12
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved									  
//////////////////////////////////////////////////////////////////////////////////

u8 g_IR_RecFlag = 0;        //������յ���־


//����ң�س�ʼ��
//����IO�Լ���ʱ��4�����벶��
void Remote_Init(void)    			  
{  
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_ICInitTypeDef  TIM_ICInitStructure;  
 
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE); //ʹ��PORTBʱ�� 
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4,ENABLE);	//TIM4 ʱ��ʹ�� 

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;				 //PB9 ���� 
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; 		//�������� 
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
 	GPIO_SetBits(GPIOB,GPIO_Pin_9);	//��ʼ��GPIOB.9
	
						  
 	TIM_TimeBaseStructure.TIM_Period = 10000; //�趨�������Զ���װֵ ���10ms���  
	TIM_TimeBaseStructure.TIM_Prescaler =(35-1); 	//Ԥ��Ƶ��,1M�ļ���Ƶ��,1us��1.	   
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ

	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure); //����ָ���Ĳ�����ʼ��TIMx

  	TIM_ICInitStructure.TIM_Channel = TIM_Channel_4;  // ѡ������� IC4ӳ�䵽TI4��
  	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Falling;	//�����ز���..��Ϊ�½��ز���
  	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;	 //���������Ƶ,����Ƶ 
  	TIM_ICInitStructure.TIM_ICFilter = 0x03;//IC4F=0011 ���������˲��� 8����ʱ��ʱ�������˲�
  	TIM_ICInit(TIM4, &TIM_ICInitStructure);//��ʼ����ʱ�����벶��ͨ��

   	TIM_Cmd(TIM4,ENABLE ); 	//ʹ�ܶ�ʱ��4
 
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;  //TIM4�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  //��ռ���ȼ�0��
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //�����ȼ�3��
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQͨ����ʹ��
	NVIC_Init(&NVIC_InitStructure);  //����NVIC_InitStruct��ָ���Ĳ�����ʼ������NVIC�Ĵ���	

	TIM_ITConfig( TIM4,TIM_IT_Update|TIM_IT_CC4,ENABLE);//��������ж� ,����CC4IE�����ж�								 
}

//ң��������״̬
//[7]:�յ����������־
//[6]:�õ���һ��������������Ϣ
//[5]:����	
//[4]:����½����Ƿ��Ѿ�������								   
//[3:0]:�����ʱ��
u8 RmtSta=0;	  	
u8 nFlag = 0;
u8 nData = 0;
u16 Dval;		//�½���ʱ��������ֵ
u32 RmtRec=0;	//������յ�������	   		    
u8  RmtCnt=0;	//�������µĴ���	  

//��ʱ��4�жϷ������	 
void TIM4_IRQHandler(void)
{ 		    	 
    if(TIM_GetITStatus(TIM4,TIM_IT_Update)!=RESET)      //�����ж�
	{
		
        if(RmtSta&0x80)//�ϴ������ݱ����յ���
		{	
			RmtSta &= ~0x10;						//ȡ���������Ѿ���������
			if((RmtSta&0x0F) == 0x00) RmtSta |= 1<<6;//����Ѿ����һ�ΰ����ļ�ֵ��Ϣ�ɼ�
			if((RmtSta&0x0F) < 15) RmtSta++;         //��������λ��������
			else
			{
				RmtSta&=~(1<<7);//���������ʶ
				RmtSta&=0xF0;	//��ռ�����	
			}
//             printf("TIMEL=%d\r\n",RmtRec);        
		}							    
	}
 	if(TIM_GetITStatus(TIM4,TIM_IT_CC4)!=RESET)
	{	  
		if(!RDATA)      //�͵�ƽ�������½��ز���, ���������ز�����TIM4
		{
            Dval = TIM_GetCapture4(TIM4);    //��ȡǰһ���ߵ�ƽ��ʱ��
            TIM_SetCounter(TIM4,0);	   	    //��ն�ʱ��ֵ�����¿�ʼ�����͵�ƽʱ��	
            TIM_OC4PolarityConfig(TIM4,TIM_ICPolarity_Rising);		//CC4P=1 ����Ϊ�����ز���	
			
            if(RmtSta&0x80)//���յ�����λ
            {
               
                if(Dval>1500 && Dval<2000)	    //1.5ms--2ms����׼��1.688ms��
                {
                    nFlag = 0;
                }                 
            }
            RmtSta|=0x10;				    //����½����Ѿ�������
		}
        else //�ߵ�ƽ�����������ز��񣬿����½��ز�����TIM4
		{			
  			Dval=TIM_GetCapture4(TIM4);    //��ȡCCR4Ҳ������CC4IF��־λ
			TIM_OC4PolarityConfig(TIM4,TIM_ICPolarity_Falling); //CC4P=0	����Ϊ�½��ز���.
            TIM_SetCounter(TIM4,0);	   	    //��ն�ʱ��ֵ�����¿�ʼ�����ߵ�ƽʱ��	
/******************************************************************************/					
		//RC-5����һλ���ݣ���1������Ϊ���ȸߵ�ƽ0.844ms+�͵�ƽ0.844ms
		//RC-5����һλ���ݣ���0������Ϊ���ȵ͵�ƽ0.844ms+�ߵ�ƽ0.844ms
		//������յĵ͵�ƽΪ1.688ms����˵��������һ���ߵ�ƽ��һ���͵�ƽ��������λ2
		//ֻ��ǰ�ε͵�ƽʱ��Ϊ1.688ms(TimL>=75 && TimL<=90)������Ķ̵ĵ͵�ƽΪ��0�������������Ϊ��1��
/******************************************************************************/ 			
			if(RmtSta&0x10)					//�½����Ѿ�����
			{
 				if(RmtSta&0x80)//���յ�����λ
				{										
                    if(Dval>600 && Dval<1200)			//0.7ms--0.98ms����׼��0.844ms��
					{
                        if(nFlag == 1)
                        {
                            nData = 0;          //�յ�����0
                            RmtRec <<= 1;	        //����һλ.
                            RmtRec += nData;	//���յ�0	
                        }
                        else
                        {
                            nData = 1;          //�յ�����1
                            RmtRec <<= 1;         //����һλ.
                            RmtRec += nData;	//���յ�1
                        }
                           
					}
                    else if(Dval>1500 && Dval<2000)	//1.5ms--2ms����׼��1.688ms��
					{
						nFlag = 1;
                        nData = 2;              //�յ���λ����1��0���� 2
                        RmtRec <<= 2;	        //����2λ.
						RmtRec += nData;	    //���յ�
					}
//                     else			//�ź��쳣�������Ǹ��ţ�
// 					{
// 						printf("�����������쳣�����ذ�ң�ذ��� TIMEL=%d\r\n",Dval);
// 						
// 					}		
 				}
                else if(Dval>600 && Dval<1200)		//δ������λ����˴�Ϊ��λ���գ��͵�ƽ��׼ֵΪ0.844ms
				{
					RmtSta|=1<<7;	//��ǳɹ����յ�����λ����
					RmtRec=1;	    //��λΪ1
                    RmtCnt=0;		//�����������������
				}						 
			}
			RmtSta&=~(1<<4);
		}				 		     	    					   
	}
 TIM_ClearFlag(TIM4,TIM_IT_Update|TIM_IT_CC4);	    
}

//����������
//����ֵ:
//	 0,û���κΰ�������
//����,���µİ�����ֵ.
u8 Remote_Scan(void)
{        
	u8 sta = 0;       
    u8 t1,t2;  
	if(RmtSta&(1<<6))//�õ�һ��������������Ϣ��
	{ 
	    sta = (RmtRec &= 0x000003F);			//�õ���ַ��
//	    t2=(RmtRec>>16)&0xff;	//�õ���ַ���� 
// 	    if((t1==(u8)~t2)&&t1==REMOTE_ID)//����ң��ʶ����(ID)����ַ 
//	    { 
//	        t1=RmtRec>>8;
//	        t2=RmtRec; 	
//	        if(t1==(u8)~t2)sta=t1;//��ֵ��ȷ	 
//		}   
//		if((sta==0)||((RmtSta&0X80)==0))//�������ݴ���/ң���Ѿ�û�а�����
//		{
//		 	RmtSta&=~(1<<6);//������յ���Ч������ʶ
//			RmtCnt=0;		//�����������������
//		}
        RmtSta&=~(1<<6);//������յ���Ч������ʶ
	}  
    return sta;
}
































