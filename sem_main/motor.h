/*
 * motor.h
 *
 * Created: 19/01/2018 5:59:54 PM
 *  Author: teddy
 */ 

#pragma once

#include <tc.h>
#include "servo.h"
#include "util.h"

constexpr tc_clock_prescaler tccPrescaler_to_tcPrescaler(tcc_clock_prescaler const prescaleValue) {
	tc_clock_prescaler rtrn = TC_CLOCK_PRESCALER_DIV1;
	bool succeeded = false;
	std::underlying_type_t<tcc_clock_prescaler> values[8][2] = {
		{TCC_CLOCK_PRESCALER_DIV1   , static_cast<std::underlying_type_t<tcc_clock_prescaler>>(TC_CLOCK_PRESCALER_DIV1   )},
		{TCC_CLOCK_PRESCALER_DIV2   , static_cast<std::underlying_type_t<tcc_clock_prescaler>>(TC_CLOCK_PRESCALER_DIV2   )},
		{TCC_CLOCK_PRESCALER_DIV4   , static_cast<std::underlying_type_t<tcc_clock_prescaler>>(TC_CLOCK_PRESCALER_DIV4   )},
		{TCC_CLOCK_PRESCALER_DIV8   , static_cast<std::underlying_type_t<tcc_clock_prescaler>>(TC_CLOCK_PRESCALER_DIV8   )},
		{TCC_CLOCK_PRESCALER_DIV16  , static_cast<std::underlying_type_t<tcc_clock_prescaler>>(TC_CLOCK_PRESCALER_DIV16  )},
		{TCC_CLOCK_PRESCALER_DIV64  , static_cast<std::underlying_type_t<tcc_clock_prescaler>>(TC_CLOCK_PRESCALER_DIV64  )},
		{TCC_CLOCK_PRESCALER_DIV256 , static_cast<std::underlying_type_t<tcc_clock_prescaler>>(TC_CLOCK_PRESCALER_DIV256 )},
		{TCC_CLOCK_PRESCALER_DIV1024, static_cast<std::underlying_type_t<tcc_clock_prescaler>>(TC_CLOCK_PRESCALER_DIV1024)}};
	for(uint8_t i = 0; i < 8 * 2; i++) {
		if(values[i / 2][0] == prescaleValue) {
			rtrn = static_cast<tc_clock_prescaler>(values[i / 2][1]);
			succeeded = true;
			break;
		}
	}
	throwIfTrue(!succeeded, "could not find matching TC prescaler value");
	return rtrn;
}

class Motor {
public:
	virtual void setDutyCycle(float const dutyCycle) = 0;
};

class TCMotor : public Motor {
	using Conf = TCCServoStaticConfig<float, uint16_t, 0xffff>::Result; 
public:
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
