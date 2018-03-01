/*
 * buzzer.h
 *
 * Created: 20/01/2018 2:58:15 PM
 *  Author: teddy
 */ 

#pragma once

#include <tc.h>

#include "../feedback/valueitem.h"

//Abstract representation of physical buzzer
class Buzzer : public ValueItem<bool> {
public:
	virtual void setBuzzerState(bool const state) = 0;
private:
	void valueItem_setValue(bool const state) override;
};

class Buzzer0 : public Buzzer {
public:
	void init();

	void setBuzzerState(bool const state) override;
private:
	tc_module instance;
};
