/*
 * sound.c
 *
 *  Created on: Jul 17, 2014
 *      Author: zacaj
 */

#include <stdint.h>
#define SOUND
#ifdef SOUND
#ifndef SDL
#include "stm32f30x.h"
#include "stm32f30x_gpio.h"
#include "stm32f30x_conf.h"
#include "stm32f3_discovery.h"
#endif
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#ifndef SDL
#include "sound.h"
#include "ff.h"
#include "timer.h"
#include "io.h"
const IOPin LATCH = { bA, P9 };
const IOPin CLOCK = { bD, P3 };
const IOPin DATA = { bB, P13 };
#endif

typedef int16_t Sample;

typedef struct {
	uint8_t *data[2];//block of memory from \file
	uint32_t size[2]; //size of \data1
	uint32_t start[2];//location in bytes where \data1 starts in \file
	uint8_t cur;
	uint8_t ready[2];//1 if the data is ready to be read from

	char *name;
	FIL file;
	uint8_t done;//set to 1 to free this sound
	uint32_t f_loc;//location in \file that will be read next
} Sound;

#define MAX_SOUND 4

Sound *sounds[MAX_SOUND];

int loadSound(char *file) {

	int i;
	for(i=0;i<MAX_SOUND;i++)
		if(sounds[i]==NULL)
			break;
	if(i==MAX_SOUND)
		return -1;
	Sound *sound=malloc(sizeof(Sound));
	FRESULT res=f_open(&sound->file,file,FA_READ);
	int n=10;
	while(res!=FR_OK && n--);
	if(res!=FR_OK) {
		free(sound);
		return -2;
	}

	//todo: search through already loaded sounds?
	sound->data[0]=sound->data[1]=NULL;
	sound->ready[0]=sound->ready[1]=0;
	sound->name=file;
	sound->f_loc=0;
	sound->cur=0;
	sound->done=0;
			//data=malloc(finfo->fsize/10);
			//res2=f_read(fp,data,finfo->fsize/10,&read);

	sounds[i]=sound;
	return i;
}


Sample sound_get(int i) {
	Sound *sound=sounds[i];

	if(sound->f_loc>=sound->file.fsize)
		sound->done=1;
	if(!sound->ready[sound->cur] || sound->f_loc>=sound->file.fsize)
		return 0;

	uint32_t off=sound->f_loc - sound->start[sound->cur];
	if(off>=sound->size[sound->cur]) {
		sound->ready[sound->cur]=0;
		free(sound->data[sound->cur]);
		sound->data[sound->cur]=NULL;
		sound->cur=!sound->cur;
		return sound_get(i);
	}
	else {
		sound->f_loc+=sizeof(Sample);
		Sample t=*((Sample*)(sound->data[sound->cur]+off));
		return t;
	}
}


const double rate = 16000;
double t = 0;
#define NOLD 0
int old[NOLD];
int at = NOLD;
uint8_t paused=0;
int a = 0;

uint32_t audio(void *data)
{
	if(!paused) {
		uint8_t out=128;

		int32_t s=0;
		for(int i=0;i<MAX_SOUND;i++) {
			if(sounds[i]==NULL)
				continue;
			Sample ss=sound_get(i);
			s+=ss;
		}

		//out=(s/256)+127;//(at/4)<2?0:255;//
		//out=s;
		//out=(at%4)<2? 0:255;
		//GPIO_Write(bD,out<<8);
	/*	if(s)
			STM_EVAL_LEDOn(LED4);
		else
			STM_EVAL_LEDOff(LED4);*/
		int32_t S=((int32_t)s)+65535/2;
		DAC_SetChannel1Data(DAC_Align_12b_R, S>>4);
		at++;
		t += 1.0 / rate;
	}
	else
		DAC_SetChannel1Data(DAC_Align_12b_R, (65535/2)>>4);
	return 0;
}

/////////////////////////////////////

uint32_t lastSoundLoadAttempt=0;

void updateSound() {
	for(int i=0;i<MAX_SOUND;i++) {
		if(sounds[i]==NULL)
			continue;
		if(sounds[i]->done) {
			if(sounds[i]->data[0]!=NULL)
				free(sounds[i]->data[0]);
			if(sounds[i]->data[1]!=NULL)
				free(sounds[i]->data[1]);
			f_close(&sounds[i]->file);
			sounds[i]=NULL;
			continue;
		}
		for(int j=0;j<2;j++) {
			if(lastSoundLoadAttempt!=0 && msElapsed-lastSoundLoadAttempt<10)
				continue;
			if(sounds[i]->data[j]==NULL) {
				uint32_t next=sounds[i]->f_loc;
				uint32_t otherEnd=next;
				if(sounds[i]->ready[!j])
					otherEnd=sounds[i]->start[!j]+sounds[i]->size[!j];
				if(otherEnd>next)
					next=otherEnd;
				if(next>sounds[i]->file.fsize) {
					next=0;
				}
				uint32_t amt=8192/2;
				if(next+amt>sounds[i]->file.fsize)
					amt=sounds[i]->file.fsize-next;


				sounds[i]->ready[j]=0;
				if(amt==0)
					continue;
				sounds[i]->data[j]=malloc(amt);

				UINT read=-1;
				f_lseek(&sounds[i]->file,next);
				FRESULT res=f_read(&sounds[i]->file,sounds[i]->data[j],amt,&read);
				if(res==FR_OK && read==amt) {
					sounds[i]->size[j]=amt;
					sounds[i]->start[j]=next;
					sounds[i]->ready[j]=1;
				}
				else {
					free(sounds[i]->data[j]);
					sounds[i]->data[j]=NULL;
				}
				lastSoundLoadAttempt=msElapsed;
			}
		}
	}
}

void initSound() {
	for (int i = 0; i < NOLD; i++)
		old[i] = 0;
	/*initOutput(LATCH);
	initOutput(CLOCK);
	initOutput(DATA);
	setOut(LATCH, 1);
	setOut(CLOCK, 0);*/

	for(int i=0;i<MAX_SOUND;i++)
		sounds[i]=NULL;
#ifndef SDL

	FATFS *fs=malloc(sizeof(FATFS));
	FILINFO *finfo=malloc(sizeof(FILINFO));
	FRESULT res;
	while((res=f_mount(0,fs))==FR_NOT_READY);
	while((res=f_stat("empty",finfo))==FR_NOT_READY);
	if(res!=FR_OK)

		STM_EVAL_LEDOn(LED3);


	//loadSound("red-pos.raw");
	//


	/* DMA2 clock enable (to be used with DAC) */
	  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);

	  /* DAC Periph clock enable */
	  RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

	  /* GPIOA clock enable */
	  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

	  /* Configure PA.04 (DAC_OUT1) as analog */
	  GPIO_InitTypeDef GPIO_InitStructure;
	  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_4;
	  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	  GPIO_Init(GPIOA, &GPIO_InitStructure);
      DAC_DeInit();
      DAC_InitTypeDef            DAC_InitStructure;

      DAC_InitStructure.DAC_Trigger = DAC_Trigger_None;
	  DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
	  DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
	  DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
	  DAC_Init(DAC_Channel_1, &DAC_InitStructure);

	  /* Enable DAC Channel1: Once the DAC channel1 is enabled, PA.05 is
	  automatically connected to the DAC converter. */
	  DAC_Cmd(DAC_Channel_1, ENABLE);

	  /* Set DAC Channel1 DHR12L register */
	  DAC_SetChannel1Data(DAC_Align_12b_L, 65535/2);


	/*RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD, ENABLE);

	GPIO_InitTypeDef init;
	init.GPIO_Mode = GPIO_Mode_OUT;
	init.GPIO_OType = GPIO_OType_PP;
	init.GPIO_Pin = GPIO_Pin_All;
	init.GPIO_PuPd = GPIO_PuPd_NOPULL;
	init.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(bD, &init);
*/
	/*RCC_PCLK2Config(RCC_HCLK_Div2);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
	{
		GPIO_InitTypeDef GPIO_InitStructure;
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_15;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_NOPULL;
		GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
		GPIO_Init(bB, &GPIO_InitStructure);
	}
	GPIO_PinAFConfig(bB,GPIO_Pin_13,GPIO_AF_5);
	GPIO_PinAFConfig(bB,GPIO_Pin_15,GPIO_AF_5);

	SPI_InitTypeDef   SPI_InitStructure;
	SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_16b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI2, &SPI_InitStructure);
	SPI_Cmd(SPI2, ENABLE);*/
	  wait(200);
	callFuncInCustom(audio, 72000000.0 / rate / 100 - 1, 100 - 1, NULL);
#endif
	//callFuncInCustom(audio,8,1125,NULL);
	//callFuncInCustom(audio,16,1125,NULL);
	//callFuncInCustom(audio,500,23,NULL);
	//callFuncInCustom(audio,1623,,NULL);
}

#else
void initSound() {}
#endif
