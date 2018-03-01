/*
 * rtc.h
 *
 * Created: 1/03/2018 10:19:42 AM
 *  Author: teddy
 */ 

#pragma once

#include <rtc_count.h>

#include "../sensor.h"

class RTCCount : public Sensor<float> {
public:
	void init();
	float value() override;
private:
	rtc_module rtc_instance;
};
