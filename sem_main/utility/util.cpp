/*
 * util.cpp
 *
 * Created: 21/01/2018 9:47:27 PM
 *  Author: teddy
 */

#include "util.h"
#include "instance.h"
#include "dep_instance.h"

void stopmovement()
{
	//Motors
	runtime::motor0->setDutyCycle(0);
	runtime::motor1->setDutyCycle(0);
	//Servos
	port_pin_set_output_level(config::servopower::servo0_power_pin, false);
	port_pin_set_output_level(config::servopower::servo1_power_pin, false);
}

void debugbreak()
{
	stopmovement();
	if(runtime::buzzer != nullptr)
		runtime::buzzer->stop();
	if(runtime::redLED != nullptr)
		runtime::redLED->setLEDState(true);
	if(runtime::greenLED != nullptr)
		runtime::greenLED->setLEDState(false);
	__asm__("BKPT");
}
