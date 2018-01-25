/*
 * buzzer.cpp
 *
 * Created: 20/01/2018 2:58:57 PM
 *  Author: teddy
 */ 

#include "buzzer.h"

#include "config.h"
#include "servo.h"
#include "motor.h"
#include "util.h"

namespace {
	constexpr TCCServoStaticConfig<float, uint16_t, 0xffff>::Result res = TCCServoStaticConfig<float, uint16_t, 0xffff>::calculateResult(
		config::buzzer::clockFrequency, config::buzzer::period, 0, 0, 0, 0);
}

void Buzzer::start()
{
	setState(true);
}

void Buzzer::stop()
{
	setState(false);
}

void Buzzer0::setState(bool const state)
{
	if(state) tc_enable(&instance);
	else tc_disable(&instance);
}

 Buzzer0::Buzzer0()
{
	tc_config config;
	tc_get_config_defaults(&config);
	config.clock_prescaler = tccPrescaler_to_tcPrescaler(res.prescaleSetting);
	config.clock_source = GCLK_GENERATOR_3;
	config.count_direction = TC_COUNT_DIRECTION_UP;
	config.counter_size = TC_COUNTER_SIZE_16BIT;
	//config.double_buffering_enabled = false;
	config.enable_capture_on_channel[0] = false;
	//config.enable_capture_on_IO = false;
	config.oneshot = false;
	config.pwm_channel[0].enabled = true;
	config.pwm_channel[0].pin_mux = MUX_PB10E_TC5_WO0;
	config.pwm_channel[0].pin_out = PIN_PB10E_TC5_WO0;
	config.run_in_standby = true;
	config.wave_generation = TC_WAVE_GENERATION_MATCH_FREQ;
	config.waveform_invert_output = 0b1;


	tc_init(&instance, TC5, &config);
	tc_set_top_value(&instance, res.prescaledTicksPerPeriod / 2);
	//tc_set_compare_value(&instance, TC_COMPARE_CAPTURE_CHANNEL_0, res.prescaledTicksPerPeriod / 2);
	//tc_enable(&instance);
}

 BuzzerManager::BuzzerManager(Buzzer &buzzer) : FeedbackManager<Buzzer>(buzzer, config::buzzermanager::sequenceQueueSize)
{
}

void BuzzerManager::cleanup() {
	item.stop();
}