/*
 * servo.h
 *
 * Created: 18/01/2018 4:06:35 PM
 *  Author: teddy
 */ 

#pragma once

#include <tcc.h>
#include "util.h"

/*!
 * \brief Abstract servo type
 * \tparam position_t The type used to set the position of the servo
 */
template <typename position_t = float>
class Servo {
public:
	virtual void setPosition(position_t const position) = 0;
};

using ServoF = Servo<float>;

//! Static configuration values for a timed servo
template <typename T, typename counter_t = uint32_t, counter_t t_max = 0x00ffffff>
struct TCCServoStaticConfig {
	//! Struct holding the results of the static calculations
	struct Result {
		//! The number of ticks of the prescaled clock per servo period
		counter_t prescaledTicksPerPeriod;
		//! The prescale setting to use
		tcc_clock_prescaler prescaleSetting;
		//! The counter value midpoint
		counter_t counter_midpoint;
		//! The counter value derivation
		counter_t counter_derivation;
		//! The minimum angle (in degrees or whatever the user wants)
		T minAngle;
		//! The maxiumum angle
		T maxAngle;
	};

	static constexpr Result calculateResult(T const clockFrequency, T const period, T const midpoint, T const derivation, T const minAngle, T const maxAngle) {
		throwIfTrue(!(period > midpoint), "period must be greater than midpoint");
		throwIfTrue(!(period > derivation), "period must be greater than derivation");
		throwIfTrue(!(minAngle <= maxAngle), "minAngle must be less than or equal to maxAngle");

		unsigned int prescaleQuotient = 0;
		tcc_clock_prescaler prescaleSetting = TCC_CLOCK_PRESCALER_DIV1;

		size_t const clockTicksPerPeriod = clockFrequency * period;
		//Probably should have just used two arrays...
		unsigned int prescaleValues[8][2] = {
			{static_cast<unsigned int>(TCC_CLOCK_PRESCALER_DIV1), 1},
			{static_cast<unsigned int>(TCC_CLOCK_PRESCALER_DIV2), 2},
			{static_cast<unsigned int>(TCC_CLOCK_PRESCALER_DIV4), 4},
			{static_cast<unsigned int>(TCC_CLOCK_PRESCALER_DIV8), 8},
			{static_cast<unsigned int>(TCC_CLOCK_PRESCALER_DIV16), 16},
			{static_cast<unsigned int>(TCC_CLOCK_PRESCALER_DIV64), 64},
			{static_cast<unsigned int>(TCC_CLOCK_PRESCALER_DIV256), 256},
			{static_cast<unsigned int>(TCC_CLOCK_PRESCALER_DIV1024), 1024}};
		for(size_t i = 0; i < 8 * 2; i++) {
			if((clockTicksPerPeriod / prescaleValues[i / 2][1]) <= t_max) {
				prescaleQuotient = prescaleValues[i / 2][1];
				prescaleSetting = static_cast<tcc_clock_prescaler>(prescaleValues[i / 2][0]);
				break;
			}
		}
		throwIfTrue(prescaleQuotient == 0, "cannot find prescale value high enough");
		size_t const prescaledClockFrequency = clockFrequency / prescaleQuotient;
		counter_t const prescaledTicksPerPeriod = clockTicksPerPeriod / prescaleQuotient;
		counter_t const counter_midpoint = midpoint * prescaledClockFrequency;
		counter_t const counter_derivation = derivation * prescaledClockFrequency;
		return Result{prescaledTicksPerPeriod, prescaleSetting, counter_midpoint, counter_derivation, minAngle, maxAngle};
	}
};

template <typename, typename>
class TCCServo {
};

template <typename position_t, typename config_t>
class TCCServo<position_t, TCCServoStaticConfig<config_t>> : public Servo<position_t> {
public:
	using Result = typename TCCServoStaticConfig<config_t>::Result;
	//! \copydoc Servo::setPosition
	void setPosition(position_t const position) override;

	TCCServo(Result const staticConfig, tcc_match_capture_channel const channel);
protected:
	Result const staticConfig;
	tcc_module instance;
	tcc_match_capture_channel const channel;
};

template <typename position_t, typename config_t>
void TCCServo<position_t, TCCServoStaticConfig<config_t>>::setPosition(position_t const position)
{
	//Determine angle range
	config_t angleRange = staticConfig.maxAngle - staticConfig.minAngle;
	//Determine the middle angle
	config_t midAngle = staticConfig.minAngle + (angleRange / 2);

	tcc_set_compare_value(&instance, channel, staticConfig.counter_midpoint + (staticConfig.counter_derivation * ((position - midAngle) / (angleRange / 2))));
}

template <typename position_t, typename config_t>
TCCServo<position_t, TCCServoStaticConfig<config_t>>::TCCServo(Result const staticConfig, tcc_match_capture_channel const channel) : staticConfig(staticConfig), channel(channel)
{
}

class TCCServo0 : public TCCServo<float, TCCServoStaticConfig<float>> {
	using Parent = TCCServo<float, TCCServoStaticConfig<float>>;
	using StaticConfig = TCCServoStaticConfig<float>;
	using Result = typename StaticConfig::Result;
public:
	TCCServo0();
};

class TCCServo1 : public TCCServo<float, TCCServoStaticConfig<float>> {
	using Parent = TCCServo<float, TCCServoStaticConfig<float>>;
	using StaticConfig = TCCServoStaticConfig<float>;
	using Result = typename StaticConfig::Result;
public:
	TCCServo1();
};
