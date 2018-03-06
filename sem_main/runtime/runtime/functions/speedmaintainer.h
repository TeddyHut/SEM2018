/*
 * speedmaintainer.h
 *
 * Created: 1/03/2018 7:33:29 PM
 *  Author: teddy
 */ 

#pragma once

#include "../function.h"
#include "timeout.h"

namespace runtime {
	struct Action_BinarySpeedOutput : public nsfunction::Action {
		Action_BinarySpeedOutput();
		bool speedIncrease = false;
	};
	class BinarySpeed : public Function {
	public:
		//If state is increasing, the function will output a speed increase until the top value is reached.
		BinarySpeed(Priority const priority);
		void setSpeed(float const speed);
		void setUpperHysteresis(float const hysteresis);
		void setLowerHysteresis(float const hysteresis);
	private:
		enum class HysteresisState {
			Falling,
			Rising,
		} hysteresisState = HysteresisState::Rising;
		void update(void *const input) override;
		float speed = 0;
		float upperHysteresis = 0;
		float lowerHysteresis = 0;
	};

	struct Action_SpeedMatcherOutput : public nsfunction::Action {
		float out = 0.0f;
	};
	//Will attempt to match two values from the inputs floats using negative inverse tan. Typically used to match motor to wheel speed.
	class SpeedMatcher : public Function {
	public:
		SpeedMatcher(Priority const priority, inputindex::data_f::Float const controlIndex, inputindex::data_f::Float const dependentIndex, actionid::ActionID const outputID);
		//Sets the index in input of what the speed matcher should try to match to
		void setControlIndex(inputindex::data_f::Float const index);
		//Sets the index in input of what the speed matcher will use as the speed of the matched item
		void setDependentIndex(inputindex::data_f::Float const index);
		//Sets action ID of the output. IMPORTANT: Any action that uses SpeedMatcher must have same struct alignment.
		void setActionOutputID(actionid::ActionID const id);
	private:
		void update(void *const input) override;
		actionid::ActionID outputID;
		inputindex::data_f::Float controlIndex;
		inputindex::data_f::Float dependentIndex;
	};

	//Same as action_speedmatcherouptut
	struct Action_LinearDutyCycleLimiterInput : public nsfunction::Action {
		float out = 0.0f;
	};
	class LinearDutyCycleLimiter : public Function {
	public:
		LinearDutyCycleLimiter(Priority const priority, Output const &parentOutputs, actionid::ActionID const outputID);
		//Used to start the limiter, will limit duty cycle from 0 to 1 over course of duration.
		void start(float const duration);
	private:
		void update(void *const input) override;
		Output const &parentOutputs;
		actionid::ActionID outputID;
		float startupTime = 0;
		ValueMatch::Linear<float> f;
	};
	//Will output if speed has been in speed +- speed * errorFactor for more than duration amount of time
	class SpeedMatchChecker : public Function {
	public:
		SpeedMatchChecker(Priority const priority);
		//Sets the error factor
		void setErrorFactor(float const errorFactor);
		//The amount of time the speed match checker has to be correct for
		void setCorrectTime(float const time);
		//Sets the input index for the control (target trying to match to)
		void setControlIndex(inputindex::data_f::Float const index);
		//Sets the input index for the thing at is being matched
		void setDependentIndex(inputindex::data_f::Float const index);
	private:
		void update(void *const input) override;
		float errorFactor = 0;
		float correctTime = 0;
		inputindex::data_f::Float controlIndex;
		inputindex::data_f::Float dependentIndex;
		//The time of the last error, used to determine whether the speed has been correct for a certain time or not
		float time_lastError = 0;
	};

	//Maintains speed of vehicle using speed matching within speed, upper hysteresis and lower hysteresis 
	class SpeedMaintainer : public Function {
		SpeedMaintainer(Priority const priority);
		//Speed in meters per second
		void setSpeed(float const speed);
		//Upper hysteresis in meters per second. After starting speed increase, the vehicle must get to set speed + upperHysteresis before disabling again.
		void setUpperHysteresis(float const hysteresis);
		//Lower hysteresis in meters per second. After starting coasting, the vehicle must get to set speed - lowerHysteresis before starting again.
		void setLowerHysteresis(float const hysteresis);
		//Sets ramp-up limit time
		void setRampUpTime(float const time);
		//Speed match timeout
		void setSpeedMatchTimeout(float const timeout);
		//Sets speed match limit ramp-up-time
		void setSpeedMatchRampUpTime(float const time);
	private:
		static constexpr Priority Priority_BinarySpeed = 0;
		static constexpr Priority Priority_SpeedMatch_Falling = 0;
		static constexpr Priority Priority_SpeedMatch_Rising = 2;
		static constexpr Priority Priority_DutyCycleLimiter = 3;
		static constexpr Priority Priority_SpeedMatchChecker = 0;
		//Has a higher priority, but not technically needed since the action is only added when they engage
		static constexpr Priority Priority_SpeedMatchCheckerTimeout = 1;
		static constexpr Priority Priority_Stop = 1;
		void update(void *const input);
		enum class CycleState {
			
		};
		BinarySpeed *child_binarySpeed = nullptr;
		SpeedMatcher *child_speedMatcher = nullptr;
		LinearDutyCycleLimiter *child_linearDutyCycleLimiter = nullptr;
		SpeedMatchChecker *child_speedMatchChecker = nullptr;
		TimeoutFunction child_speedMatchCheckerTimeout = nullptr;
		float rampUpTime = 0;
		float speedMatchRampUpTime = 0;
		float speedMatchTimeout = config::runtime::speedMatchChecker_timeout;
	};
}
