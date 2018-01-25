/*
 * buzzer.h
 *
 * Created: 20/01/2018 2:58:15 PM
 *  Author: teddy
 */ 

#pragma once

#include <functional>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <tc.h>
#include <type_traits>
#include "util.h"

//Abstract representation of physical buzzer
class Buzzer {
public:
	virtual void setState(bool const state) = 0;
	//Same as calling setState(true)
	void start();
	//Same as calling setState(false)
	void stop();
};

//Manages running the buzzers tones
class BuzzerManager : public FeedbackManager<Buzzer> {
public:
	BuzzerManager(Buzzer &buzzer);
protected:
	void cleanup() override;
};

class Buzzer0 : public Buzzer {
public:
	void setState(bool const state) override;

	Buzzer0();
private:
	tc_module instance;
};
