/*
 * leds.cpp
 *
 * Created: 22/01/2018 3:19:48 AM
 *  Author: teddy
 */ 

#include <port.h>

#include "gpiooutput.h"

void GPIOOutput::init(uint8_t const pin)
{
	this->pin = pin;
	port_config config;
	port_get_config_defaults(&config);
	config.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(pin, &config);
}

void GPIOOutput::setPinState(bool const state)
{
	port_pin_set_output_level(pin, state);
}

void GPIOOutput::valueItem_setValue(bool const value) {
	setPinState(value);
}
