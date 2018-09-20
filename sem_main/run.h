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

namespace Run {
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

	class Pull : public Task {
	public:
		Pull();
		Output update(Input const &input) override;
		Task *complete(Input const &input) override;
		void displayUpdate(Display &disp) override;
	};

	//Should REALLY put these two in a common base class... but it doesn't really matter in this instance
	//Tests duty cycle effect on speed
	class DutyCycle : public Task {
	public:
		DutyCycle();
		Output update(Input const &input) override;
		Task *complete(Input const &input) override;
		void displayUpdate(Display &disp) override;
	private:
		bool initialised = false;
		ValueMatch::Linear<float, float> f1;
		int stage = 0;
		float lastStageTime = 0;
	};

	class Frequency : public Task {
	public:
		Frequency();
		Output update(Input const &input) override;
		Task *complete(Input const &input) override;
		void displayUpdate(Display &disp) override;
	private:
		bool initialised = false;
		ValueMatch::Linear<float, float> f1;
		int stage = 0;
		float lastStageTime = 0;
	};

	class Delay : public Task {
	public:
		Delay();
		Output update(Input const &input) override;
		Task *complete(Input const &input) override;
		void displayUpdate(Display &disp) override;
	private:
		bool initialised = false;
		float startTime;
	};
}
