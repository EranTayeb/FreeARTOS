#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "xgpio.h"
#include "XScuGic.h"
#include "xtmrctr.h"
#include "xttcps.h"
#include "queue.h"
#include "stdio.h"
#include "xil_printf.h"
#include "event_groups.h"

#define LED1 (0x01 << 20)
#define LED2 (0x01 << 21)
#define GPIO_DEVICE_ID XPAR_GPIO_0_DEVICE_ID
#define MASK_BUTTON (0x01 << 17)
#define DeviceId 1
#define CHANNEL 1

int test = 0;
int count = 0 ;


extern XScuGic xInterruptController;

TaskHandle_t initbuttonID, hTaskPrint;
int isBUTTON=0;

void vButtonISR(void *CallBackRef);
void initbutton(void *pvParameters);
void ledoff();
TimerHandle_t timer1;

// Global variable for GPIO instance
XGpio Gpio;
//	int Temperature,Humidity=0;
int main(void) {
	int led1_2 = LED1 | LED2;
    // Initialize GPIO
    int Status = XGpio_Initialize(&Gpio, GPIO_DEVICE_ID);
    if (Status != XST_SUCCESS) {
        xil_printf("GPIO Initialization Failed\r\n");
        return XST_FAILURE;
    }
    XGpio_SetDataDirection(&Gpio, 1, ~led1_2);

    timer1 = xTimerCreate("timer1",pdMS_TO_TICKS(5000),pdFALSE, NULL,ledoff);

    xTaskCreate(initbutton, "initbutton", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &initbuttonID);

    // Start the scheduler
    vTaskStartScheduler();

    while (1);
    return 0;
}

// Task to read DHT sensor data
void initbutton(void *pvParameters) {

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
	static TickType_t counttimein ;
	TickType_t counttimeout ;
	TickType_t totaltime ;
	BaseType_t  pxHigherPriorityTaskWoken = pdFALSE;


	 	u32 val = XGpio_DiscreteRead(&Gpio, 1);
	 	if ((MASK_BUTTON & val) == 0 ) {
	 		count++;
	 		if (count == 1){
	 		counttimein =  xTaskGetTickCountFromISR();
	 		}
	 		test = 1 ;
	 		val = XGpio_DiscreteRead(&Gpio, 1);
	 	}
	 	if ((MASK_BUTTON & val) != 0 && test == 1) {
	 			counttimeout =  xTaskGetTickCountFromISR();
	 			totaltime = (counttimeout -counttimein)*portTICK_PERIOD_MS ;
		 		test = 0 ;
		 		xil_printf("%d \n\r",totaltime);
	 			if (totaltime>30 && totaltime<200 ){
	 				XGpio_DiscreteSet(&Gpio,CHANNEL, LED1);
	 				xTimerStartFromISR(timer1,&pxHigherPriorityTaskWoken);
	 				count = 0 ;
	 	}
	 			if (totaltime>200 ){

	 				count = 0 ;
	 	}


	 	}
	 	// Clear the interrupt
	 	XGpio_InterruptClear(&Gpio, 1);
		portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);

}

void ledoff(){
	XGpio_DiscreteClear(&Gpio,CHANNEL, LED1);
}

//&&(pdMS_TO_TICKS(counttimeout-counttimein)<200
