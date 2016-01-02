#include "simulation.h"
#include "simulator.h"
#include <unistd.h>
#include <QThreadPool>
#include <QVector>
#include <QQueue>
#include <QMutex>
#include <QMap>

struct queue_t {
	int size, depth;
	QMutex *mutex;
	QQueue<void *> *queue;
};

QVector<queue_t> vecQueue;
QMutex mutexQueue;
QVector<QMutex *> vecMutex;
QMutex mutexMutex;

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
	QThread::currentThread()->msleep(time);
	//usleep(time * 1000);
}

void xQueueReset(QueueHandle_t queue)
{
	mutexQueue.lock();
	queue_t q = vecQueue[queue];
	mutexQueue.unlock();
	if (q.size == 0)
		return;
	q.mutex->lock();
	while (!q.queue->isEmpty())
		free(q.queue->dequeue());
	q.mutex->unlock();
}

int xQueueReceive(QueueHandle_t queue, void *ptr, int time)
{
	mutexQueue.lock();
	queue_t q = vecQueue[queue];
	mutexQueue.unlock();
	if (q.size == 0)
		return pdFALSE;
	while (q.queue->isEmpty() && time-- != 0)
		usleep(1000);
	QMutexLocker locker(q.mutex);
	if (q.queue->isEmpty())
		return pdFALSE;
	void *p = q.queue->dequeue();
	memcpy(ptr, p, q.size);
	free(p);
	return pdTRUE;
}

int uxQueueSpacesAvailable(QueueHandle_t queue)
{
	mutexQueue.lock();
	queue_t q = vecQueue[queue];
	mutexQueue.unlock();
	if (q.size == 0)
		return 0;
	return q.queue->count();
}

int xQueueSendToBack(QueueHandle_t queue, void *ptr, int time)
{
	mutexQueue.lock();
	queue_t q = vecQueue[queue];
	mutexQueue.unlock();
	if (q.size == 0)
		return pdFALSE;
	void *p = malloc(q.size);
	memcpy(p, ptr, q.size);
	q.mutex->lock();
	q.queue->enqueue(p);
	q.mutex->unlock();
	return true;
}

QueueHandle_t xQueueCreate(int depth, int size)
{
	queue_t q;
	q.size = size;
	q.depth = depth;
	q.queue = new QQueue<void *>;
	q.mutex = new QMutex;
	mutexQueue.lock();
	vecQueue.append(q);
	mutexQueue.unlock();
	return vecQueue.count() - 1;
}

int xSemaphoreTake(SemaphoreHandle_t sempr, int time)
{
	mutexMutex.lock();
	QMutex *mutex = vecMutex[sempr];
	mutexMutex.unlock();
	if (mutex == 0)
		return pdFALSE;
	return mutex->tryLock(time);
}

void xSemaphoreGive(SemaphoreHandle_t sempr)
{
	mutexMutex.lock();
	QMutex *mutex = vecMutex[sempr];
	mutexMutex.unlock();
	if (mutex == 0)
		return;
	return mutex->unlock();
}

int xSemaphoreCreateMutex()
{
	QMutex *mutex = new QMutex;
	mutexMutex.lock();
	vecMutex.append(mutex);
	mutexMutex.unlock();
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

void vPortFree(void *ptr)
{
	sim->memFree(ptr);
}

void *pvPortMalloc(int size)
{
	return sim->memAlloc(size);
}
