/*
 * leds.h
 *
 * Created: 22/01/2018 3:18:52 AM
 *  Author: teddy
 */ 

#pragma once

#include "../feedback/valueitem.h"

class GPIOOutput : public ValueItem<bool>  {
public:
	void init(uint8_t const pin);

	void setPinState(bool const state);
private:
	void valueItem_setValue(bool const value) override;
	uint8_t pin;
};
