/*
 * sensor.cpp
 *
 * Created: 10/02/2018 12:59:00 PM
 *  Author: teddy
 */ 

#include "sensor.h"
#include "util.h"
#include "main_config.h"

void EMC1701CurrentSensor::init(EMC1701 *const emc)
{
	this->emc = emc;
}

float EMC1701CurrentSensor::voltageDifference()
{
	if(emc == nullptr)
		return 0;
	int16_t adcval;
	*reinterpret_cast<uint16_t *>(&adcval) = (emc->readRegister(emc1701::Register::VsenseHighByte)) << 8 | (emc->readRegister(emc1701::Register::VsenseLowByte));
	return adcutil::adc_voltage<int16_t, 0x7fff>(adcval, config::emc1701::referenceVoltage);
}

float EMC1701CurrentSensor::value()
{
	return voltageDifference() / config::emc1701::referenceVoltage;
}

void EMC1701VoltageSensor::init(EMC1701 *const emc)
{
	this->emc = emc;
}

float EMC1701VoltageSensor::value()
{
	if(emc == nullptr)
		return 0;
	uint16_t vsourcevalue = (emc->readRegister(emc1701::Register::VsourceHighByte) << 8) | (emc->readRegister(emc1701::Register::VsourceLowByte));
	//From the emc1701 datasheet
	return 23.9883f * (vsourcevalue / 4094);
}
