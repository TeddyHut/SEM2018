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

namespace Run {
	enum class TaskIdentity {
		None = 0,
		Idle,
		Startup,
		Engage,
		Disengage,
		Coast,
		CoastRamp,
		SpeedMatch,
		OPCheck,
		BatteryCheck,
		MotorCheck,
		ADPControl,
		_size,
	};

	struct Input {
		//The currently running task
		TaskIdentity currentID = TaskIdentity::None;
		//Time in seconds since start of the operation
		float time = 0;
		//BMS Data
		BMS::Data bms0data;
		BMS::Data bms1data;
		//Total voltage of both batteries
		float voltage = 0;
		//Duty cycle (0-1) of motor0
		float motor0DutyCycle = 0;
		//Duty cycle (0-1) of motor1
		float motor1DutyCycle = 0;
		//Position (-180-180) of servo0
		float servo0Position = config::run::servo0restposition;
		//Position (-180-180) of servo1
		float servo1Position = config::run::servo1restposition;
		//Speed of motor0 (teeth per second)
		float motor0Speed = 0;
		//Speed of motor1 (teeth per second)
		float motor1Speed = 0;
		//Speed of drive shaft (teeth per second)
		float driveSpeed = 0;
		//Speed of vehicle (m/s)
		float vehicleSpeed = 0;
		//Current going through motor0 (A)
		float motor0Current = 0;
		//Current going through motor1 (A)
		float motor1Current = 0;
		//Total energy usage of motor0 (W/h)
		float motor0EnergyUsage = 0;
		//Total energy usage of motor1 (W/h)
		float motor1EnergyUsage = 0;
		//Total energy usage overall (W/h)
		float totalEnergyUsage = 0;
		//The total distance covered
		float distance = 0;
		//The state of the operator presence button
		bool opState = false;
		//TO ADD:
		//Accelerometer data, information about I2C voltage/current sensors
	};
	
	struct Output {
		struct Element {
			enum {
				Motor0DutyCycle = 0,
				Motor1DutyCycle,
				Servo0Position,
				Servo1Position,
				_size,
			};
		};
		float motor0DutyCycle;
		float motor1DutyCycle;
		float servo0Position;
		float servo1Position;
		std::bitset<Element::_size> output;
	};
}
