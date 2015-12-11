#ifndef SIM_H
#define SIM_H

#include <malloc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define printf_P(...)	printf(__VA_ARGS__)
#define puts_P(s)	puts(s)
#define fputs_P(s, o)	fputs(s, o)
#define PSTR(s)		s
#define pdPASS		1
#define pdTRUE		1
#define pdFALSE		0
#define portMAX_DELAY	__INT_MAX__

#define tskPROT_PRIORITY	0

#define ESC_GREY
#define ESC_RED
#define ESC_GREEN
#define ESC_YELLOW
#define ESC_BLUE
#define ESC_MAGENTA
#define ESC_CYAN
#define ESC_WHITE

typedef int QueueHandle_t;
typedef int SemaphoreHandle_t;

void vPortFree(void *ptr);
void *pvPortMalloc(int size);

void vTaskDelay(int time);
void xQueueReset(QueueHandle_t queue);
int xQueueReceive(QueueHandle_t queue, void *ptr, int time);
int xSemaphoreTake(SemaphoreHandle_t sempr, int time);
void xSemaphoreGive(SemaphoreHandle_t sempr);
int uxQueueSpacesAvailable(QueueHandle_t queue);
int xQueueSendToBack(QueueHandle_t queue, void *ptr, int time);
QueueHandle_t xQueueCreate(int depth, int size);
int xSemaphoreCreateMutex();
int xTaskCreate(void (*func)(void *), char *name, int stack, void *param, int priority, void *handle);

void taskStart();
void taskRun(void (*func)(void *), char *name, void *param);
void taskStop();

#ifdef __cplusplus
}
#endif

#endif // SEMPHR_H
