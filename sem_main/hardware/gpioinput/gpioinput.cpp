/*
 * gpioinput.cpp
 *
 * Created: 1/03/2018 11:07:27 AM
 *  Author: teddy
 */ 

#include "gpioinput.h"

void gpiopin::sensor::State::init(GPIOPin *const parent)
{
	this->parent = parent;
}

bool gpiopin::sensor::State::value()
{
	if(parent != nullptr)
		return parent->state();
	return false;
}
