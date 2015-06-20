/**
*****************************************************************************
**
**  File        : main.c
**
**  Abstract    : main function.
**
**  Functions   : main
**
**  Environment : Atollic TrueSTUDIO(R)
**
**  Distribution: The file is distributed
**                of any kind.
**
**  (c)Copyright Atollic AB.
**  You may use this file as-is or modify it according to the needs of your
**  project. This file may only be built (assembled or compiled and linked)
**  using the Atollic TrueSTUDIO(R) product. The use of this file together
**  with other tools than Atollic TrueSTUDIO(R) is not permitted.
**
*****************************************************************************
*/

/* Includes */
#include <stdint.h>
#include "stm32f30x.h"
#include "stm32f30x_gpio.h"
#include "stm32f30x_conf.h"
#include "stm32f3_discovery.h"
#include <stdio.h>
#include <math.h>
#include "timer.h"
#include "io.h"
#include "sound.h"
//#include "game.h"
#include "ff.h"
//#include "score.h"
/* Private typedef */

/* Private define  */

/* Private macro */

/* Private variables */
uint32_t buttonState=0;

/* Private function prototypes */
/* Private functions */

/*
 * no out: PF0,1
 * half out: PE8-15 (led)
 *
 * check:
 * PE0-5 PC14,15 PB3,6,7 PA5-7,11-14
 */

IOPin pins[8]={
		{bC,P7},
		{bD,P14},
		{bD,P12},
		{bD,P10},
		{bD,P8},
		{bB,P14},
		{bB,P12},
		{bB,P10},
		/*{GPIOA,GPIO_Pin_5},//out	in
		{GPIOA,GPIO_Pin_6},//out	in
		{GPIOA,GPIO_Pin_7},//out	in
		{GPIOA,GPIO_Pin_11},//0/0
		{GPIOA,GPIO_Pin_12},//0/0
		{GPIOA,GPIO_Pin_13},//1.5/3  3/3 break?
		{GPIOA,GPIO_Pin_14},//0/0	in  break?

		{GPIOB,GPIO_Pin_3},//0/0
		{GPIOB,GPIO_Pin_6},//3/3
		{GPIOB,GPIO_Pin_7},//3/3	3/3

		{GPIOC,GPIO_Pin_14},//0/0
		{GPIOC,GPIO_Pin_15},//0/0

		{GPIOE,GPIO_Pin_0},//0/.4	in
		{GPIOE,GPIO_Pin_1},//0/.4	in
		{GPIOE,GPIO_Pin_2},//2.4/3	3/3
		{GPIOE,GPIO_Pin_3},//out	in
		{GPIOE,GPIO_Pin_4},//0/.4	in
		{GPIOE,GPIO_Pin_5},//0/0	in*/
};
/**
 * 62 io
 * 4 i
 * 8 led
 * 1 button
 * 10 bad
 *
**===========================================================================
**
**  Abstract: main program
**
**===========================================================================
*/

uint32_t ii;
uint8_t bsint[]={127,130,133,136,139,142,145,148,151,155,158,161,164,167,170,173,175,178,181,184,187,190,192,195,198,200,203,205,208,210,212,215,217,219,221,223,225,227,229,231,233,235,236,238,239,241,242,244,245,246,247,248,249,250,251,251,252,253,253,254,254,254,254,254,254,254,254,254,254,254,253,253,252,251,251,250,249,248,247,246,245,244,242,241,239,238,236,235,233,231,229,227,225,223,221,219,217,215,212,210,208,205,203,200,198,195,192,190,187,184,181,178,175,173,170,167,164,161,158,155,151,148,145,142,139,136,133,130,126,123,120,117,114,111,108,105,102,98,95,92,89,86,83,80,78,75,72,69,66,63,61,58,55,53,50,48,45,43,41,38,36,34,32,30,28,26,24,22,20,18,17,15,14,12,11,9,8,7,6,5,4,3,2,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,2,3,4,5,6,7,8,9,11,12,14,15,17,18,20,22,24,26,28,30,32,34,36,38,41,43,45,48,50,53,55,58,61,63,66,69,72,75,78,80,83,86,89,92,95,98,102,105,108,111,114,117,120,123};
#define  bsin(a) (((uint8_t)(bsint[((uint8_t)((a)))])))
#define MAX_CIRCLE_ANGLE      512
#define HALF_MAX_CIRCLE_ANGLE (MAX_CIRCLE_ANGLE/2)
#define QUARTER_MAX_CIRCLE_ANGLE (MAX_CIRCLE_ANGLE/4)
#define MASK_MAX_CIRCLE_ANGLE (MAX_CIRCLE_ANGLE - 1)
#define PI 3.14159265358979323846f
float _sin(float n)
{
   float f = n * HALF_MAX_CIRCLE_ANGLE / PI;
   int i=f;
   if (i < 0)
   {
      return fast_cossin_table[(-((-i)&MASK_MAX_CIRCLE_ANGLE)) + MAX_CIRCLE_ANGLE];
   }
   else
   {
      return fast_cossin_table[i&MASK_MAX_CIRCLE_ANGLE];
   }

}
//return 1 to disable timer, 0 to repeat
uint32_t disableLight(void *d)
{
	setOut(pins[(int)d],0);
	STM_EVAL_LEDOff(LED3+(int)d);
	return 1;
}
extern uint8_t mLEDState[];
extern uint8_t LED_Dirty;
uint8_t pwmFunc(void *d)
{
	unsigned char pwm=bsin((msElapsed+(int)d*300)*(int)(256.f/2.5)/1000);//(float)_sin((float)(msElapsed+(int)d*300)/1000.f*2*PI/1.8)*128+127;//
	return pwm;
}
Solenoid* solenoids[] = {
		&HOLD,
		&SCORE[0],
		&SCORE[1],
		&SCORE[2],
		&SCORE[3],
		&BONUS[0],
		&BONUS[1],
		&BONUS[2],
		&BONUS[3],
		&BALL_SHOOT,
		&LEFT_DROP_RESET,
		&RIGHT_DROP_RESET,
		&TOP_DROP_RESET,
		&FIVE_DROP_RESET,
		&LEFT_CAPTURE_EJECT,
		&RIGHT_CAPTURE_EJECT,
		&TOP_CAPTURE_EJECT,
		&BALL_ACK

};
uint32_t timerFunc(void* data) {

	STM_EVAL_LEDToggle(LED8);
	return 0;
}
void switchPlayerRelay(int n);

char *soundFiles[]={
		"",
		""/*cUSE_EXTRA_BALL*/,
		"succmrev.raw"/*cJACKPOT*/,
		"out1.raw"/*cBALL_OUT*/,
		"exblecho.raw"/*cEXTRA_BALL*/,
		""/*cCOLLECT_BONUS*/,
		"siren.raw"/*cLOCK_START*/,
		"captpos1.raw"/*cBALL_CAPTURED_MB*/,
		"captstay.raw"/*cBALL_CAPTURED_LOCK*/,
		""/*cJACKPOT_READY*/,
		"exblgntl.raw"/*cEXTRA_BALL_READY*/,
		""/*cCOLLECT_BONUS_READY*/,
		""/*cBALLS_LOCKED*/,
		"drpcpgen.raw"/*cMB_READY*/,
		"lanepos1.raw"/*cLANE_COMPLETE*/,
		"shoot1.raw"/*cSHOOT*/,
		"drpsplng.raw"/*cTHREE_COMPLETE*/,
		"wrong2.raw"/*cBALL_CAPTURED_REJECTED*/,
		"drop3gen.raw"/*cFIVE_HIT*/,/////////////////////////////
		"shotcare.raw"/*cSHOOT_CAREFULLY*/,
		"red-pos.raw"/*cRED_HIT*/,
		"lanegen1.raw"/*cLANE_HIT*/,
		"captneg2.raw"/*cBALL_CAPTURE_EJECT*/,
		"drpspsht.raw"/*cTHREE_HIT*/,
		"captneg1.raw"/*cACTIVATE_MISS*/,
		""/*cROLLOVER*/,
		"red-neg.raw"/*cRED_MISS*/,
		"drop3gen.raw"/*cFIVE_MISS*/,
		""/*cPOP_HIT*/,
		"drpshtgn.raw"/*cTHREE_MISS*/,
		""/*cLANE_MISS*/,
};

int main(void)
{

	/* Example use SysTick timer and read System core clock */
	SysTick_Config(72000);  /* 1  ms if clock frequency 72 MHz */
	initTimers();
	//initIOs();
#ifdef SOUND
	initSound();
	//initDisplay();
#else
	//initScores();
	//initGame();
#endif
	//setOut(HOLD.pin,1);
	//fireSolenoid(&HOLD);
	STM_EVAL_PBInit(BUTTON_USER,BUTTON_MODE_GPIO);
	int i;
//9 on p1,p3
	//0 on p2-1000,p4
	SystemCoreClockUpdate();
	for(i=0;i<8;i++)
		STM_EVAL_LEDInit(LED3+i);
	//STM_EVAL_LEDToggle(LED3);
	ii = 0;
	int iii=0;
	int iiii=3;
	int on=0;
	//STM_EVAL_LEDOn(LED4);
	for(int i=0;i<nLED;i++)
	{
		//setPWMFunc(i,pwmFunc,i);
		//setPWM(i,255);
		//setLed(i,ON);
		//setFlash(i,3000);
		//offsetLed(i,3000*(i%2));
		//setLed(i,FLASHING);
	}
	//setLed(0,FLASHING);
	//callFuncIn(timerFunc,1000,NULL);
	//switchPlayerRelay(1);
	LED_Dirty=1;

#define COM_CLOCK P3
#define COM_DATA P2
#define COM_ACK P4

	 GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin =  COM_CLOCK|COM_DATA;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOF, ENABLE);
	GPIO_InitStructure.GPIO_Pin =  COM_ACK;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOF, &GPIO_InitStructure);
	GPIO_WriteBit(GPIOF,COM_ACK,0);

	uint8_t command=0;
	uint8_t nCommand=0;
	uint8_t cClock=1;
	uint32_t lastCommTime=0;


	uint32_t temp=0;
	while (1)
	{
		//updateIOs();
		uint8_t clk=GPIO_ReadInputDataBit(GPIOA,COM_CLOCK);
		if(clk && !cClock) {
			lastCommTime=msElapsed;
			command>>=1;
			command|=GPIO_ReadInputDataBit(GPIOA,COM_DATA)<<7;
			GPIO_WriteBit(GPIOF,COM_ACK,0);
			nCommand++;
			if(nCommand==8) {
				switch(command) {
				case 'A':
					paused=1;
					break;
				case 'L':
					paused=0;
					break;
				default:
					loadSound(soundFiles[command]);
				}
				command=0;
				nCommand=0;
				lastCommTime=0;
			}
		}
		else if(!clk && cClock) {
			GPIO_WriteBit(GPIOF,COM_ACK,1);
		}
		if(!cClock && msElapsed-lastCommTime>100 && lastCommTime!=0) {
			nCommand=0;
			command=0;
		}
		//if(msElapsed%5000==0)
		//	loadSound("red-pos.raw");
		cClock=clk;
		//if(temp++==5000)
		if(buttonState!=STM_EVAL_PBGetState(BUTTON_USER))
		{
			buttonState=!buttonState;
			if(buttonState)
			{
				loadSound("red-pos.raw");
			}
		}
#ifdef SOUND
		//updateDisplay();
		updateSound();

#else

		if(1)
		{
			//updateGame();
		}
#endif
	}
	return 0;
}
#if 1
void IRQHander()
{
	printf("");
}
void WWDG_IRQHandler()
{IRQHander();
}
                   /* Window WatchDog */
void PVD_IRQHandler()
{IRQHander();
}
                    /* PVD through EXTI Line detection */
void TAMPER_STAMP_IRQHandler()
{IRQHander();
}
           /* Tamper and TimeStamps through the EXTI line */
void RTC_WKUP_IRQHandler()
{IRQHander();
}
               /* RTC Wakeup through the EXTI line */
void FLASH_IRQHandler()
{IRQHander();
}
                  /* FLASH */
void RCC_IRQHandler()
{IRQHander();
}
                    /* RCC */
void EXTI0_IRQHandler()
{IRQHander();
}
                  /* EXTI Line0 */
void EXTI1_IRQHandler()
{IRQHander();
}
                  /* EXTI Line1 */
void EXTI2_TS_IRQHandler()
{IRQHander();
}
               /* EXTI Line2 and Touch Sense */
void EXTI3_IRQHandler()
{IRQHander();
}
                  /* EXTI Line3 */
void EXTI4_IRQHandler()
{IRQHander();
}
                  /* EXTI Line4 */
void DMA1_Channel1_IRQHandler()
{IRQHander();
}
          /* DMA1 Channel 1 */
void DMA1_Channel2_IRQHandler()
{IRQHander();
}
          /* DMA1 Channel 2 */
void DMA1_Channel3_IRQHandler()
{IRQHander();
}
          /* DMA1 Channel 3 */
void DMA1_Channel4_IRQHandler()
{IRQHander();
}
          /* DMA1 Channel 4 */
void DMA1_Channel5_IRQHandler()
{IRQHander();
}
          /* DMA1 Channel 5 */
void DMA1_Channel6_IRQHandler()
{IRQHander();
}
          /* DMA1 Channel 6 */
void DMA1_Channel7_IRQHandler()
{IRQHander();
}
          /* DMA1 Channel 7 */
void ADC1_2_IRQHandler()
{IRQHander();
}
                 /* ADC1 and ADC2 */
void USB_HP_CAN1_TX_IRQHandler()
{IRQHander();
}
         /* USB Device High Priority or CAN1 TX */
void USB_LP_CAN1_RX0_IRQHandler()
{IRQHander();
}
        /* USB Device Low Priority or CAN1 RX0 */
void CAN1_RX1_IRQHandler()
{IRQHander();
}
               /* CAN1 RX1 */
void CAN1_SCE_IRQHandler()
{IRQHander();
}
               /* CAN1 SCE */
void EXTI9_5_IRQHandler()
{IRQHander();
}
                /* External Line[9:5]s */
            /* TIM4 */
void I2C1_EV_IRQHandler()
{IRQHander();
}
                /* I2C1 Event */
void I2C1_ER_IRQHandler()
{IRQHander();
}
                /* I2C1 Error */
void I2C2_EV_IRQHandler()
{IRQHander();
}
                /* I2C2 Event */
void I2C2_ER_IRQHandler()
{IRQHander();
}
                /* I2C2 Error */
void SPI1_IRQHandler()
{IRQHander();
}
                   /* SPI1 */
void SPI2_IRQHandler()
{IRQHander();
}
                   /* SPI2 */
void USART1_IRQHandler()
{IRQHander();
}
                 /* USART1 */
void USART2_IRQHandler()
{IRQHander();
}
                 /* USART2 */
void USART3_IRQHandler()
{IRQHander();
}
                 /* USART3 */
void EXTI15_10_IRQHandler()
{IRQHander();
}
              /* External Line[15:10]s */
void RTC_Alarm_IRQHandler()
{IRQHander();
}
              /* RTC Alarm (A and B) through EXTI Line */
void USBWakeUp_IRQHandler()
{IRQHander();
}

void ADC3_IRQHandler()
{IRQHander();
}
                   /* ADC3 */

                                 /* Reserved */
void SPI3_IRQHandler()
{IRQHander();
}
                   /* SPI3 */
void UART4_IRQHandler()
{IRQHander();
}
                  /* UART4 */
void UART5_IRQHandler()
{IRQHander();
}

void DMA2_Channel1_IRQHandler()
{IRQHander();
}
          /* DMA2 Channel 1 */
void DMA2_Channel2_IRQHandler()
{IRQHander();
}
          /* DMA2 Channel 2 */
void DMA2_Channel3_IRQHandler()
{IRQHander();
}
          /* DMA2 Channel 3 */
void DMA2_Channel4_IRQHandler()
{IRQHander();
}
          /* DMA2 Channel 4 */
void DMA2_Channel5_IRQHandler()
{IRQHander();
}
          /* DMA2 Channel 5 */
void ADC4_IRQHandler()
{IRQHander();
}
                   /* ADC4 */
                         /* Reserved */
void COMP1_2_3_IRQHandler()
{IRQHander();
}
              /* COMP1, COMP2 and COMP3 */
void COMP4_5_6_IRQHandler()
{IRQHander();
}
              /* COMP4, COMP5 and COMP6 */
void COMP7_IRQHandler()
{IRQHander();
}
                  /* COMP7 */

                                 /* Reserved */
void USB_HP_IRQHandler()
{IRQHander();
}
                 /* USB High Priority remap */
void USB_LP_IRQHandler()
{IRQHander();
}
                 /* USB Low Priority remap */
void USBWakeUp_RMP_IRQHandler()
{IRQHander();
}

                                 /* Reserved */
void FPU_IRQHandler()
{IRQHander();
}
                    /* FPU */
#endif
