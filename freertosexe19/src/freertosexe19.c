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
#include "event_groups.h"

#define GPIO_DEVICE_ID XPAR_GPIO_0_DEVICE_ID
#define LED1 (0x01 << 20)
#define LED2 (0x01 << 21)
#define dhtPin (0x01 << 27)

#define event_read (1 << 0)
#define event_print (1 << 1)

void TaskDHT();
void TaskDelay();
void TaskPrint();

TaskHandle_t TaskDHTID ;
TaskHandle_t TaskDelayID;
TaskHandle_t TaskPrintID;

Dht *sensor ;
XGpio Gpio;
#define LED_CHANNEL 1

int  temperature ;
int  humidity ;
int global;

EventGroupHandle_t  eventgroup ;

int main(){


	//Initialize GPIO
	int Status = XGpio_Initialize(&Gpio, XPAR_GPIO_0_DEVICE_ID);
	if (Status != XST_SUCCESS) {
			xil_printf("Gpio Initialization Failed\r\n");
			return XST_FAILURE;
		}

	sensor = Dht_init(&Gpio,dhtPin);

    xTaskCreate(TaskDHT, "TaskDHT", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, &TaskDHTID);
    xTaskCreate(TaskDelay, "TaskDelay", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &TaskDelayID);
    xTaskCreate(TaskPrint, "TaskPrint", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &TaskPrintID);


    vTaskStartScheduler(); // Start the scheduler

    while(1); // Should never reach here
    return 0;

}

void TaskDHT() {
	uint32_t notificationValue ;
    while(1) {

        xTaskNotifyWait(0, 0, &notificationValue, portMAX_DELAY); // Wait indefinitely for the notification
    	Dht_takeData(sensor);
    	Dht_getResult(sensor, &temperature, &humidity);
    	global=((temperature<<16)|humidity);

        xTaskNotify(TaskPrintID, (uint32_t)global, eSetValueWithoutOverwrite); // Send data directly to print task

    }
}

void TaskPrint() {
	while(1){
		uint32_t receivedValue ;
        xTaskNotifyWait(0, 0, &receivedValue, portMAX_DELAY);
        humidity = (receivedValue & 0x7777);
       // temperature = (receivedValue & 0x7700);
        temperature = (receivedValue>>16);
		xil_printf("temperature = %d  , humidity = %d \r\n" , temperature ,humidity);
	}

}

void TaskDelay() {
    const TickType_t xDelay = pdMS_TO_TICKS(2000); // 2 seconds delay
    while(1) {
        vTaskDelay(xDelay); // Delay for 2 seconds
        xTaskNotify(TaskDHTID, 0, eNoAction); // Notify DHT task to start reading


    }
}
