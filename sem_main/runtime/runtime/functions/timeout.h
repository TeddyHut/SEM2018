/*
 * timeout.h
 *
 * Created: 6/03/2018 11:55:15 PM
 *  Author: teddy
 */ 

#pragma once

#include "../function.h"

namespace runtime {

//Will timeout after a certain amount of time and add a generic action with outputID
class TimeoutFunction : public Function {
public:
	TimeoutFunction(Priority const priority, actionid::ActionID const outputID);
	//Starts a timeout
	void startTimeout(float const timeout);
	//Sets the outputID
	void setOutputID(actionid::ActionID const outputID);
private:
	void update(void *const input) override;
	//The time at which the timeout was started (a value less than 0 indicates a new timeout period has started)
	float startTime = 0;
	//The timeout period
	float timeout = 0;
	//The output action ID
	actionid::ActionID outputID;
};

}
