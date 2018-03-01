/*
 * timer.h
 *
 * Created: 28/02/2018 10:52:53 AM
 *  Author: teddy
 */ 

#pragma once

#include <type_traits>

#include <tcc.h>
#include <tc.h>

namespace timer {
	//Used to find values to be used for times at compile time using constexpr. Originally designed for servos, therefore this is built to accomodate them
	template <typename T, typename counter_t = uint32_t, counter_t t_max = 0x00ffffff>
	struct StaticConfig {
		//! Struct holding the results of the static calculations
		struct Result {
			//! The number of ticks of the prescaled clock per period
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
			//throwIfTrue(!(period > midpoint), "period must be greater than midpoint");
			//throwIfTrue(!(period > derivation), "period must be greater than derivation");
			//throwIfTrue(!(minAngle <= maxAngle), "minAngle must be less than or equal to maxAngle");

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
			for(size_t i = 0; i < (8 * 2); i++) {
				if((clockTicksPerPeriod / prescaleValues[i / 2][1]) <= t_max) {
					prescaleQuotient = prescaleValues[i / 2][1];
					prescaleSetting = static_cast<tcc_clock_prescaler>(prescaleValues[i / 2][0]);
					break;
				}
			}
			//throwIfTrue(prescaleQuotient == 0, "cannot find prescale value high enough");
			size_t const prescaledClockFrequency = clockFrequency / prescaleQuotient;
			counter_t const prescaledTicksPerPeriod = clockTicksPerPeriod / prescaleQuotient;
			counter_t const counter_midpoint = midpoint * prescaledClockFrequency;
			counter_t const counter_derivation = derivation * prescaledClockFrequency;
			return Result{prescaledTicksPerPeriod, prescaleSetting, counter_midpoint, counter_derivation, minAngle, maxAngle};
		}
	};

	//Used to convert from a tcc prescaler to a tc prescaler.
	constexpr tc_clock_prescaler tccPrescaler_to_tcPrescaler(tcc_clock_prescaler const prescaleValue) {
		tc_clock_prescaler rtrn = TC_CLOCK_PRESCALER_DIV1;
		//bool succeeded = false;
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
				//succeeded = true;
				break;
			}
		}
		//throwIfTrue(!succeeded, "could not find matching TC prescaler value");
		return rtrn;
	}
}
