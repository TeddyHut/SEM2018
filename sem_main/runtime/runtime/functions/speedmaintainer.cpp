/*
 * speedmaintainer.cpp
 *
 * Created: 4/03/2018 2:30:18 PM
 *  Author: teddy
 */ 

#include <algorithm>

#include "../runtime.h"

#include "speedmaintainer.h"

runtime::Action_BinarySpeedOutput::Action_BinarySpeedOutput() : Action(actionid::BinarySpeed_Output) {}

runtime::BinarySpeed::BinarySpeed(Priority const priority) : Function(priority) {}

void runtime::BinarySpeed::setSpeed(float const speed)
{
	this->speed = speed;
}

void runtime::BinarySpeed::setUpperHysteresis(float const hysteresis)
{
	this->upperHysteresis = hysteresis;
}

void runtime::BinarySpeed::setLowerHysteresis(float const hysteresis)
{
	this->lowerHysteresis = hysteresis;
}

void runtime::BinarySpeed::update(void *const input)
{
	auto const in = static_cast<runtime::Input *>(input);
	Action_BinarySpeedOutput act;
	if(hysteresisState == HysteresisState::Rising) {
		//Increase the speed while the vehicle is lower than speed + upperHysteresis
		act.speedIncrease = in->data_float[inputindex::data_f::Speed_Wheel_ms] < speed + upperHysteresis;
		//If the vehicle has reached the speed.
		if(!act.speedIncrease)
			hysteresisState = HysteresisState::Falling;
			addAction(act);

	}
	else {
		//Increase speed if the vehicle gets lower than speed - lowerHysteresis
		act.speedIncrease = in->data_float[inputindex::data_f::Speed_Wheel_ms] < speed - lowerHysteresis;
		//If the vehicle has started speeding up.
		if(act.speedIncrease) {
			hysteresisState = HysteresisState::Rising;
			addAction(act);
		}
	}
}

runtime::SpeedMatcher::SpeedMatcher(Priority const priority, inputindex::data_f::Float const controlIndex, inputindex::data_f::Float const dependentIndex, actionid::ActionID const outputID) :
    Function(priority), controlIndex(controlIndex), dependentIndex(dependentIndex), outputID(outputID) {}

void runtime::SpeedMatcher::setControlIndex(inputindex::data_f::Float const index)
{
	this->controlIndex = index;
}

void runtime::SpeedMatcher::setDependentIndex(inputindex::data_f::Float const index)
{
	this->dependentIndex = index;
}

void runtime::SpeedMatcher::setActionOutputID(actionid::ActionID const id)
{
	this->outputID = id;
}

void runtime::SpeedMatcher::update(void *const input)
{
	auto const in = static_cast<runtime::Input *>(input);
	Action_SpeedMatcherOutput act;
	act.id = outputID;
	act.out = std::max(static_cast<float>(-(2.0f / M_PI) * std::atan((in->data_float[dependentIndex] in->data_float[controlIndex]) / 500.0f)), 0.0f);
	addAction(act);
}

runtime::LinearDutyCycleLimiter::LinearDutyCycleLimiter(Priority const priority, Output const &parentOutputs, actionid::ActionID const outputID)
    : Function(priority), parentOutputs(parentOutputs), outputID(outputID) {}

void runtime::LinearDutyCycleLimiter::start(float const duration)
{
	//Will be processed during update
	startupTime = duration;
}

void runtime::LinearDutyCycleLimiter::update(void *const input)
{
	auto const in = static_cast<runtime::Input *>(input);
	float const &time = in->data_float[inputindex::data_f::RTCTime];
	if(startupTime != 0.0f)
		f.startup(0.0f, 1.0f, time, time + startupTime);
	Action_LinearDutyCycleLimiterInput const *const oldAct = static_cast<Action_LinearDutyCycleLimiterInput const *>(findAction(outputID));
	if(oldAct != nullptr) {
		//If the present action is too high, it needs to be limited
		if(oldAct->out > f.currentValue(time)) {
			Action_LinearDutyCycleLimiterInput newAct;
			newAct.id = oldAct->id;
			newAct.out = f.currentValue(time);
			addAction(newAct);
			//This action will then be pulled out during function_update in the parent, with a priority equal to this function.
		}
	}
}

void runtime::SpeedMatchChecker::setErrorFactor(float const errorFactor)
{
	this->errorFactor = errorFactor;
}

void runtime::SpeedMatchChecker::setCorrectTime(float const time)
{
	this->correctTime = time;
}

void runtime::SpeedMatchChecker::setControlIndex(inputindex::data_f::Float const index)
{
	this->controlIndex = index;
}

void runtime::SpeedMatchChecker::setDependentIndex(inputindex::data_f::Float const index)
{
	this->dependentIndex = index;
}

void runtime::SpeedMatchChecker::update(void *const input)
{
	auto const in = static_cast<runtime::Input *>(input);
	float const &time = in->data_float[inputindex::data_f::RTCTime];
	float const &speed_control = in->data_float[controlIndex];
	float const &speed_dependent = in->data_float[dependentIndex];
	//If the speeds are not close enough, set this to be the time of last error
	if(!((speed_dependent <= speed_control + (speed_control * errorFactor)) && (speed_dependent >= speed_control - (speed_control * errorFactor)))) {
		time_lastError = time;
	}
	//If the speeds have been matching for correctTime, then we are ready for servo engagement
	if(time - time_lastError >= correctTime) {
		Action act;
		act.id = actionid::SpeedMatchChecker_Ready;
		addAction(act);
	}
}


void runtime::SpeedMaintainer::setSpeed(float const speed)
{
	child_binarySpeed->setSpeed(speed);
}


void runtime::SpeedMaintainer::setUpperHysteresis(float const hysteresis)
{
	child_binarySpeed->setUpperHysteresis(hysteresis);
}

void runtime::SpeedMaintainer::setLowerHysteresis(float const hysteresis)
{
	child_binarySpeed->setLowerHysteresis(hysteresis);
}

void runtime::SpeedMaintainer::setRampUpTime(float const time)
{
	rampUpTime = time;
}

void runtime::SpeedMaintainer::setSpeedMatchTimeout(float const timeout)
{
	this->speedMatchTimeout = timeout;
}

void runtime::SpeedMaintainer::setSpeedMatchRampUpTime(float const time)
{
	this->speedMatchRampUpTime = time;
}

void runtime::SpeedMaintainer::update(void *const input)
{
	auto const in = static_cast<runtime::Input *>(input);
	//Add all the generic stop actions, so that by default things are just off
	
	//Get binary speed output, if present. Action is only added if there is a change.
	Action_BinarySpeedOutput *binarySpeedOutput = findAction(actionid::BinarySpeed_Output);
	//If a change needs to happen
	if(binarySpeedOutput != nullptr) {
		//If the motors should start speedmatching to bring up speed.
		if(binarySpeedOutput->speedIncrease) {
			//Start the duty cycle limiter so that motor speed doesn't suddenly "jump up".
			child_linearDutyCycleLimiter->start(rampUpTime);
			//Start the speed match timeout
			child_speedMatchCheckerTimeout->startTimeout(speedMatchChecker_timeout);
			//Increase the priority of the speed matcher so that it starts making an effect
			child_speedMatcher->setPriority(Priority_SpeedMatch_Rising);
		}
		//Motors should stop and servos should disengage
		else {
			//
		}
	}
	Action *speedMatchCheckerOutput = findAction(actionid::SpeedMatchChecker_Ready);
	if(speedMatchCheckerOutput != nullptr) {
		//Ready to engage

	}
}
