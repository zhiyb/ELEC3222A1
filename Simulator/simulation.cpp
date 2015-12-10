#include "simulation.h"
#include <unistd.h>
#include <QThreadPool>
#include <QVector>
#include <QQueue>
#include <QMutex>
#include <QMap>

struct queue_t {
	int size, depth;
	QQueue<void *> *queue;
};

QVector<queue_t> vecQueue;
QVector<QMutex *> vecMutex;

class Task: public QRunnable
{
public:
	Task(void (*func)(void *), void *param, QString name):
		QRunnable(), func(func), param(param), name(name) {}

	void run() {func(param);}

private:
	void (*func)(void *);
	void *param;
	QString name;
};

QVector<Task *> vecTask;

void vTaskDelay(int time)
{
	usleep(time * 1000);
}

void xQueueReset(QueueHandle_t queue)
{
	queue_t q = vecQueue[queue];
	if (q.size == 0)
		return;
	while (!q.queue->isEmpty()) {
		void *ptr = q.queue->dequeue();
		free(ptr);
	}
}

int xQueueReceive(QueueHandle_t queue, void *ptr, int time)
{
	queue_t q = vecQueue[queue];
	if (q.size == 0)
		return pdFALSE;
	while (q.queue->isEmpty() && time-- != 0)
		usleep(1000);
	if (q.queue->isEmpty())
		return pdFALSE;
	void *p = q.queue->dequeue();
	memcpy(ptr, p, q.size);
	free(p);
	return pdTRUE;
}

int xSemaphoreTake(SemaphoreHandle_t sempr, int time)
{
	QMutex *mutex = vecMutex[sempr];
	if (mutex == 0)
		return pdFALSE;
	return mutex->tryLock(time);
}

void xSemaphoreGive(SemaphoreHandle_t sempr)
{
	QMutex *mutex = vecMutex[sempr];
	if (mutex == 0)
		return;
	return mutex->unlock();
}

int uxQueueSpacesAvailable(QueueHandle_t queue)
{
	queue_t q = vecQueue[queue];
	if (q.size == 0)
		return 0;
	return q.queue->count();
}

int xQueueSendToBack(QueueHandle_t queue, void *ptr, int time)
{
	queue_t q = vecQueue[queue];
	if (q.size == 0)
		return pdFALSE;
	void *p = malloc(q.size);
	memcpy(p, ptr, q.size);
	q.queue->enqueue(p);
	return true;
}

QueueHandle_t xQueueCreate(int depth, int size)
{
	queue_t q;
	q.size = size;
	q.depth = depth;
	q.queue = new QQueue<void *>;
	vecQueue.append(q);
	return vecQueue.count() - 1;
}

int xSemaphoreCreateMutex()
{
	QMutex *mutex = new QMutex;
	vecMutex.append(mutex);
	return vecMutex.count() - 1;
}

int xTaskCreate(void (*func)(void *), char *name, int stack, void *param, int priority, void *handle)
{
	Task *task = new Task(func, param, name);
	task->setAutoDelete(true);
	vecTask.append(task);
	return pdPASS;
}

void taskStart()
{
	for (int i = 0; i < vecTask.size(); i++)
		QThreadPool::globalInstance()->start(vecTask.at(i));
}

void taskStop()
{
}

void taskRun(void (*func)(void *), char *name, void *param)
{
	Task *task = new Task(func, param, name);
	task->setAutoDelete(true);
	QThreadPool::globalInstance()->start(task);
}
