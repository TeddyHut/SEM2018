/*
 * motorcurrent.h
 *
 * Created: 1/03/2018 9:43:11 AM
 *  Author: teddy
 */ 

#pragma once

#include <FreeRTOS.h>
#include <semphr.h>

#include "../adcmanager/adcmanager.h"
#include "../sensor.h"

class MotorCurrent : public Sensor<float> {
public:
	void init(ADCManager *const parent, adc_positive_input const input);
	float value() override;
private:
	void addJob();
	ADCManager *parent = nullptr;
	adc_positive_input input = ADC_POSITIVE_INPUT_PIN0;
	uint16_t adcBuffer = 0;
};
