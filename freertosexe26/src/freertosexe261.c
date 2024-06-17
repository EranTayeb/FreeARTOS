#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "XGpio.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "xuartps.h"
#include <stdio.h>
#include <string.h>
#include "timers.h"
#include "XTmrCtr.h"
#include "XScuGic.h"

#define LED1_PIN (0x01 << 20)
#define LED2_PIN (0x01 << 21)
#define GPIO_DEVICE_ID XPAR_GPIO_0_DEVICE_ID
#define LED_CHANNEL 1
#define DeviceId 0
#define MASK_BUTTON (0x01 <<17)
extern XScuGic xInterruptController;
XGpio Gpio; /* The Instance of the GPIO Driver */
TaskHandle_t hTaskMain, hCLI;
void TaskMain(void *pvParameters);
int buffer[]={0,262,294,330,349,392,440,494,523,587};
int buffertimesong[]={400,400,800,400,400,800,400,400,800,400,400,800,400,400,400,400};
int bufferfrequencysong[]={392,330,330,349,294,294,262,294,330,349,392,392,392,392,330,330};
int buffesize = 16 ;


XGpio Gpio; /* The Instance of the GPIO Driver */


void CLI(void *pvParameters);
int init_PWM();
void changePWM(int bright);
void stopPWM();
void init_BUTTON() ;
void vButtonISR(void *CallBackRef);
void song(void *pvParameters);

XTmrCtr TimerInst;
XUartPs InstancePtr_uart;
int PwmPeriod = (1000000000/262);
int PwmHighTime;
TimerHandle_t xTimer1;
TimerHandle_t xTimersong;


int count=0;
int countsong=0;


int main() {

	XUartPs_Config *Config = XUartPs_LookupConfig(DeviceId);
	XUartPs_CfgInitialize(&InstancePtr_uart, Config,Config->BaseAddress);



	xTaskCreate(TaskMain, "TaskMain", (configMINIMAL_STACK_SIZE * 2), NULL,tskIDLE_PRIORITY + 1, &hTaskMain);
	xTaskCreate(CLI, "CLI", configMINIMAL_STACK_SIZE, NULL,tskIDLE_PRIORITY + 1, &hCLI);
	xTimer1 = xTimerCreate("Timer1", pdMS_TO_TICKS(1000), pdFALSE,NULL,(TimerCallbackFunction_t)stopPWM);
	xTimersong = xTimerCreate("Timer1", pdMS_TO_TICKS(1000), pdFALSE,NULL,(TimerCallbackFunction_t)song);


	vTaskStartScheduler(); // Start the scheduler

	while (1) {
		; // Should never reach here
	}
	return 0;
}

// LED Task
void TaskMain(void *pvParameters) {
	int led1_2 = LED1_PIN | LED2_PIN;

	//Initialize Gpio//
	int Status = XGpio_Initialize(&Gpio, XPAR_GPIO_0_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("Gpio Initialization Failed\r\n");
		return XST_FAILURE;
	}
	XGpio_SetDataDirection(&Gpio, LED_CHANNEL, ~led1_2);

	uint32_t receivedValue;
	init_PWM();
	init_BUTTON();

	while(1){
	  xTaskNotifyWait(0, 0, &receivedValue, portMAX_DELAY); // Wait indefinitely for the data
	 // uint32_t receive = (uint32_t)receivedValue;
	  	  if(count==0)
	  	  {
	  		  changePWM(buffer[receivedValue]);
	  		  xTimerStart(xTimer1, 0);
	  	  }
	  	  if(count==1)
	  	  {
		 		xil_printf("working\r\n ");
	 			xTimerStart(xTimersong, 0);
	 			if(countsong == buffesize){
		   		count=0;
		   		countsong = 0 ;
	 			}
	   }
	}

}

void CLI(void *pvParameters) {
	char ch;
	uint32_t num;
	while (1) {
		int Status = XUartPs_Recv(&InstancePtr_uart, (u8*) &ch, 1);
		if (Status != 0) {
			// Echo the received character back to the sender
			XUartPs_Send(&InstancePtr_uart, (u8*) &ch, 1);
			char a=ch-'0';
			num=(uint32_t)a;
			xTaskNotify(hTaskMain,num,eSetValueWithOverwrite);
	//vTaskDelay(pdMS_TO_TICKS(100)); // Delay to debounce input reading
}
	}
}
int init_PWM(){
	int status = XTmrCtr_Initialize(&TimerInst,XPAR_AXI_TIMER_1_DEVICE_ID); // init the timer instance
	if (status != 0) {
		xil_printf("XTmrCtr_Initialize Failed\r\n");
		return XST_FAILURE;
	}
	PwmHighTime=(PwmPeriod/2);
	XTmrCtr_PwmDisable(&TimerInst);
	XTmrCtr_PwmConfigure(&TimerInst, PwmPeriod, PwmHighTime);
	XTmrCtr_PwmEnable(&TimerInst); //enable the timer instance to work
	return 0;
}

void changePWM(int frequency){
	PwmPeriod=(1000000000/frequency);
	 PwmHighTime = (PwmPeriod/2); //Default by me
	XTmrCtr_PwmDisable(&TimerInst);
	XTmrCtr_PwmConfigure(&TimerInst, PwmPeriod , PwmHighTime);
	XTmrCtr_PwmEnable(&TimerInst); //enable the timer instance to work
}
void stopPWM(void *pvParameters){
	XTmrCtr_PwmDisable(&TimerInst);
}
void song(void *pvParameters) {
	if (++countsong < buffesize) {
		changePWM(bufferfrequencysong[countsong]);
		xTimerChangePeriod(xTimersong, pdMS_TO_TICKS(buffertimesong[countsong]),0);
	} else {
		xTimerStart(xTimer1, 0);
	}

}


void init_BUTTON() {
	 xil_printf("init_BUTTON\r\n ");
		// Instead of XScuPsu_Connect
		 xPortInstallInterruptHandler(XPAR_FABRIC_GPIO_0_VEC_ID, (XInterruptHandler) vButtonISR,(void *) &Gpio);
		 XScuGic_SetPriorityTriggerType(
		&xInterruptController, XPAR_FABRIC_GPIO_0_VEC_ID, 0xA0, 0x3);
		 // Enable the interrupt for GPIO channel
		 	XGpio_InterruptEnable(&Gpio, 1);
		 	// Enable all interrupts in the GPIO
		 	XGpio_InterruptGlobalEnable(&Gpio);
		 vPortEnableInterrupt(XPAR_FABRIC_GPIO_0_VEC_ID);
    }
void vButtonISR(void *CallBackRef) {
	BaseType_t pxHigherPriorityTaskWoken;
	u32 val = XGpio_DiscreteRead(&Gpio, 1);
		 	if ((MASK_BUTTON & val) == 0) {
		 		count=1;
		 		xil_printf("vButtonISR\r\n ");
			 	xTaskNotifyFromISR(hTaskMain,0,eNoAction,&pxHigherPriorityTaskWoken);
		 		    }
		 XGpio_InterruptClear(&Gpio, 1);
	 	 portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);




}
