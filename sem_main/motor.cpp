/*
 * motor.cpp
 *
 * Created: 19/01/2018 6:35:32 PM
 *  Author: teddy
 */ 

#include "main_config.h"
#include "motor.h"
#include <type_traits>

void TCMotor::setDutyCycle(float const dutyCycle)
{
	this->dutyCycle = dutyCycle;
	//Set to duty cycle * period compare
	uint16_t compareValue = dutyCycle * instance.hw->COUNT16.CC[0].bit.CC;
	//Prevent a glitch where it drops down for a tiny bit at 100%
	if(compareValue >= instance.hw->COUNT16.CC[0].bit.CC) compareValue++;
	instance.hw->COUNT16.CC[1].bit.CC = compareValue;
	//instance.hw->COUNT16.CC[1].bit.CC = instance.hw->COUNT16.CC[0].bit.CC + 1;
}

void TCMotor::setPeriod(float const period)
{
	this->period = period;
	auto const &timerConfig = TimerTemplate::calculateResult(config::motor::clockFrequency, period, 0, 0, 0, 0);
	instance.hw->COUNT16.CTRLA.bit.PRESCALER = tccPrescaler_to_tcPrescaler(timerConfig.prescaleSetting);
	instance.hw->COUNT16.CC[0].bit.CC = timerConfig.prescaledTicksPerPeriod;
	setDutyCycle(dutyCycle);
}

TCMotor0::TCMotor0()
{
	
	tc_config config;
	tc_get_config_defaults(&config);
	config.clock_source = GCLK_GENERATOR_3;
	config.count_direction = TC_COUNT_DIRECTION_UP;
	config.counter_size = TC_COUNTER_SIZE_16BIT;
	config.enable_capture_on_channel[0] = false;
	config.enable_capture_on_channel[1] = false;
	config.oneshot = false;
	//Using channel 1 so that 0 can be the top
	config.pwm_channel[1].enabled = true;
	config.pwm_channel[1].pin_mux = MUX_PA23E_TC4_WO1;
	config.pwm_channel[1].pin_out = PIN_PA23E_TC4_WO1;
	config.run_in_standby = true;
	config.wave_generation = TC_WAVE_GENERATION_MATCH_PWM;
	config.waveform_invert_output = 0b0;

	tc_init(&instance, TC4, &config);
	setPeriod(1.0f / 1000.0f); //1000Hz
	setDutyCycle(0.0f);
	//Do this to invert waveform
	instance.hw->COUNT16.CTRLC.bit.INVEN1 = true;
	tc_enable(&instance);
}
