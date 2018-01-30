/*
 * util.h
 *
 * Created: 19/01/2018 6:53:44 PM
 *  Author: teddy
 */ 

#pragma once

//#include <exception>
//#include <stdexcept>
#include <functional>
#include <memory>
#include <type_traits>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include "config.h"

extern "C" {
//int rpl_vsnprintf(char *, size_t, const char *, va_list);
int rpl_snprintf(char *, size_t, const char *, ...) __attribute__((format(printf, 3, 4)));
//int rpl_vasprintf(char **, const char *, va_list);
//int rpl_asprintf(char **, const char *, ...);
}

template <typename T>
class deleter_free {
public:
	void operator() (T *const p) const {
		p->~T();
		vPortFree(p);
	}
};

constexpr void throwIfTrue(bool const p, char const *const str) {
	//p ? throw std::logic_error(str) : 0;
}

namespace adcutil {
	constexpr float vdiv_factor(float const r1, float const r2) {
		return r2 / (r1 + r2);
	}

	constexpr float vdiv_input(float const output, float const factor) {
		return output / factor;
	}

	template <typename adc_t, adc_t max>
	constexpr float adc_voltage(adc_t const value, float const vref) {
		return vref * (static_cast<float>(value) / static_cast<float>(max));
	}

	template <typename adc_t, adc_t max>
	constexpr adc_t adc_value(float const voltage, float const vref) {
		return (voltage / vref) * static_cast<float>(max);
	}
}

void debugbreak();

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
