/*
 * run.h
 *
 * Created: 25/01/2018 1:47:44 PM
 *  Author: teddy
 */ 

#pragma once

//TODO: This all needs desperate refactoring

#include <bitset>
#include <memory>
#include <FreeRTOS.h>
#include "main_config.h"
#include "bms.h"
#include "valuematch.h"
#include "util.h"
#include "runcommon.h"
#include "rundisplay.h"

namespace program {
	void init();

	class Task {
	public:
		TaskIdentity id = TaskIdentity::None;
		virtual Output update(Input const &input) = 0;
		virtual Task* complete(Input const &input) = 0;
		virtual void displayUpdate(Display &disp);
		Task(TaskIdentity const id = TaskIdentity::None);
		virtual ~Task() = default;
	};

	//Desc: Waits until the OP button is pressed, and then starts testing
	//NextTast:
	class Idle : public Task {
	public:
		Idle(bool const countdown = true);
		Output update(Input const &input) override;
		Task *complete(Input const &input) override;
		void displayUpdate(Display &disp) override;
	private:
		TickType_t lastCount;
		bool previousOPState = true;
		bool doCountdown;
		int countdown = -1;
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
		Output update(Input const &input) override;
		Task *complete(Input const &input) override;
		void displayUpdate(Display &disp) override;
		CoastRamp(Input const &input);
	private:
		ValueMatch::Linear<float, float> f;
	};

	class Finished : public Task {
	public:
		Finished(float const time, float const energy, float const distance);
		Output update(Input const &input) override;
		Task *complete(Input const &input) override;
		void displayUpdate(Display &disp) override;
	private:
		float time = 0;
		float energy = 0;
		float distance = 0;
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
		bool keepBeeping = true;
		float errorTime = 0;
		bool finished = false;
		
	};
}
