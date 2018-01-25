/*
 * run.h
 *
 * Created: 25/01/2018 1:47:44 PM
 *  Author: teddy
 */ 

#pragma once

#include <FreeRTOS.h>
#include <timers.h>
#include <bitset>
#include <string>
#include <memory>
#include "config.h"
#include "bms.h"
#include "valuematch.h"
#include "util.h"

namespace Run {
	struct Input {
		//Time in seconds since start of the operation
		double time;
		//BMS Data
		BMS::Data bms0data;
		BMS::Data bms1data;
		//Duty cycle (0-1) of motor0
		float motor0DutyCycle;
		//Duty cycle (0-1) of motor1
		float motor1DutyCycle;
		//Position (-180-180) of servo0
		float servo0Position;
		//Position (-180-180) of servo1
		float servo1Position;
		//Speed of motor0 (teeth per second)
		float motor0Speed;
		//Speed of motor1 (teeth per second)
		float motor1Speed;
		//Speed of drive shaft (teeth per second)
		float driveSpeed;
		//Speed of vehicle (m/s)
		float vehicleSpeed;
		//Current going through motor0 (A)
		float motor0Current;
		//Current going through motor1 (A)
		float motor1Current;
		//Total energy usage of motor0 (W/h)
		float motor0EnergyUsage;
		//Total energy usage of motor1 (W/h)
		float motor1EnergyUsage;
		//Total energy usage overall (W/h)
		float totalEnergyUsage;
		//The total distance covered
		float distance;
		//The state of the operator presence button
		bool opState;
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

	class DisplayLine {
	public:
		virtual std::string get_text(Input const &input) = 0;
		virtual ~DisplayLine() = default;
	};

	//Used in display line to switch between statistics
	class TimerUpdate {
	public:
		TimerUpdate(TickType_t const period = config::run::displaycycleperiod);
		virtual ~TimerUpdate();
	protected:
		TimerHandle_t timer;
		virtual void cycle() = 0;
	private:
		static void callback(TimerHandle_t timer);
	};

	//Displays the startup prompt for top line
	class DL_Idle_Top : public DisplayLine {
	public:
		std::string get_text(Input const &input);
	};

	//Displays battery statistics
	class DL_Battery : public DisplayLine, public TimerUpdate {
	public:
		std::string get_text(Input const &input);
	protected:
		void cycle() override;
		enum class Cycle {
			Voltage = 0,
			Current,
			AvgCellVoltage,
			_size,
		} curcycle;
	};

	//Displays current vehicle speed and total energy usage. Switches to total time. During normal circumstances always on the top.
	class DL_SpeedEnergy : public DisplayLine, public TimerUpdate {
	public:
		std::string get_text(Input const &input);
	protected:
		void cycle() override;
		enum class Cycle {
			SpeedEnergy = 0,
			Time,
			_size,
		} curcycle;
	};

	//Alternates between ramping statistics (motor current usages, duty cycle, ramping speed)
	class DL_Ramping : public DisplayLine, public TimerUpdate {
	public:
		std::string get_text(Input const &input);
	protected:
		void cycle() override;
		enum class Cycle {
			MotorCurrent = 0,
			DutyCycle,
			Rampspeed,
			_size,
		} curcycle;
	};

	//Alternates between coasting statistics (distance traveled, time coasting, battery voltages)
	class DL_Coasting : public DisplayLine, public TimerUpdate {
	public:
		std::string get_text(Input const &input);
	protected:
		void cycle() override;
		enum class Cycle {
			Distance,
			CoastTime,
			BatteryVoltage,
			_size,
		} curcycle;
		//Used to workout coasting time
		bool firstGetText = true;
		float starttime;
	};

	struct Display {
		//The current displayline used for the top line
		std::unique_ptr<DisplayLine, decltype(deleter_free)> topline;
		//Display line used for the bottom line
		std::unique_ptr<DisplayLine, decltype(deleter_free)> bottomline;
	};

	class Task {
	public:
		virtual Output update(Input const &input) = 0;
		virtual Task* complete(Input const &input) = 0;
		virtual void displayUpdate(Display &disp);
		virtual ~Task() = default;
	};

	//Expected normal run task order:
	//Idle -> Engage -> Startup -> Disengage -> Coast -> SpeedMatch -> Engage -> CoastRamp -> Disengage -> Coast ...

	//Desc: Waits until the OP button is pressed, and then engages motors
	//NextTast: Engage(input, Both, Startup)
	class Idle : public Task {
	public:
		Output update(Input const &input) override;
		Task *complete(Input const &input) override;
		void displayUpdate(Display &disp) override;
	};
	
	//Desc: Engages either or both of the servos
	//NextTask:
	//	if post == Startup -> Startup
	//	if post == CoastRamp -> CoastRamp
	class Engage : public Task {
	public:
		enum class PostTask {
			Startup,
			CoastRamp,
		};
		enum class Servo {
			Servo0,
			Servo1,
			Both,
		};
		Output update(Input const &input) override;
		Task *complete(Input const &input) override;
		Engage(Input const &input, Servo const servo, PostTask const post);
	private:
		ValueMatch::Linear<float, double> f;
		Servo servo;
		PostTask post;
	};

	//Desc: Disengages the servos over time config::run::servodisengagetime
	//NextTask: Coast
	class Disengage : public Task {
	public:
		enum class Servo {
			Servo0,
			Servo1,
			Both,
		};
		Output update(Input const &input) override;
		Task *complete(Input const &input) override;
		Disengage(Input const &input, Servo const servo);
	private:
		ValueMatch::Linear<float, double> f;
		Servo servo;
	};

	//Desc: Brings motor duty cycle upto 100% over time of config::run::rampuptime. Stops motors when input.driveSpeed is greater than config::run::maxspeed
	//NextTask: Disengage(Both)
	class Startup : public Task {
	public:
		Output update(Input const &input) override;
		Task *complete(Input const &input) override;
		void displayUpdate(Display &disp) override;
		Startup(Input const &input);
	private:
		ValueMatch::Linear<float, double> f;
	};

	//Desc: Waits until speed is less than or equal to config::minspeed
	//NextTask: SpeedMatch
	class Coast : public Task {
	public:
		Output update(Input const &input) override;
		Task *complete(Input const &input) override;
		void displayUpdate(Display &disp) override;
	};

	//Desc: Ramps up dutyCycle of motor0 until the maximum speed is reached
	//NextTask: Disengage(Servo0)
	class CoastRamp : public Task {
	public:
		Output update(Input const &input) override;
		Task *complete(Input const &input) override;
		void displayUpdate(Display &disp) override;
		CoastRamp(Input const &input);
	private:
		ValueMatch::Linear<float, double> f;
	};

	//Matches the speed of motor0 to the speed of the drive shaft (from 0 to 1 over time config::matchramptime), within config::errorFactor. Then sets to motor duty cycle to zero
	//NextTask: Engage(Servo0, CoastRamp)
	class SpeedMatch : public Task {
	public:
		Output update(Input const &input) override;
		Task *complete(Input const &input) override;
		SpeedMatch(Input const &value);
	private:
		ValueMatch::Linear<float, double> f;
	};

	//---Non funcitonality tasks---
	class OPCheck : public Task {
	public:
		Output update(Input const &input) override;
		Task *complete(Input const &input) override;
	private:
		bool previousOPState = false;
	};
	
	//Checks battery voltage and alerts driver if battery voltage is getting low
	class BatteryCheck : public Task {
		Output update(Input const &input) override;
		Task *complete(Input const &input) override;
	};

	//Checks that motor current usage isn't too high
	class MotorCheck : public Task {
		Output update(Input const &input) override;
		Task *complete(Input const &input) override;
	};
}
