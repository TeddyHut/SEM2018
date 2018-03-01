/*
 * motor.h
 *
 * Created: 19/01/2018 5:59:54 PM
 *  Author: teddy
 */ 

#pragma once

#include <tc.h>

#include "../utility/timer.h"
#include "../feedback/valueitem.h"

class Motor : public ValueItem<float> {
public:
	virtual void setDutyCycle(float const dutyCycle) = 0;
private:
	void valueItem_setValue(float const value) override;
};

class TCMotor : public Motor {
public:
	using Conf = timer::StaticConfig<float, uint16_t, 0xffff>::Result; 

	void setDutyCycle(float const dutyCycle) override;

	TCMotor(Conf const conf, tc_compare_capture_channel const channel);
protected:
	Conf const conf;
	tc_module instance;
	tc_compare_capture_channel const channel;
};

class TCMotor0 : public TCMotor {
public:
	TCMotor0();
};

class TCMotor1 : public TCMotor {
public:
	TCMotor1();
};
