/*
 * timeout.cpp
 *
 * Created: 6/03/2018 11:55:31 PM
 *  Author: teddy
 */ 

#include "../runtime.h"
#include "timeout.h"

runtime::TimeoutFunction::TimeoutFunction(Priority const priority, actionid::ActionID const outputID)
    : Function(priority), outputID(outputID) {}

void runtime::TimeoutFunction::startTimeout(float const timeout)
{
	//Indicate to the update function that a new timeout period has started
	this->startTime = -1;
	this->timeout = timeout;
}

void runtime::TimeoutFunction::setOutputID(actionid::ActionID const outputID)
{
	this->outputID = outputID;
}

void runtime::TimeoutFunction::update(void *const input)
{
	auto const in = static_cast<runtime::Input *>(input);
	float const &time = in->data_float[inputindex::data_f::RTCTime];
	if(startTime < 0)
	startTime = time;
	if(time >= startTime + timeout) {
		Action act;
		act.id = outputID;
		addAction(act);
	}
}
