/*
 * freertos_task.cpp
 *
 * Created: 28/02/2018 12:25:51 PM
 *  Author: teddy
 */ 

#include "freertos_task.h"

bool utility::FreeRTOSTask::init(char const *const taskName, unsigned short const stackDepth, UBaseType_t const priority /*= 1*/)
{
	if(xTaskCreate(taskFunction, taskName, stackDepth, this, priority, &m_task) != pdPASS) {
		freeRTOSTask_creationError();
		return true;
	}
	return false;
}

void utility::FreeRTOSTask::freeRTOSTask_creationError()
{
	//Error
}

void utility::FreeRTOSTask::taskFunction(void *const task_param)
{
	static_cast<FreeRTOSTask *>(task_param)->task_main();
}
