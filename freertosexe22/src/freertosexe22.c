#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "xgpio.h"
#include "dht.h"
#include "XScuGic.h"
#include "xtmrctr.h"
#include "xttcps.h"
#include "queue.h"
#include "stdio.h"
#include "xil_printf.h"
#include "event_groups.h"
#define LED1 0x01 << 20
#define LED2 0x01 << 21
#define DHT_PIN (0x01 << 27)
#define GPIO_DEVICE_ID XPAR_GPIO_0_DEVICE_ID
#define MASK_BUTTON (0x01 <<17)
#define DeviceId 1
extern XScuGic xInterruptController;
TaskHandle_t hTaskDHT, hTaskPrint;
XTtcPs InstancePtr_TTC;
int Temperature, Humidity = 0;
typedef struct {
	int data;
	int ID;
} struct_Data;
void TtcHandler(void *CallBackRef);
void vButtonISR(void *CallBackRef);
void TaskDHT(void *pvParameters);
void TaskPrint(void *pvParameters);
int TTC_Initialization();

// Global variable for GPIO instance
XGpio Gpio;
Dht *dhtSensor;
QueueHandle_t xQueue;


int main(void) {
	int led1_2 = LED1 | LED2;
	// Initialize GPIO
	int Status = XGpio_Initialize(&Gpio, GPIO_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("GPIO Initialization Failed\r\n");
		return XST_FAILURE;
	}
	XGpio_SetDataDirection(&Gpio, 1, ~led1_2);

	dhtSensor = Dht_init(&Gpio, DHT_PIN);
	xQueue = xQueueCreate(5, sizeof(struct_Data));
	// Create tasks
	xTaskCreate(TaskDHT, "TaskDHT", configMINIMAL_STACK_SIZE, NULL,tskIDLE_PRIORITY + 2, &hTaskDHT);
	xTaskCreate(TaskPrint, "TaskPrint", configMINIMAL_STACK_SIZE, NULL,tskIDLE_PRIORITY + 1, &hTaskPrint);

	// Start the scheduler
	vTaskStartScheduler();

	while (1){
		;
	}
	return 0;
}

// Task to read DHT sensor data
void TaskDHT(void *pvParameters) {
	uint32_t notificationValue;
	struct_Data data;
	// Instead of XScuPsu_Connect
	xPortInstallInterruptHandler(XPAR_FABRIC_GPIO_0_VEC_ID,(XInterruptHandler) vButtonISR, (void *) &Gpio);
	XScuGic_SetPriorityTriggerType(&xInterruptController,XPAR_FABRIC_GPIO_0_VEC_ID, 0xA0, 0x3);
	// Enable the interrupt for GPIO channel
	XGpio_InterruptEnable(&Gpio, 1);
	// Enable all interrupts in the GPIO
	XGpio_InterruptGlobalEnable(&Gpio);
	vPortEnableInterrupt(XPAR_FABRIC_GPIO_0_VEC_ID);

	int status = TTC_Initialization();
	if (status != 0) {
		xil_printf("TTC Initialization Failed\r\n");
	}
	if (status == 0) {
		xil_printf("TTC Initialization sucsess\r\n");
	}

	while (1) {
		xTaskNotifyWait(0, 0, &notificationValue, portMAX_DELAY); // Wait indefinitely for the notification
		Dht_takeData(dhtSensor);
		Dht_getResult(dhtSensor, &Temperature, &Humidity);
		//notification is received form the button interrupt
		if (notificationValue == 1) {
			data.ID = 1;
			data.data = Temperature;
		}
		// the notification is received from the timer
		if (notificationValue == 0) {
			data.ID = 0;
			data.data = Humidity;
		}
		int status = xQueueSendToBack(xQueue, (void* )&data, 0);
		if (status != pdPASS) {
			printf("Queue is FULL\r\n");
		}
		taskYIELD()
		;
	}

}

// Task to print the results
void TaskPrint(void *pvParameters) {
	struct_Data receivedValue;
	while (1) {
		int status = xQueueReceive(xQueue, &receivedValue, portMAX_DELAY);
		if (status == pdPASS) {
			if (receivedValue.ID == 1) {
				xil_printf("Temperature: %d \r\n", Temperature);
			} else if (receivedValue.ID == 0) {
				xil_printf(" Humidity: %d%%\r\n", Humidity);
			}
		}
	}
	taskYIELD();

}


void vButtonISR(void *CallBackRef) {
	BaseType_t pxHigherPriorityTaskWoken;
	u32 val = XGpio_DiscreteRead(&Gpio, 1);
	if ((MASK_BUTTON & val) == 0) {
		xTaskNotifyFromISR(hTaskDHT, 1, eSetValueWithOverwrite,
				&pxHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
	}
	// Clear the interrupt
	XGpio_InterruptClear(&Gpio, 1);
}

void TtcHandler(void *CallBackRef) {
	BaseType_t pxHigherPriorityTaskWoken;
	xil_printf("TtcHandler!\r\n ");
	xTaskNotifyFromISR(hTaskDHT, 0, eSetValueWithOverwrite,
			&pxHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);

}

int TTC_Initialization() {
	XInterval Interval;
	u8 Prescaler;
	//initialize the TTC
	XTtcPs_Config * ttc_config = XTtcPs_LookupConfig(DeviceId);
	int status = XTtcPs_CfgInitialize(&InstancePtr_TTC, ttc_config,
			ttc_config->BaseAddress);
	if (status != 0) {
		xil_printf("XTtcPs_SetOptions failed\r\n");
		return 1;
	}

	xPortInstallInterruptHandler(
	XPS_TTC0_1_INT_ID, (XInterruptHandler) XTtcPs_InterruptHandler,
			(void *) &InstancePtr_TTC);
	// Instead of XScuPsu_Enable

	//status = XScuGic_Connect(&IntcInstance, XPS_TTC0_0_INT_ID,
	//(Xil_InterruptHandler) XTtcPs_InterruptHandler, &InstancePtr_TTC);
	XTtcPs_SetStatusHandler(&InstancePtr_TTC, &InstancePtr_TTC,
			(XTtcPs_StatusHandler) TtcHandler);
	XScuGic_SetPriorityTriggerType(&xInterruptController, XPS_TTC0_1_INT_ID,
			0xA0, 0x3);

	status = XTtcPs_SetOptions(&InstancePtr_TTC, XTTCPS_OPTION_INTERVAL_MODE);
	if (status != 0) {
		xil_printf("XTtcPs_SetOptions failed\r\n");
		return 1;
	}

	// Calculate and set the timer interval and prescaler
	XTtcPs_CalcIntervalFromFreq(&InstancePtr_TTC, 1, &Interval, &Prescaler);
	XTtcPs_SetInterval(&InstancePtr_TTC, (Interval * 5));
	XTtcPs_SetPrescaler(&InstancePtr_TTC, Prescaler);

	XTtcPs_EnableInterrupts(&InstancePtr_TTC, XTTCPS_IXR_INTERVAL_MASK);

	vPortEnableInterrupt(XPS_TTC0_1_INT_ID);

	XTtcPs_Start(&InstancePtr_TTC);
	return 0;
}
