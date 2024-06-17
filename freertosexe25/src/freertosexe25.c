#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "xil_printf.h"
#include <stdio.h>
#include <stdint.h>
#include "xuartps.h"
#include "FreeRTOS_CLI.h"
#include "dht.h"
#include "XGpio.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "queue.h"

#define GPIO_DEVICE_ID XPAR_GPIO_0_DEVICE_ID
#define LED1 (0x01 << 20)
#define LED2 (0x01 << 21)
#define DHT_PIN (0x01 << 27)
int Temperature,Humidity=0;
Dht * dhtSensor;

#define CHANNEL 1
#define DeviceId 0
XUartPs InstancePtrUART;
QueueHandle_t xQueue;
XGpio Gpio;


char  BufferPtr[100] ;
char  BufferPtr2[100] ;
char  BufferPtrCLI[100] ;

int Status;


TaskStatus_t pxTaskStatusArray [10];

// Generic task function for printing

void TaskUart();
void TaskCLI();
BaseType_t dht_get(char * writeBuffer, size_t bufferLen, const char *pcCommandString);
BaseType_t list(char * writeBuffer, size_t bufferLen, const char *pcCommandString);
BaseType_t stat(char * writeBuffer, size_t bufferLen, const char *pcCommandString);
BaseType_t led(char * writeBuffer, size_t bufferLen, const char *pcCommandString);



static const CLI_Command_Definition_t listCommand =
{
"list",
"print list of task names one per line\r\n",
list,
0 // expects exactly one parameter
};
static const CLI_Command_Definition_t statCommand =
{
"stat",
"print run-time statistics\r\n",
stat,
0 // expects exactly one parameter
};
static const CLI_Command_Definition_t ledCommand =
{
"led",
"o led action 0 – off, 1 – on\r\n",
led,
1 // expects exactly one parameter
};

static const CLI_Command_Definition_t dht_getCommand =
{
"dht_get",
" print temperature and humidity\r\n",
dht_get,
0 // expects exactly one parameter
};




int main(void) {
	//Initialize GPIO
	int Status = XGpio_Initialize(&Gpio, XPAR_GPIO_0_DEVICE_ID);
	if (Status != XST_SUCCESS) {
			xil_printf("Gpio Initialization Failed\r\n");
			return XST_FAILURE;
		}
	XGpio_SetDataDirection(&Gpio, CHANNEL, ~LED1);

    dhtSensor = Dht_init(&Gpio, DHT_PIN);

	xQueue = xQueueCreate( 5, 100*sizeof( char ) );

    XUartPs_Config  *Config =  XUartPs_LookupConfig(DeviceId);

    Status = XUartPs_CfgInitialize( &InstancePtrUART, Config, Config->BaseAddress);



    FreeRTOS_CLIRegisterCommand(&listCommand);
    FreeRTOS_CLIRegisterCommand(&statCommand);
    FreeRTOS_CLIRegisterCommand(&ledCommand);
    FreeRTOS_CLIRegisterCommand(&dht_getCommand);

    xTaskCreate(TaskCLI, "TaskCLI", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(TaskUart, "TaskUart", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);



        vTaskStartScheduler();

    while (1);
    return 0;
}



void TaskCLI() {

		while (1) {

			Status = xQueueReceive(xQueue, BufferPtr2, portMAX_DELAY);

			FreeRTOS_CLIProcessCommand(BufferPtr2, BufferPtrCLI,100);
			xil_printf("%s\n\r",BufferPtrCLI);


		}

}


void TaskUart(){
	char ch;
	u32 NumBytes = 1 ;
	int count =0 ;
	while (1) {

		Status = XUartPs_Recv(&InstancePtrUART, (u8*)&ch, NumBytes);
		if (Status != 0) {
			Status = XUartPs_Send(&InstancePtrUART, (u8*)&ch, NumBytes);
			BufferPtr[count] = ch;
			count++;
			if ((BufferPtr[count - 1] == '\n')
					|| (BufferPtr[count - 1] == '\r')) {
				xil_printf("\r\n");
				BufferPtr[count - 1] = '\0';
				count = 0;

					Status = xQueueSendToBack(xQueue, BufferPtr, 0);
					if (Status != pdPASS) {
						xil_printf("Queue is FULL.\r\n");
				}
				taskYIELD();
			}
		}

	}

}

BaseType_t dht_get(char * writeBuffer, size_t bufferLen, const char *pcCommandString) {

	Dht_takeData(dhtSensor);
	      //  Dht_getResult(dhtSensor, &globalTemperature, &globalHumidity);
	Dht_getResult(dhtSensor, &Temperature, &Humidity);
	sprintf (writeBuffer ,"Temperature - %d , Humidity - %d ",Temperature ,Humidity);

return pdFALSE;
}

BaseType_t list(char * writeBuffer, size_t bufferLen, const char *pcCommandString) {
	vTaskList(writeBuffer);
return pdFALSE;
}

BaseType_t stat(char * writeBuffer, size_t bufferLen, const char *pcCommandString) {
	vTaskGetRunTimeStats(writeBuffer);
return pdFALSE;
}

BaseType_t led(char * writeBuffer, size_t bufferLen, const char *pcCommandString) {
	BaseType_t parameterStringLength;

	char * parameter = FreeRTOS_CLIGetParameter(  pcCommandString, 1, &parameterStringLength);
	if (*parameter == '1'){
		XGpio_DiscreteSet(&Gpio,CHANNEL, LED1);
	}
	if (*parameter == '0'){
		XGpio_DiscreteClear(&Gpio,CHANNEL, LED1);
	}
	writeBuffer[0]='\0';
return pdFALSE;
}

