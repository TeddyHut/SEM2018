/*
 * run.h
 *
 * Created: 25/01/2018 1:47:44 PM
 *  Author: teddy
 */ 

#pragma once

#include <FreeRTOS.h>
#include <semphr.h>
#include <timers.h>
#include <bitset>
#include <memory>
#include "config.h"
#include "bms.h"
#include "valuematch.h"
#include "util.h"

namespace Run {
	struct Input {
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

	class DisplayLine {
	public:
		virtual void get_text(char str[], Input const &input) = 0;
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
		void get_text(char str[], Input const &input);
	};

	//Displays battery statistics
	class DL_Battery : public DisplayLine, public TimerUpdate {
	public:
		void get_text(char str[], Input const &input);
	protected:
		void cycle() override;
		enum class Cycle {
			Voltage = 0,
			Current,
			AvgCellVoltage,
			_size,
		} curcycle = Cycle::Voltage;
	};

	//Displays current vehicle speed and total energy usage. Switches to total time. During normal circumstances always on the top.
	class DL_SpeedEnergy : public DisplayLine, public TimerUpdate {
	public:
		void get_text(char str[], Input const &input);
	protected:
		void cycle() override;
		enum class Cycle {
			SpeedEnergy = 0,
			Time,
			_size,
		} curcycle = Cycle::SpeedEnergy;
	};

	//Alternates between ramping statistics (motor current usages, duty cycle, ramping speed)
	class DL_Ramping : public DisplayLine, public TimerUpdate {
	public:
		void get_text(char str[], Input const &input);
	protected:
		void cycle() override;
		enum class Cycle {
			MotorCurrent = 0,
			DutyCycle,
			Rampspeed,
			_size,
		} curcycle = Cycle::MotorCurrent;
	};

	//Alternates between coasting statistics (distance traveled, time coasting, battery voltages)
	class DL_Coasting : public DisplayLine, public TimerUpdate {
	public:
		void get_text(char str[], Input const &input);
	protected:
		void cycle() override;
		enum class Cycle {
			Distance,
			CoastTime,
			BatteryVoltage,
			_size,
		} curcycle = Cycle::Distance;
		//Used to workout coasting time
		bool firstGetText = true;
		float starttime;
	};

	struct Display {
		//The current displayline used for the top line
		std::unique_ptr<DisplayLine, deleter_free<DisplayLine>> topline;
		//Display line used for the bottom line
		std::unique_ptr<DisplayLine, deleter_free<DisplayLine>> bottomline;
		
		void printDisplay(Input const &input) const;
	};

	class Task {
	public:
		enum class Identity {
			None,
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
		} identity = Identity::None;
		virtual Output update(Input const &input) = 0;
		virtual Task* complete(Input const &input) = 0;
		virtual void displayUpdate(Display &disp);
		Task(Identity const id = Identity::None);
		virtual ~Task() = default;
	};
	//Desc: Waits until the OP button is pressed, and then engages motors
	//NextTast: Engage(input, Both, Startup)
	class Idle : public Task {
	public:
		Idle();
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
		ValueMatch::Linear<float, float> f0;
		ValueMatch::Linear<float, float> f1;
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
		ValueMatch::Linear<float, float> f0;
		ValueMatch::Linear<float, float> f1;
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
		ValueMatch::Linear<float, float> f;
	};

	//Desc: Waits until speed is less than or equal to config::minspeed
	//NextTask: SpeedMatch
	class Coast : public Task {
	public:
		Coast();
		Output update(Input const &input) override;
		Task *complete(Input const &input) override;
		void displayUpdate(Display &disp) override;
	};

	//Desc: Ramps up dutyCycle of motor0 until the maximum speed is reached
	//NextTask: Disengage(Servo0)
	class CoastRamp : public Task {
	public:
		CoastRamp();
		Output update(Input const &input) override;
		Task *complete(Input const &input) override;
		void displayUpdate(Display &disp) override;
		CoastRamp(Input const &input);
	private:
		ValueMatch::Linear<float, float> f;
	};

	//Matches the speed of motor0 to the speed of the drive shaft (from 0 to 1 over time config::matchramptime), within config::errorFactor. Then sets to motor duty cycle to zero
	//NextTask: Engage(Servo0, CoastRamp)
	class SpeedMatch : public Task {
	public:
		Output update(Input const &input) override;
		Task *complete(Input const &input) override;
		SpeedMatch(Input const &input);
	private:
		ValueMatch::Linear<float, float> f;
	};

	//---Non funcitonality tasks---
	class OPCheck : public Task {
	public:
		Output update(Input const &input) override;
		Task *complete(Input const &input) override;
		OPCheck();
	private:
		SemaphoreHandle_t sem_buzzerComplete;
		bool previousOPState = false;
		bool buzzerInQueue = false;
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
