/*
 * encoder.cpp
 *
 * Created: 19/01/2018 11:28:37 PM
 *  Author: teddy
 */ 

#include "encoder.h"
#include "config.h"
#include <extint.h>
#include <events.h>

tcc_module tccEncoder::instance;

namespace {
	void tcc2_overflow_callback(tcc_module *const module) {
		Encoder0::overflow();
		Encoder1::overflow();
		Encoder2::overflow();
	}

	void tcc2_channel_0_callback(tcc_module *const module) {
		Encoder0::compare(tcc_get_capture_value(module, TCC_MATCH_CAPTURE_CHANNEL_0));
	}

	void tcc2_channel_1_callback(tcc_module *const module) {
		Encoder1::compare(tcc_get_capture_value(module, TCC_MATCH_CAPTURE_CHANNEL_1));
	}

	void tcc2_counterEvent_callback(tcc_module *const module) {
		Encoder2::compare(tcc_get_capture_value(module, TCC_MATCH_CAPTURE_CHANNEL_0));
	}
}

float tccEncoder::motor_speedConvert(float const interval)
{
	return 0;
}

float tccEncoder::motor_driveTrainConvert(float const interval)
{
	return 0;
}

void tccEncoder::init()
{
	//TCC2 has 4 input events. Input event 0, input event 1, compare match input 0, compare match input 1.
	//CM0 goes to CC0, and used for motor0 on the callback function
	//CM1 goes to CC1, and used for motor1 on the callback function
	//IN0 can be configured to go to fill CC0 and CC1 with preriod and duty cycle. Period is put into CC0

	//---Setup external interrupts and events---
	constexpr uint8_t enc0_extintChannel = 6;
	extint_chan_conf en0conf;
	extint_chan_get_config_defaults(&en0conf);
	en0conf.detection_criteria = EXTINT_DETECT_RISING;
	en0conf.gpio_pin = PIN_PB22A_EIC_EXTINT6;
	en0conf.gpio_pin_mux = MUX_PB22A_EIC_EXTINT6;
	en0conf.gpio_pin_pull = EXTINT_PULL_DOWN;
	extint_chan_set_config(enc0_extintChannel, &en0conf);

	constexpr uint8_t enc1_extintChannel = 7;
	extint_chan_conf en1conf;
	extint_chan_get_config_defaults(&en1conf);
	en0conf.detection_criteria = EXTINT_DETECT_RISING;
	en0conf.gpio_pin = PIN_PB23A_EIC_EXTINT7;
	en0conf.gpio_pin_mux = MUX_PB23A_EIC_EXTINT7;
	en0conf.gpio_pin_pull = EXTINT_PULL_DOWN;
	extint_chan_set_config(enc1_extintChannel, &en1conf);
	
	constexpr uint8_t enc2_extintChannel = 2;
	extint_chan_conf en2conf;
	extint_chan_get_config_defaults(&en2conf);
	en0conf.detection_criteria = EXTINT_DETECT_RISING;
	en0conf.gpio_pin = PIN_PA02A_EIC_EXTINT2;
	en0conf.gpio_pin_mux = MUX_PA02A_EIC_EXTINT2;
	en0conf.gpio_pin_pull = EXTINT_PULL_DOWN;
	extint_chan_set_config(enc2_extintChannel, &en2conf);

	extint_events extevents;
	extevents.generate_event_on_detect[enc0_extintChannel] = true;
	extevents.generate_event_on_detect[enc1_extintChannel] = true;
	extevents.generate_event_on_detect[enc2_extintChannel] = true;
	extint_enable_events(&extevents);

	//---Init TCC2 (motor0 and motor1)---
	tcc_config config;
	tcc_get_config_defaults(&config, TCC2);
	config.capture.channel_function[0] = TCC_CHANNEL_FUNCTION_CAPTURE;
	config.counter.clock_prescaler = ::config::encoder::prescaeSetting;
	config.counter.clock_source = ::config::encoder::clockSource;
	config.counter.direction = TCC_COUNT_DIRECTION_UP;
	config.counter.period = 0xffff;
	config.double_buffering_enabled = false;
	config.run_in_standby = true;
	
	tcc_init(&instance, TCC2, &config);

	tcc_events events;
	events.input_config[0].action = TCC_EVENT_ACTION_PERIOD_PULSE_WIDTH_CAPTURE;
	events.on_input_event_perform_action[0] = true;
	tcc_enable_events(&instance, &events);

	tcc_register_callback(&instance, tcc2_overflow_callback, TCC_CALLBACK_OVERFLOW);
	tcc_enable_callback(&instance, TCC_CALLBACK_OVERFLOW);

	tcc_register_callback(&instance, tcc2_channel_0_callback, TCC_CALLBACK_CHANNEL_0);
	tcc_enable_callback(&instance, TCC_CALLBACK_CHANNEL_0);

	tcc_register_callback(&instance, tcc2_channel_1_callback, TCC_CALLBACK_CHANNEL_1);
	tcc_enable_callback(&instance, TCC_CALLBACK_CHANNEL_1);

	tcc_register_callback(&instance, tcc2_counterEvent_callback, TCC_CALLBACK_COUNTER_EVENT);
	tcc_enable_callback(&instance, TCC_CALLBACK_COUNTER_EVENT);

	//---Setup event routing---
	events_config evconfig_en0;
	events_get_config_defaults(&evconfig_en0);
	evconfig_en0.edge_detect = EVENTS_EDGE_DETECT_NONE;
	evconfig_en0.path = EVENTS_PATH_ASYNCHRONOUS;
	evconfig_en0.generator = EVSYS_ID_GEN_EIC_EXTINT_6;
	events_resource evresource_en0;
	events_allocate(&evresource_en0, &evconfig_en0);
	events_attach_user(&evresource_en0, EVSYS_ID_USER_TCC2_MC_0);
	
	events_config evconfig_en1;
	events_get_config_defaults(&evconfig_en1);
	evconfig_en1.edge_detect = EVENTS_EDGE_DETECT_NONE;
	evconfig_en1.path = EVENTS_PATH_ASYNCHRONOUS;
	evconfig_en1.generator = EVSYS_ID_GEN_EIC_EXTINT_7;
	events_resource evresource_en1;
	events_allocate(&evresource_en1, &evconfig_en1);
	events_attach_user(&evresource_en1, EVSYS_ID_USER_TCC2_MC_1);

	events_config evconfig_en2;
	events_get_config_defaults(&evconfig_en2);
	evconfig_en2.edge_detect = EVENTS_EDGE_DETECT_NONE;
	evconfig_en2.path = EVENTS_PATH_ASYNCHRONOUS;
	evconfig_en2.generator = EVSYS_ID_GEN_EIC_EXTINT_2;
	events_resource evresource_en2;
	events_allocate(&evresource_en2, &evconfig_en2);
	events_attach_user(&evresource_en2, EVSYS_ID_USER_TCC2_EV_0);

	//Enable the TCC
	tcc_enable(&instance);
}
