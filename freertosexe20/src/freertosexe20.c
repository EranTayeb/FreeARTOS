#include <stdio.h>
#include "xil_printf.h"
#include "xparameters.h"
#include "FreeRTOS.h"
#include "xgpio.h"
#include "task.h"
#include "XTmrCtr.h"
#include "timers.h"

#define LED0 (1<<21)
#define CHANNEL 1
#define GPIO_DEVICE_ID  XPAR_GPIO_0_DEVICE_ID
#define PERIOD 1000000

XGpio Gpio; /* The Instance of the GPIO*/
XTmrCtr TimerInst;
TimerHandle_t blinkID;
TimerHandle_t brightID;

int initGPIO(int status);

int init_PWM(int status ,u32 PwmPeriod,u32 PwmHighTime);

void toggel();

void blinkFunc();

void changePWM(int bright1);

void brightFunc();

int main(int argc, char **argv) {
	int status = 0;
	status = initGPIO(status);
	if(status != 0)
		printf("problem at initGPIO\n\r");

	u32 PwmHighTime = (PERIOD/100)*99; //Default by me

	status = init_PWM(status,PERIOD, PwmHighTime);
	if(status != 0)
		printf("problem at init_PWM\n\r");

	blinkID = xTimerCreate("blinkTimer",pdMS_TO_TICKS(2000),pdTRUE, NULL,blinkFunc);
	brightID = xTimerCreate("brightTimer",pdMS_TO_TICKS(1500),pdTRUE, NULL,brightFunc);

	xTimerStart(blinkID, 0);
	xTimerStart(brightID, 0);
	// Start scheduler
	vTaskStartScheduler();

}


void toggel(){
	u32 num = 0;
	num = XGpio_DiscreteRead(&Gpio, CHANNEL);
	XGpio_DiscreteWrite(&Gpio,CHANNEL, LED0^num);
}

void blinkFunc(){
	toggel();
}

void changePWM(int bright1){
	u32 PwmHighTime = (PERIOD/100)*bright1; //Default by me
	XTmrCtr_PwmDisable(&TimerInst);
	XTmrCtr_PwmConfigure(&TimerInst, PERIOD , PwmHighTime);
	XTmrCtr_PwmEnable(&TimerInst); //enable the timer instance to work
}

//init the PWM instance
int init_PWM(int status ,u32 PwmPeriod,u32 PwmHighTime){
	status = XTmrCtr_Initialize(&TimerInst, XPAR_AXI_TIMER_0_DEVICE_ID); // init the timer instance
	if (status != 0) {
		printf("XUartPs_CfgInitialize Failed\r\n");
		return XST_FAILURE;
	}
	XTmrCtr_PwmDisable(&TimerInst);
	XTmrCtr_PwmConfigure(&TimerInst, PwmPeriod, PwmHighTime);
	XTmrCtr_PwmEnable(&TimerInst); //enable the timer instance to work
	return 0;
}

int initGPIO(int status){
	status = XGpio_Initialize(&Gpio, GPIO_DEVICE_ID);   /* initialize GPIO*/
	if (status != 0) {
		xil_printf("Gpio Initialization Failed\r\n");
		return XST_FAILURE;
	}
	XGpio_SetDataDirection(&Gpio, CHANNEL, ~LED0); //set direction in the i\o register/
	return 0;
}

void brightFunc(){
	static int bright = 99;
	bright-=10;
	if(bright <= 0)
		bright = 99;
	changePWM(bright);
}
