/*
 * manualserial.h
 *
 * Created: 26/01/2018 1:12:53 PM
 *  Author: teddy
 */ 

#pragma once

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <usart.h>

class ManualSerial {
public:
	void init();
private:
	static void callback(usart_module *const module);
	static void taskFunction(void *manualserial);
	void task_main();
	static TaskHandle_t task;
	usart_module instance;
};
