/*
 * runcommon.h
 *
 * Created: 2/02/2018 12:13:56 AM
 *  Author: teddy
 */

#pragma once

#include <bitset>
#include "util.h"
#include "bms.h"

namespace program {
	enum class TaskIdentity {
		None = 0,
		Idle,
		Startup,
		Coast,
		CoastRamp,
		Finished,
		OPCheck,
		_size,
	};

	struct Input {
		enum class State {
			Idle,
			Running,
			Finished,
		} programstate = State::Idle;
		//The currently running task
		TaskIdentity currentID = TaskIdentity::None;
		//Time in seconds since start of the operation
		float time = 0;
		//BMS Data
		BMS::Data bms0data;
		BMS::Data bms1data;
		//Total voltage of both batteries
		float voltage = 0;
		//The current to use for calculations
		float calculationCurrent = 0;
		//The frequency of the motor
		float motorPWMFrequency = 0;
		//Duty cycle (0-1) of motor
		float motorDutyCycle = 0;
		//Speed of motor (rotations per second)
		float motorRPS = 0;
		//The number of 'ticks' (encoder interrupts) the motor has had
		unsigned int motorTicks = 0;
		//Current going through motor (A)
		float motorCurrent = 0;
		//Total energy usage of motor (W/h)
		float motorEnergyUsage = 0;
		//The wheel speed (rotations per second)
		float wheelRPS = 0;
		//The number of wheel interrupts
		unsigned int wheelTicks = 0;
		//The vehicle speed (in meters per second)
		float vehicleSpeedMs = 0;
		//Total energy usage overall (W/h)
		float totalEnergyUsage = 0;
		//The total distance covered
		float distance = 0;
		//The state of the operator presence button
		bool opState = false;
		//Whether to log data on this iteration
		bool logCycle = false;
		//The number of log samples taken
		unsigned int samples = 0;
		//Distance covered since starting
		float startDistance = 0;
		//Time since starting
		float startTime = 0;
		//TO ADD:
		//Accelerometer data, information about I2C voltage/current sensors
	};
	
	struct Output {
		struct Element {
			enum {
				MotorDutyCycle = 0,
				MotorPWMFrequency,
				State,
				_size,
			};
		};
		float motorDutyCycle;
		float motorPWMFrequency;
		Input::State programstate;
		std::bitset<Element::_size> output;
	};
}
