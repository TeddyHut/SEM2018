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
#include <queue.h>
#include "config.h"

auto deleter_free  = [](auto p){
	using p_t = std::remove_pointer_t<decltype(p)>;
	p->~p_t();
	std::free(p);
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
	FeedbackManager(T &item, size_t const queueSize = 4);
protected:
	virtual void cleanup();
	virtual void queueFull();
	static void taskFunction(void *const feedbackManager);

	void task_main();
	T &item;
	TaskHandle_t task;
	QueueHandle_t queue;
};

template <typename T, typename Sequence_t>
void FeedbackManager<T, Sequence_t>::cleanup() {}

template <typename T, typename Sequence_t>
void FeedbackManager<T, Sequence_t>::queueFull() {}

template <typename T, typename Sequence_t /*= std::function<void (T &)>*/>
void FeedbackManager<T, Sequence_t>::taskFunction(void *const feedbackManager)
{
	static_cast<FeedbackManager *>(feedbackManager)->task_main();
}

template <typename T, typename Sequence_t /*= std::function<void (T &)>*/>
FeedbackManager<T, Sequence_t>::FeedbackManager(T &item, size_t const queueSize /*= 4*/) : item(item)
{
	queue = xQueueCreate(queueSize, sizeof(Sequence_t *));
	if(queue == NULL) {
		debugbreak();
	}
	if(xTaskCreate(taskFunction, config::feedbackmanager::taskName, config::feedbackmanager::taskStackDepth, this, config::feedbackmanager::taskPriority, &task) == pdFAIL) {
		debugbreak();
	}
}

template <typename T, typename Sequence_t>
void FeedbackManager<T, Sequence_t>::task_main()
{
	while(true) {
		Sequence_t *seq;
		xQueueReceive(queue, &seq, portMAX_DELAY);
		(*seq)(item);
		seq->~Sequence_t();
		std::free(seq);
	}
}

template <typename T, typename Sequence_t>
void FeedbackManager<T, Sequence_t>::registerSequence(Sequence_t const &seq)
{
	//Manual memory management on modern C++ :(. Have to do this because otherwise... IDK. Too tired. Might actually work without this. Whatever.
	void *buffer = std::malloc(sizeof(Sequence_t));
	Sequence_t *seqptr = new (buffer)(Sequence_t) (seq);
	if(xQueueSendToBack(queue, &seqptr, 0) == errQUEUE_FULL)
		queueFull();
}
