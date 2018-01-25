/*
 * spimanager.h
 *
 * Created: 23/01/2018 6:24:10 PM
 *  Author: teddy
 */ 

#pragma once

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <spi.h>
#include <vector>


class SPIManager {
public:
	//An SPI job
	struct Job {
		void *inbuffer = nullptr;
		void *outbuffer = nullptr;
		size_t len = 0;
		uint8_t ss_pin = 0;
		SemaphoreHandle_t sem_complete = NULL;
	};

	void addJob(Job const &job);
	SPIManager();
protected:
	SemaphoreHandle_t sem_startup;
	spi_module spi_instance;
private:
	static void taskFunction(void *spimanager);
	void task_main();
	TaskHandle_t task;
	QueueHandle_t que_pendingJobs;
};

class SPIManager0 : public SPIManager {
public:
	SPIManager0();
};
