/*
 * leds.h
 *
 * Created: 22/01/2018 3:18:52 AM
 *  Author: teddy
 */ 

#pragma once

#include "util.h"

class LED {
public:
	virtual void setLEDState(bool const state) = 0;
};

class LEDManager : public FeedbackManager<LED> {
public:
	void init(LED *const led);
};

class PortLED : public LED {
public:
	void setLEDState(bool const state) override;
	void init(uint8_t const pin);
private:
	uint8_t pin;
};
