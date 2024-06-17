#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "xgpio.h"
#include "xparameters.h"
#include "xil_printf.h"

#define GPIO_DEVICE_ID XPAR_GPIO_0_DEVICE_ID
#define LED1 (0x01 << 20)
#define LED2 (0x01 << 21)


StaticTask_t xTaskBufferLED1;
StaticTask_t xTaskBufferLED2;
StaticTask_t xTaskBufferPrint;
StackType_t xStackLED1[configMINIMAL_STACK_SIZE];
StackType_t xStackLED2[configMINIMAL_STACK_SIZE];
StackType_t xStackPrint[configMINIMAL_STACK_SIZE];


TaskHandle_t myTaskLED1;
TaskHandle_t myTaskLED2;
TaskHandle_t myTaskPrint;


void TaskLED(void *pvParameters);
void TaskPrint(void *pvParameters);


typedef struct {
    int *counter;
    uint32_t pin;
    int delay;
} TaskParameters;

XGpio Gpio;
#define LED_CHANNEL 1

 int Task1Counter = 0;
 int Task2Counter = 0;
int numled = 0;
int numprint = 0;
int main(){

	int led1_2 = LED1 | LED2;

	//Initialize GPIO
	int Status = XGpio_Initialize(&Gpio, XPAR_GPIO_0_DEVICE_ID);
	if (Status != XST_SUCCESS) {
			xil_printf("Gpio Initialization Failed\r\n");
			return XST_FAILURE;
		}
	XGpio_SetDataDirection(&Gpio, LED_CHANNEL, ~led1_2);

	TaskParameters params1 = { &Task1Counter, LED1, 300 }; // Parameters for LED1
	TaskParameters params2 = { &Task2Counter, LED2, 500 }; // Parameters for LED2

	myTaskLED1 = xTaskCreateStatic( TaskLED,"LED1",configMINIMAL_STACK_SIZE , &params1 ,
	tskIDLE_PRIORITY + 1,xStackLED1,&xTaskBufferLED1);

	myTaskLED2 = xTaskCreateStatic( TaskLED,"LED2",configMINIMAL_STACK_SIZE , &params2 ,
	tskIDLE_PRIORITY + 1,xStackLED2,&xTaskBufferLED2);

	myTaskPrint = xTaskCreateStatic( TaskPrint,"Print",configMINIMAL_STACK_SIZE , NULL ,
	tskIDLE_PRIORITY + 1,xStackPrint,&xTaskBufferPrint);



    vTaskStartScheduler(); // Start the scheduler

    while(1); // Should never reach here
    return 0;

}

void TaskLED(void *pvParameters) {
    TaskParameters *params = (TaskParameters *) pvParameters;
    const TickType_t xDelay = pdMS_TO_TICKS(params->delay);
    while(1) {
        (*(params->counter))++;
        u32 val;
        			val = XGpio_DiscreteRead(&Gpio, LED_CHANNEL);
        			val ^= params->pin;
        			XGpio_DiscreteWrite(&Gpio, LED_CHANNEL, val);
        vTaskDelay(xDelay); // Delay for task-specific period
        numled++ ;
		if(numled==1){
        xil_printf("memory LED :%lu \n\r" ,uxTaskGetStackHighWaterMark( myTaskLED1 ));
		}

    }

}

void TaskPrint(void *pvParameters) {
    const TickType_t xDelay = pdMS_TO_TICKS(3000); // 3 seconds delay
    while(1) {
        xil_printf("LED1 Task Counter: %d, LED2 Task Counter: %d\r\n", Task1Counter, Task2Counter);
        vTaskDelay(xDelay); // Delay for 3 seconds
        numprint++ ;
		if(numprint==1){
        xil_printf("memory print :%lu \n\r" ,uxTaskGetStackHighWaterMark( myTaskPrint ));
		}

    }
}
