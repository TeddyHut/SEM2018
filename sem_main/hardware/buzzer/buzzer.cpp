/*
 * buzzer.cpp
 *
 * Created: 20/01/2018 2:58:57 PM
 *  Author: teddy
 */ 

#include "buzzer.h"

#include "../../main_config.h"
#include "../utility/timer.h"

constexpr timer::StaticConfig<float, uint16_t, 0xffff>::Result res = timer::StaticConfig<float, uint16_t, 0xffff>::calculateResult(
	config::buzzer::clockFrequency, config::buzzer::period, 0, 0, 0, 0);

void Buzzer::valueItem_setValue(bool const state)
{
	setBuzzerState(state);
}

void Buzzer0::setBuzzerState(bool const state)
{
	if(state) tc_enable(&instance);
	else tc_disable(&instance);
}

void Buzzer0::init()
{
	tc_config config;
	tc_get_config_defaults(&config);
	config.clock_prescaler = timer::tccPrescaler_to_tcPrescaler(res.prescaleSetting);
	config.clock_source = GCLK_GENERATOR_3;
	config.count_direction = TC_COUNT_DIRECTION_UP;
	config.counter_size = TC_COUNTER_SIZE_16BIT;
	config.enable_capture_on_channel[0] = false;
	config.oneshot = false;
	config.pwm_channel[0].enabled = true;
	config.pwm_channel[0].pin_mux = MUX_PB10E_TC5_WO0;
	config.pwm_channel[0].pin_out = PIN_PB10E_TC5_WO0;
	config.run_in_standby = true;
	config.wave_generation = TC_WAVE_GENERATION_MATCH_FREQ;
	config.waveform_invert_output = 0b1;
	
	tc_init(&instance, TC5, &config);
	tc_set_top_value(&instance, res.prescaledTicksPerPeriod / 2);
}
