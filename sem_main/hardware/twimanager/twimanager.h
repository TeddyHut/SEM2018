/*
 * twimanager.h
 *
 * Created: 10/02/2018 9:00:12 AM
 *  Author: teddy
 */ 

//Should make this and SPIManager inherit from a common base-class... thing is they were just different enough not to

#pragma once

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

#include <i2c_master.h>
#include <i2c_master_interrupt.h>

#include "../../utility/freertos_task.h"

class TWIManager : public utility::FreeRTOSTask {
public:
	struct Job {
		enum class Mode {
			Read,
			Write,
			//Makes use of repeated start	
			WriteRead,
		} mode = Mode::Read;
		void *inbuffer = nullptr;
		void *outbuffer = nullptr;
		size_t readlen = 0;
		size_t writelen = 0;
		//Address of slave
		uint8_t address = 0;
		SemaphoreHandle_t sem_complete = NULL;
	};

	void addJob(Job const &job, TickType_t const waitTime = 0);
	void init();
protected:
	i2c_master_module twi_instance;
	void task_main() override;
	QueueHandle_t que_pendingJobs;
};

class TWIManager0 : public TWIManager {
public:
	static void twi_callback(i2c_master_module *const module);
	static TaskHandle_t callbackTask;
	void init();
};
