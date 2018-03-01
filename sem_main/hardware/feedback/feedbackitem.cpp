/*
 * feedbackitem.cpp
 *
 * Created: 28/02/2018 12:50:54 PM
 *  Author: teddy
 */ 

#include <FreeRTOS.h>
#include <task.h>

#include "feedbackitem.h"

using namespace feedbackitem;

feedbackitem::action::Action::Action(Type const type /*= Type::None*/) : type(type) {}

void feedbackitem::action::Action::start() {}

feedbackitem::action::Wait::Wait(TickType_t const duration) : Action(Type::Wait), duration(duration) {}

void feedbackitem::action::Wait::start()
{
	//Saving memory
	duration = xTaskGetTickCount() + duration;
}

bool feedbackitem::action::Wait::update()
{
	return xTaskGetTickCount() >= duration;
}

void FeedbackItem::wait(TickType_t const ticks)
{
	action::Wait act;
	act.duration = ticks;
	addAction(act);
}

void FeedbackItem::update()
{
	if(!actions.empty()) {
		if(currentAction != actions[0].get()) {
			currentAction = actions[0].get();
			actions[0]->start();
		}
		if(actions[0]->update()) {
			actions.erase(actions.begin());
			if(actions.empty())
				currentAction = nullptr;
		}
	}
}
