/*
 * adcmanager.h
 *
 * Created: 1/03/2018 8:48:44 AM
 *  Author: teddy
 */ 

#pragma once

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <adc.h>
#include <adc_callback.h>

#include "../../utility/freertos_task.h"

class ADCManager : public utility::FreeRTOSTask {
public:
	struct Job {
		adc_positive_input input = ADC_POSITIVE_INPUT_PIN0;
		SemaphoreHandle_t sem_complete = NULL;
		uint16_t *output = nullptr;
	};
	void addJob(Job const &job);
	void init();
protected:
	void task_main() override;
	adc_module adc_instance;
	QueueHandle_t que_pendingJobs;
	static TaskHandle_t task;
	static void interrupt_callback(adc_module *const module);
};
