/*
 * servo.h
 *
 * Created: 18/01/2018 4:06:35 PM
 *  Author: teddy
 */ 

#pragma once

#include <tcc.h>

#include "../utility/timer.h"
#include "../feedback/valueitem.h"

template <typename position_t = float>
class Servo : public ValueItem<position_t> {
public:
	virtual void setPosition(position_t const position) = 0;
private:
	void valueItem_setValue(position_t const value) override;
};

template <typename position_t /*= float*/>
void Servo<position_t>::valueItem_setValue(position_t const value)
{
	setPosition(value);
}

using ServoF = Servo<float>;

//An empty primary template so that specialization can occur
template <typename, typename>
class TCCServo {
};

//Specialization used to determine config_t
template <typename position_t, typename config_t>
class TCCServo<position_t, timer::StaticConfig<config_t>> : public Servo<position_t> {
public:
	using Result = typename timer::StaticConfig<config_t>::Result;

	void setPosition(position_t const position) override;

	TCCServo(Result const staticConfig, tcc_match_capture_channel const channel);
protected:
	Result const staticConfig;
	tcc_module instance;
	tcc_match_capture_channel const channel;
};

template <typename position_t, typename config_t>
void TCCServo<position_t, timer::StaticConfig<config_t>>::setPosition(position_t const position)
{
	//Determine angle range
	config_t angleRange = staticConfig.maxAngle - staticConfig.minAngle;
	//Determine the middle angle
	config_t midAngle = staticConfig.minAngle + (angleRange / 2);

	tcc_set_compare_value(&instance, channel, staticConfig.counter_midpoint + (staticConfig.counter_derivation * ((position - midAngle) / (angleRange / 2))));
}

template <typename position_t, typename config_t>
TCCServo<position_t, timer::StaticConfig<config_t>>::TCCServo(Result const staticConfig, tcc_match_capture_channel const channel) : staticConfig(staticConfig), channel(channel)
{
}

class TCCServo0 : public TCCServo<float, timer::StaticConfig<float>> {
	using Parent = TCCServo<float, timer::StaticConfig<float>>;
	using StaticConfig = timer::StaticConfig<float>;
	using Result = typename StaticConfig::Result;
public:
	TCCServo0();
};

class TCCServo1 : public TCCServo<float, timer::StaticConfig<float>> {
	using Parent = TCCServo<float, timer::StaticConfig<float>>;
	using StaticConfig = timer::StaticConfig<float>;
	using Result = typename StaticConfig::Result;
public:
	TCCServo1();
};
