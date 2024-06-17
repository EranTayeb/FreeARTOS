#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "xil_printf.h"
#include <stdio.h>
#include <stdint.h>

SemaphoreHandle_t printMutex;

// Generic task function for printing
void printTask(void *pvParameters) {
    char *message = (char *)pvParameters;
    TickType_t delay;
    while (1) {
        delay = pdMS_TO_TICKS(10 + (rand() % 90)); // Random delay between 10 and 99 ms
        if (xSemaphoreTake(printMutex, portMAX_DELAY) == pdTRUE) {
                        xil_printf("%s", message);
                        xSemaphoreGive(printMutex);
        vTaskDelay(delay);
    }
}
}

int main(void) {

	printMutex = xSemaphoreCreateMutex();

      char* string1="Task 1 ************************************************************\r\n";
      char* string2="Task 2 ------------------------------------------------------------\r\n";
	  char* string3= "Task 3 ////////////////////////////////////////////////////////////\r\n";

    if (printMutex != NULL) {
        xTaskCreate(printTask, "PrintTask3", configMINIMAL_STACK_SIZE, string3, tskIDLE_PRIORITY + 3, NULL);
        xTaskCreate(printTask, "PrintTask2", configMINIMAL_STACK_SIZE, string2, tskIDLE_PRIORITY + 2, NULL);
        xTaskCreate(printTask, "PrintTask1", configMINIMAL_STACK_SIZE, string1, tskIDLE_PRIORITY + 1, NULL);

        vTaskStartScheduler();
    } else {
    	xil_printf("Failed to create mutex\r\n");
    }

    while (1);
    return 0;
}
