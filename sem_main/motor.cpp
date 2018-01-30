/*
 * motor.cpp
 *
 * Created: 19/01/2018 6:35:32 PM
 *  Author: teddy
 */ 

#include "config.h"
#include "motor.h"
#include <type_traits>

namespace {
	constexpr TCCServoStaticConfig<float, uint16_t, 0xffff>::Result res = TCCServoStaticConfig<float, uint16_t, 0xffff>::calculateResult(
		config::motor::clockFrequency, config::motor::period, 0, 0, 0, 0);
}

void TCMotor::setDutyCycle(float const dutyCycle)
{
	tc_set_compare_value(&instance, channel, 0xffff * dutyCycle);
}

TCMotor::TCMotor(Conf const conf, tc_compare_capture_channel const channel) : conf(conf), channel(channel)
{
}

TCMotor0::TCMotor0() : TCMotor(res, TC_COMPARE_CAPTURE_CHANNEL_0)
{
	//Using PWM channel 1 because channel 0 is used as top in 16 bit mode
	tc_config config;
	tc_get_config_defaults(&config);
	config.clock_prescaler = tccPrescaler_to_tcPrescaler(res.prescaleSetting);
	config.clock_source = GCLK_GENERATOR_3;
	config.count_direction = TC_COUNT_DIRECTION_UP;
	config.counter_size = TC_COUNTER_SIZE_16BIT;
	//config.double_buffering_enabled = false;
	config.enable_capture_on_channel[0] = false;
	config.enable_capture_on_channel[1] = false;
	//config.enable_capture_on_IO = false;
	config.oneshot = false;
	config.pwm_channel[0].enabled = true;
	config.pwm_channel[0].pin_mux = MUX_PA18E_TC3_WO0;
	config.pwm_channel[0].pin_out = PIN_PA18E_TC3_WO0;
	config.run_in_standby = true;
	config.wave_generation = TC_WAVE_GENERATION_NORMAL_PWM;
	config.waveform_invert_output = 0b0;

	tc_init(&instance, TC3, &config);
	//tc_set_top_value(&instance, res.prescaledTicksPerPeriod);
	tc_enable(&instance);
}

 TCMotor1::TCMotor1() : TCMotor(res, TC_COMPARE_CAPTURE_CHANNEL_0)
{
	tc_config config;
	tc_get_config_defaults(&config);
	config.clock_prescaler = tccPrescaler_to_tcPrescaler(res.prescaleSetting);
	config.clock_source = GCLK_GENERATOR_3;
	config.count_direction = TC_COUNT_DIRECTION_UP;
	config.counter_size = TC_COUNTER_SIZE_16BIT;
	//config.double_buffering_enabled = false;
	config.enable_capture_on_channel[0] = false;
	config.enable_capture_on_channel[1] = false;
	//config.enable_capture_on_IO = false;
	config.oneshot = false;
	config.pwm_channel[0].enabled = true;
	config.pwm_channel[0].pin_mux = MUX_PA22E_TC4_WO0;
	config.pwm_channel[0].pin_out = PIN_PA22E_TC4_WO0;
	config.run_in_standby = true;
	config.wave_generation = TC_WAVE_GENERATION_NORMAL_PWM;
	config.waveform_invert_output = 0b0;

	tc_init(&instance, TC4, &config);
	//tc_set_top_value(&instance, res.prescaledTicksPerPeriod);
	tc_enable(&instance);
}
