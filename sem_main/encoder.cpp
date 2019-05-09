/*
 * encoder.cpp
 *
 * Created: 19/01/2018 11:28:37 PM
 *  Author: teddy
 */ 

#include "encoder.h"
#include "main_config.h"
#include <string.h>
#include <extint.h>
#include <events.h>
#include <port.h>
#include <events_hooks.h>

tcc_module tccEncoder::instance;

void tcc2_overflow_callback(tcc_module *const module) {
	Encoder0::overflow();
	Encoder1::overflow();
}

void event_motor_callback() {
	Encoder0::compare(Encoder0::getCounterValue());
}

void event_wheel_callback() {
	Encoder0::compare(Encoder0::getCounterValue());
}

void EVSYS_Handler() {
	uint32_t const flags = EVSYS->INTFLAG.reg;
	//Channel 0 event
	//if(flags & 0x00000100)
	//	event_motor0_callback();
	////Channel 1 event
	//if(flags & 0x00000200)
	//	event_motor1_callback();
	//Channel 2 event
	if(flags & 0x00000400)
		event_wheel_callback();
	//Clear handled flags
	EVSYS->INTFLAG.reg = flags;
}


float tccEncoder::motor_speedConvert(float const interval)
{
/*
	//Need to get gears per second
	//Interval = number of clock ticks since last cycle
	//Get the time of the interval in seconds
	float sec_interval = (interval / config::motor::clockFrequency);
	//Lower interval = more gears per second.
	if(sec_interval >= 1.5 || sec_interval == 0)
		return 0;
	return 3 / sec_interval;
	*/
	return 0;
}

/*
float tccEncoder::motor_driveTrainConvert(float const interval)
{
	return motor_speedConvert(interval) / config::hardware::motortogeartickratio;
}*/

void tccEncoder::init()
{
	//TCC2 has 4 input events. Input event 0, input event 1, compare match input 0, compare match input 1.
	//CM0 goes to CC0, and used for motor0 on the callback function
	//CM1 goes to CC1, and used for motor1 on the callback function
	//IN0 can be configured to go to fill CC0 and CC1 with preriod and duty cycle. Period is put into CC0
	///^^^ Was having trouble getting that working. Just uses events interrupts now (not ext so that priority zero can be set)

	//---Enable pins as using a pull resistor, PORTMUX mode, and input--- (supposed to do this automatically I think, but is only doing it for PB22)
	port_config pinconfig;
	port_get_config_defaults(&pinconfig);
	pinconfig.direction = PORT_PIN_DIR_INPUT;
	pinconfig.input_pull = PORT_PIN_PULL_NONE;
	
	port_pin_set_config(PIN_PA02, &pinconfig);
	port_pin_set_config(PIN_PB22, &pinconfig);
	port_pin_set_config(PIN_PB23, &pinconfig);
	PORT->Group[0].PINCFG[2].bit.PMUXEN = 0x1;
	PORT->Group[1].PINCFG[22].bit.PMUXEN = 0x1;
	PORT->Group[1].PINCFG[23].bit.PMUXEN = 0x1;
	//^^Must need to do that due to driver bugs (not sure what else I'm going wrong?)

	//---Setup external interrupts and events---
	/*
	constexpr uint8_t enc0_extintChannel = 6;
	extint_chan_conf en0conf;
	extint_chan_get_config_defaults(&en0conf);
	en0conf.detection_criteria = EXTINT_DETECT_RISING;
	en0conf.gpio_pin = PIN_PB22A_EIC_EXTINT6;
	en0conf.gpio_pin_mux = MUX_PB22A_EIC_EXTINT6;
	en0conf.gpio_pin_pull = EXTINT_PULL_UP;
	extint_chan_set_config(enc0_extintChannel, &en0conf);
	//extint_register_callback(extint0Callback, 6, EXTINT_CALLBACK_TYPE_DETECT);
	//extint_chan_enable_callback(6, EXTINT_CALLBACK_TYPE_DETECT);
	EIC->CONFIG->bit.SENSE6 = 0x1;

	constexpr uint8_t enc1_extintChannel = 7;
	extint_chan_conf en1conf;
	extint_chan_get_config_defaults(&en1conf);
	en0conf.detection_criteria = EXTINT_DETECT_RISING;
	en0conf.gpio_pin = PIN_PB23A_EIC_EXTINT7;
	en0conf.gpio_pin_mux = MUX_PB23A_EIC_EXTINT7;
	en0conf.gpio_pin_pull = EXTINT_PULL_UP;
	extint_chan_set_config(enc1_extintChannel, &en1conf);
	//extint_register_callback(extint1Callback, 7, EXTINT_CALLBACK_TYPE_DETECT);
	//extint_chan_enable_callback(7, EXTINT_CALLBACK_TYPE_DETECT);
	//Either I'm doing somerhing wrong or the sense config ins't working properly (setting to 0x2 instead of 0x1)
	EIC->CONFIG->bit.SENSE7 = 0x1;
	*/
	
	//Wheel sensor
	constexpr uint8_t enc2_extintChannel = 2;
	extint_chan_conf en2conf;
	extint_chan_get_config_defaults(&en2conf);
	en2conf.detection_criteria = EXTINT_DETECT_RISING;
	en2conf.gpio_pin = PIN_PA02A_EIC_EXTINT2;
	en2conf.gpio_pin_mux = MUX_PA02A_EIC_EXTINT2;
	en2conf.gpio_pin_pull = EXTINT_PULL_DOWN;
	en2conf.filter_input_signal = true;

	extint_chan_set_config(enc2_extintChannel, &en2conf);
	//extint_register_callback(extint2Callback, 2, EXTINT_CALLBACK_TYPE_DETECT);
	//extint_chan_enable_callback(2, EXTINT_CALLBACK_TYPE_DETECT);
	EIC->CONFIG->bit.SENSE2 = 0x1;

	extint_events extevents;
	memset(&extevents, 0, sizeof(extint_events));
	//extevents.generate_event_on_detect[enc0_extintChannel] = true;
	//extevents.generate_event_on_detect[enc1_extintChannel] = true;
	extevents.generate_event_on_detect[enc2_extintChannel] = true;
	extint_enable_events(&extevents);

	//---Init TCC2 (motor0 and motor1)---
	tcc_config config;
	tcc_get_config_defaults(&config, TCC2);
	//Compare is used for SpeedMatch update, to get higher refresh rate
	config.capture.channel_function[0] = TCC_CHANNEL_FUNCTION_COMPARE;
	config.counter.clock_prescaler = ::config::encoder::prescaeSetting;
	config.counter.clock_source = ::config::encoder::clockSource;
	config.counter.direction = TCC_COUNT_DIRECTION_UP;
	config.counter.period = 0xffff;
	config.double_buffering_enabled = false;
	config.run_in_standby = true;
	
	tcc_init(&instance, TCC2, &config);

	tcc_register_callback(&instance, tcc2_overflow_callback, TCC_CALLBACK_OVERFLOW);
	tcc_enable_callback(&instance, TCC_CALLBACK_OVERFLOW);

	system_gclk_chan_config gclk_chan_conf;
	system_gclk_chan_get_config_defaults(&gclk_chan_conf);
	gclk_chan_conf.source_generator = ::config::encoder::clockSource;

	////Select channel 0, synchronous path, rising edge selection, extint6 generator
	//EVSYS->CHANNEL.reg = (0 << 0) | (0 << 24) | (1 << 26) | (0x12 << 16);
	//system_gclk_chan_set_config(EVSYS_GCLK_ID_0, &gclk_chan_conf);
	//system_gclk_chan_enable(EVSYS_GCLK_ID_0);
	//
	////Select channel 1, synchronous path, rising edge selection, extint7 generator
	//EVSYS->CHANNEL.reg = (1 << 0) | (0 << 24) | (1 << 26) | (0x13 << 16);
	//system_gclk_chan_set_config(EVSYS_GCLK_ID_1, &gclk_chan_conf);
	//system_gclk_chan_enable(EVSYS_GCLK_ID_1);

	//Select channel 0, synchronous path, rising edge selection, extint2 generator
	EVSYS->CHANNEL.reg = (2 << 0) | (0 << 24) | (1 << 26) | (0x0e << 16);
	
	system_gclk_chan_set_config(EVSYS_GCLK_ID_2, &gclk_chan_conf);
	system_gclk_chan_enable(EVSYS_GCLK_ID_2);
	
	//Interrupt enable for channels 0, 1, and 2
	EVSYS->INTENSET.reg = 0x00000700;

	//Enable system interrupts for EVSYS
	if (!system_interrupt_is_enabled(SYSTEM_INTERRUPT_MODULE_EVSYS)) {
		system_interrupt_enable(SYSTEM_INTERRUPT_MODULE_EVSYS);
	}

	//Enable the TCC
	tcc_enable(&instance);
}
