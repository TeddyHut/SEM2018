/*
 * freertos_task.h
 *
 * Created: 28/02/2018 11:30:51 AM
 *  Author: teddy
 */ 

#pragma once

#include <FreeRTOS.h>
#include <task.h>

namespace utility {
	class FreeRTOSTask {
	protected:
		//Returns true and calls freeRTOSTask_creationError if there is an error
		bool init(char const *const taskName, unsigned short const stackDepth, UBaseType_t const priority = 1);
		virtual void freeRTOSTask_creationError();
		virtual void task_main() = 0;

		TaskHandle_t m_task = NULL;
	private:
		static void taskFunction(void *const task_param);
	};
}