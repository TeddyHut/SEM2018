/*
 * leds.cpp
 *
 * Created: 22/01/2018 3:19:48 AM
 *  Author: teddy
 */ 

#include "leds.h"
#include <port.h>

void PortLED::setLEDState(bool const state)
{
	port_pin_set_output_level(pin, state);
}

void PortLED::init(uint8_t const pin)
{
	this->pin = pin;
	port_config config;
	port_get_config_defaults(&config);
	config.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(pin, &config);
}

void LEDManager::init(LED *const led)
{
	FeedbackManager<LED>::init(led, config::ledmanager::sequenceSqueueSize);
}
