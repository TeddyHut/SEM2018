/*
 * motorcurrent.cpp
 *
 * Created: 1/03/2018 9:43:32 AM
 *  Author: teddy
 */ 

#include "../../main_config.h"
#include "../../utility/conversions.h"

#include "motorcurrent.h"

void MotorCurrent::init(ADCManager *const parent, adc_positive_input const input)
{
	this->parent = parent;
	this->input = input;
	addJob();
}

float MotorCurrent::value()
{
	addJob();
	float opamp_outV = utility::conversions::adc_voltage<uint16_t, 4096, float>(adcBuffer, (1.0f / 1.48f) * config::hardware::vcc);
	float opamp_inV = utility::conversions::motor_current_opamp_transformation_inV(opamp_outV, config::hardware::vcc);
	return utility::conversions::acs711_current(opamp_inV, config::hardware::vcc);
}

void MotorCurrent::addJob()
{
	if(parent != nullptr) {
		ADCManager::Job job;
		job.input = input;
		job.output = &adcBuffer;
		parent->addJob(job);
	}
}
