/*
 * leds.cpp
 *
 * Created: 22/01/2018 3:19:48 AM
 *  Author: teddy
 */ 

#include "leds.h"
#include <port.h>

LEDManager::LEDManager(LED &led) : FeedbackManager<LED>(led)
{
}

void PortLED::setLEDState(bool const state)
{
	port_pin_set_output_level(pin, state);
}

 PortLED::PortLED(uint8_t const pin) : pin(pin)
{
	port_config config;
	port_get_config_defaults(&config);
	config.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(pin, &config);
}
