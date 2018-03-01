/*
 * sensor.h
 *
 * Created: 10/02/2018 12:58:44 PM
 *  Author: teddy
 */ 

#pragma once

template <typename T = float>
class Sensor {
public:
	virtual T value() = 0;
};
