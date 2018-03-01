/*
 * adcmanager.cpp
 *
 * Created: 1/03/2018 8:49:01 AM
 *  Author: teddy
 */ 

#include "../../main_config.h"

#include "adcmanager.h"

void ADCManager::addJob(Job const &job)
{
	if(xQueueSendToBack(que_pendingJobs, &job, 0) == errQUEUE_FULL) {
		//Error
	}
}

void ADCManager::init()
{
	adc_config adcconfig;
	adc_get_config_defaults(&adcconfig);
	adcconfig.clock_source = GCLK_GENERATOR_1;
	adcconfig.clock_prescaler = ADC_CLOCK_PRESCALER_DIV32;
	adcconfig.reference = ADC_REFERENCE_INTVCC1;
	adcconfig.resolution = ADC_RESOLUTION_12BIT;
	adcconfig.gain_factor = ADC_GAIN_FACTOR_1X;
	adcconfig.negative_input = ADC_NEGATIVE_INPUT_GND;
	adcconfig.accumulate_samples = ADC_ACCUMULATE_SAMPLES_4;
	adcconfig.divide_result = ADC_DIVIDE_RESULT_4;
	adcconfig.left_adjust = false;
	adcconfig.differential_mode = false;
	adcconfig.freerunning = false;
	adcconfig.reference_compensation_enable = false;
	adcconfig.sample_length = 16;
	adc_init(&adc_instance, ADC, &adcconfig);

	adc_register_callback(&adc_instance, interrupt_callback, ADC_CALLBACK_READ_BUFFER);
	adc_enable_callback(&adc_instance, ADC_CALLBACK_READ_BUFFER);

	adc_enable(&adc_instance);

	if((que_pendingJobs = xQueueCreate(config::adcmanager::pendingQueueSize, sizeof(Job))) == NULL) {
		//Error
	}
	FreeRTOSTask::init(config::adcmanager::taskName, config::adcmanager::taskStackDepth, config::adcmanager::taskPriority);
}

void ADCManager::task_main()
{
	uint16_t buffer[2];
	while(true) {
		Job job;
		xQueueReceive(que_pendingJobs, &job, portMAX_DELAY);
		adc_set_positive_input(&adc_instance, job.input);
		adc_read_buffer_job(&adc_instance, buffer, 2);
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		//Get the second so that the ADC can have balanced after input chance
		*job.output = buffer[1];
		if(job.sem_complete != NULL)
			xSemaphoreGive(job.sem_complete);
	}
}

TaskHandle_t ADCManager::task = NULL;

void ADCManager::interrupt_callback(adc_module *const module)
{
	BaseType_t prio = 0;
	vTaskNotifyGiveFromISR(task, &prio);
	portYIELD_FROM_ISR(prio);
}
