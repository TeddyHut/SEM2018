/*
 * sensor.h
 *
 * Created: 10/02/2018 12:58:44 PM
 *  Author: teddy
 */ 

#pragma once

#include "emc1701.h"

template <typename T = float>
class Sensor {
public:
	virtual T value() = 0;
};

class MotorCurrentSensor : public Sensor<float> {
public:
};

class BMSCurrentSensor : public Sensor<float> {
public:
};

class EMC1701CurrentSensor : public Sensor<float> {
public:
	void init(EMC1701 *const emc);
	float voltageDifference();
	float value() override;
private:
	EMC1701 *emc = nullptr;
};

class EMC1701VoltageSensor : public Sensor<float> {
public:
	void init(EMC1701 *const emc);
	float value() override;
private:
	EMC1701 *emc = nullptr;
};
