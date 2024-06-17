#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "xgpio.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "queue.h"
#include "xuartps.h"
#include <stdio.h>
#include <string.h>
#include "dht.h"
#include "semphr.h"

#define GPIO_DEVICE_ID XPAR_GPIO_0_DEVICE_ID
#define LED1 (0x01 << 20)
#define LED2 (0x01 << 21)
#define dhtPin (0x01 << 27)

void TaskDHT();
void TaskDelay();
void TaskPrint();

SemaphoreHandle_t semDelay;
SemaphoreHandle_t semPrint;
Dht *sensor ;
XGpio Gpio;
#define LED_CHANNEL 1

int  temperature ;
int  humidity ;


int main(){


	//Initialize GPIO
	int Status = XGpio_Initialize(&Gpio, XPAR_GPIO_0_DEVICE_ID);
	if (Status != XST_SUCCESS) {
			xil_printf("Gpio Initialization Failed\r\n");
			return XST_FAILURE;
		}

	sensor = Dht_init(&Gpio,dhtPin);

    xTaskCreate(TaskDHT, "TaskDHT", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(TaskDelay, "TaskDelay", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(TaskPrint, "TaskPrint", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

    vSemaphoreCreateBinary(semDelay);
    vSemaphoreCreateBinary(semPrint);


    vTaskStartScheduler(); // Start the scheduler

    while(1); // Should never reach here
    return 0;

}

void TaskDHT() {
    while(1) {
    	xSemaphoreTake(semDelay, portMAX_DELAY);
    	Dht_takeData(sensor);
    	Dht_getResult(sensor, &temperature, &humidity);
    	xSemaphoreGive(semPrint);

    }
}

void TaskPrint() {
	while(1){
		xSemaphoreTake(semPrint, portMAX_DELAY);
		xil_printf("temperature = %d  , humidity = %d \r\n" , temperature ,humidity);
	}

}

void TaskDelay() {
    const TickType_t xDelay = pdMS_TO_TICKS(2000); // 2 seconds delay
    while(1) {
        vTaskDelay(xDelay); // Delay for 2 seconds
    	xSemaphoreGive(semDelay);

    }
}
