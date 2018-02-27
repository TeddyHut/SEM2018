/*
 * feedbackmanager.h
 *
 * Created: 24/02/2018 5:34:48 PM
 *  Author: teddy
 */ 

#pragma once

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

//Used as a base class for 'feedback management', eg Buzzers and LEDs
template <typename T, typename Sequence_t = std::function<void (T &)>>
class FeedbackManager {
	public:
	void registerSequence(Sequence_t const &seq);
	void registerSequence(Sequence_t const &seq, SemaphoreHandle_t const complete);
	void init(T *const item, size_t const queueSize);
	QueueHandle_t queue;
	protected:
	struct SequenceMeta {
		Sequence_t sequence;
		SemaphoreHandle_t sem_complete = NULL;
	};
	virtual void cleanup();
	virtual void queueFull();
	static void taskFunction(void *const feedbackManager);

	void task_main();
	T *item;
	TaskHandle_t task;
};

template <typename T, typename Sequence_t /*= std::function<void (T &)>*/>
void FeedbackManager<T, Sequence_t>::init(T *const item, size_t const queueSize)
{
	this->item = item;
	queue = xQueueCreate(queueSize, sizeof(Sequence_t *));
	if(queue == NULL)
	debugbreak();
	if(xTaskCreate(taskFunction, config::feedbackmanager::taskName, config::feedbackmanager::taskStackDepth, this, config::feedbackmanager::taskPriority, &task) == pdFAIL)
	debugbreak();
}

template <typename T, typename Sequence_t>
void FeedbackManager<T, Sequence_t>::cleanup() {}

template <typename T, typename Sequence_t>
void FeedbackManager<T, Sequence_t>::queueFull() {}

template <typename T, typename Sequence_t /*= std::function<void (T &)>*/>
void FeedbackManager<T, Sequence_t>::taskFunction(void *const feedbackManager)
{
	static_cast<FeedbackManager *>(feedbackManager)->task_main();
}

template <typename T, typename Sequence_t>
void FeedbackManager<T, Sequence_t>::task_main()
{
	while(true) {
		SequenceMeta *seq;
		xQueueReceive(queue, &seq, portMAX_DELAY);
		(seq->sequence)(*item);
		if(seq->sem_complete != NULL)
		xSemaphoreGive(seq->sem_complete);
		seq->~SequenceMeta();
		vPortFree(seq);
	}
}

template <typename T, typename Sequence_t>
void FeedbackManager<T, Sequence_t>::registerSequence(Sequence_t const &seq)
{
	registerSequence(seq, NULL);
}

template <typename T, typename Sequence_t /*= std::function<void (T &)>*/>
void FeedbackManager<T, Sequence_t>::registerSequence(Sequence_t const &seq, SemaphoreHandle_t const complete)
{
	SequenceMeta *seqptr = new (pvPortMalloc(sizeof(SequenceMeta))) SequenceMeta{seq, complete};
	if(xQueueSendToBack(queue, &seqptr, portMAX_DELAY) == errQUEUE_FULL) {
		seqptr->~SequenceMeta();
		vPortFree(seqptr);
		queueFull();
	}
}
