/*
 * run.cpp
 *
 * Created: 25/01/2018 1:47:58 PM
 *  Author: teddy
 */ 

#include "run.h"
#include "instance.h"
#include "dep_instance.h"
#include <timers.h>
#include <algorithm>
#include <numeric>
#include <type_traits>

template <typename T>
constexpr bool withinError(T const value, T const target, T const error) {
	return (value < (target + (target * error))) && (value > (target - (target * error)));
}

template <typename T>
constexpr T enumCycle(T const v) {
	static_assert(std::is_enum<T>::value, "T must be of enum type");
	T next = static_cast<T>(static_cast<std::underlying_type_t<T>>(v) + 1);
	if(next == T::_size)
		next = static_cast<T>(0);
	return next;
}

Run::TimerUpdate::TimerUpdate(TickType_t const period /*= config::run::displaycycleperiod*/)
{
	if((timer = xTimerCreate("line", period, pdTRUE, this, callback)) == NULL)
		debugbreak();
}

Run::TimerUpdate::~TimerUpdate()
{
	if(timer != NULL)
		xTimerDelete(timer, portMAX_DELAY);
}

std::string Run::DL_Idle_Top::get_text(Input const &input)
{
	return "Ready";
}

void Run::TimerUpdate::callback(TimerHandle_t timer)
{
	static_cast<TimerUpdate *>(pvTimerGetTimerID(timer))->cycle();
}

std::string Run::DL_Battery::get_text(Input const &input)
{
	char str[16];
	switch(curcycle) {
	case Cycle::Current:
		std::snprintf(str, 16, "BC1 %#4.2f C2 %#4.2f", input.bms0data.current, input.bms1data.current);
		break;
	case Cycle::Voltage:
		std::snprintf(str, 16, "BV1 %#4.1f V2 %#4.1f",
			std::accumulate(input.bms0data.cellVoltage.begin(), input.bms0data.cellVoltage.end(), 0.0f),
			std::accumulate(input.bms1data.cellVoltage.begin(), input.bms1data.cellVoltage.end(), 0.0f));
		break;
	case Cycle::AvgCellVoltage:
		std::snprintf(str, 16, "BA1 %#4.2f A2 %#4.2f",
			std::accumulate(input.bms0data.cellVoltage.begin(), input.bms0data.cellVoltage.end(), 0.0f) / input.bms0data.cellVoltage.size(),
			std::accumulate(input.bms1data.cellVoltage.begin(), input.bms1data.cellVoltage.end(), 0.0f) / input.bms1data.cellVoltage.size());
		break;
	case Cycle::_size:
		break;
	}
	return {str, 16};
}

void Run::DL_Battery::cycle()
{
	curcycle = enumCycle(curcycle);
}

std::string Run::DL_SpeedEnergy::get_text(Input const &input)
{
	char str[16];
	switch(curcycle) {
	case Cycle::SpeedEnergy:
		snprintf(str, 16, "%#5.2fkmh %#5.2fWh", input.vehicleSpeed, input.totalEnergyUsage);
		break;
	case Cycle::Time:
		snprintf(str, 16, "Time:      %02u:%02u", static_cast<unsigned int>(input.time) / 60, static_cast<unsigned int>(input.time) % 60);
		break;
	case Cycle::_size:
		break;
	}
	return {str, 16};
}

void Run::DL_SpeedEnergy::cycle()
{
	curcycle = enumCycle(curcycle);
}

std::string Run::DL_Ramping::get_text(Input const &input)
{
	char str[16];
	switch(curcycle) {
	case Cycle::MotorCurrent:
		snprintf(str, 16, "MC1 %4.2f C2 %4.2f", input.motor0Current, input.motor1Current);
		break;
	case Cycle::DutyCycle:
		snprintf(str, 16, "MD1 %4.2f D2 %4.2f", input.motor0DutyCycle, input.motor1DutyCycle);
		break;
	case Cycle::Rampspeed:
		snprintf(str, 16, "MR1 %4.4u R2 %4.4u", static_cast<unsigned int>(input.motor0Speed), static_cast<unsigned int>(input.motor1Speed));
		break;
	case Cycle::_size:
		break;
	}
	return {str, 16};
}

void Run::DL_Ramping::cycle()
{
	curcycle = enumCycle(curcycle);
}

std::string Run::DL_Coasting::get_text(Input const &input)
{
	if(firstGetText) {
		firstGetText = false;
		starttime = input.time;
	}
	char str[16];
	switch(curcycle) {
	case Cycle::BatteryVoltage:
		std::snprintf(str, 16, "BV1 %#4.1f V2 %#4.1f",
			std::accumulate(input.bms0data.cellVoltage.begin(), input.bms0data.cellVoltage.end(), 0.0f),
			std::accumulate(input.bms1data.cellVoltage.begin(), input.bms1data.cellVoltage.end(), 0.0f));
		break;
	case Cycle::CoastTime:
		snprintf(str, 16, "Coasting:  %2.2u:%2.2u", static_cast<unsigned int>(input.time - starttime) / 60, static_cast<unsigned int>(input.time - starttime) % 60);
		break;
	case Cycle::Distance:
		snprintf(str, 16, "Distance: %#6.3f", input.distance);
		break;
	case Cycle::_size:
		break;
	}
	return {str, 16};
}

void Run::Task::displayUpdate(Display &disp)
{
}

Run::Output Run::Idle::update(Input const &input)
{
	Output out;
	out.servo0Position = config::run::servorestposition;
	out.servo1Position = config::run::servorestposition;
	out.motor0DutyCycle = 0;
	out.motor1DutyCycle = 0;
	out.output.set();
	return out;
}

Run::Task *Run::Idle::complete(Input const &input)
{
	if(input.opState)
		return new (malloc(sizeof(Engage))) Engage(input, Engage::Servo::Both, Engage::PostTask::Startup);
	return nullptr;
}

void Run::Idle::displayUpdate(Display &disp)
{
	disp.topline.reset(new (malloc(sizeof(DL_Idle_Top))) DL_Idle_Top);
	disp.bottomline.reset(new (malloc(sizeof(DL_Battery))) DL_Battery);
}

Run::Output Run::Engage::update(Input const &input)
{
	Output out;
	if((servo == Engage::Servo::Servo0) || (servo == Engage::Servo::Both)) {
		out.servo0Position = f.currentValue(input.time);
		out.output[Output::Element::Servo0Position] = true;
	}
	if((servo == Engage::Servo::Servo1) || (servo == Engage::Servo::Both)) {
		out.servo0Position = f.currentValue(input.time);
		out.output[Output::Element::Servo1Position] = true;
	}
	return out;
}

Run::Task * Run::Engage::complete(Input const &input)
{
	if(input.time >= f.get_time_destination()) {
		if(post == PostTask::Startup)
			return new (malloc(sizeof(Startup))) Startup(input);
		else
			return new (malloc(sizeof(CoastRamp))) CoastRamp(input);
	}
	return nullptr;
}

Run::Engage::Engage(Input const &input, Servo const servo, PostTask const post) : servo(servo), post(post)
{
	runtime::vbBuzzermanager->registerSequence([](Buzzer &buz) {
		//3 quick beeps
		for(unsigned int i = 0; i < 3; i++) {
			buz.setState(true);
			vTaskDelay(msToTicks(75));
			buz.setState(false);
			vTaskDelay(msToTicks(50));
		}
	});
	f.startup(config::run::servorestposition, config::run::servoengagedpositon, input.time, input.time + config::run::servoengagetime);
}

Run::Output Run::Startup::update(Input const &input)
{
	Output out;
	out.motor0DutyCycle = f.currentValue(input.time);
	out.motor1DutyCycle = f.currentValue(input.time);
	if(input.driveSpeed >= config::run::maximumspeed) {
		out.motor0DutyCycle = 0;
		out.motor1DutyCycle = 0;
	}
	out.output[Output::Element::Motor0DutyCycle] = true;
	out.output[Output::Element::Motor1DutyCycle] = true;
	return out;
}

Run::Task * Run::Startup::complete(Input const &input)
{
	if(input.driveSpeed >= config::run::maximumspeed)
		return new (malloc(sizeof(Disengage))) Disengage(input, Disengage::Servo::Both);
	return nullptr;
}

void Run::Startup::displayUpdate(Display &disp)
{
	disp.topline.reset(new (malloc(sizeof(DL_SpeedEnergy))) DL_SpeedEnergy);
	disp.bottomline.reset(new (malloc(sizeof(DL_Ramping))) DL_Ramping);
}

 Run::Startup::Startup(Input const &input)
{
	f.startup(0, 1, input.time, input.time + config::run::ramuptime);
}

Run::Output Run::Disengage::update(Input const &input)
{
	//Literally the same thing as Engage, maybe should just make the same class?
	Output out;
	if((servo == Disengage::Servo::Servo0) || (servo == Disengage::Servo::Both)) {
		out.servo0Position = f.currentValue(input.time);
		out.output[Output::Element::Servo0Position] = true;
	}
	if((servo == Disengage::Servo::Servo1) || (servo == Disengage::Servo::Both)) {
		out.servo0Position = f.currentValue(input.time);
		out.output[Output::Element::Servo1Position] = true;
	}
	return out;
}

Run::Task * Run::Disengage::complete(Input const &input)
{
	//Also basically the same as engage
	if(input.time >= f.get_time_destination())
		return new (malloc(sizeof(Coast))) Coast;
	return nullptr;
}

 Run::Disengage::Disengage(Input const &input, Servo const servo) : servo(servo)
{
	f.startup(config::run::servoengagedpositon, config::run::servorestposition, input.time, input.time + config::run::servodisengagetime);
}

Run::Output Run::Coast::update(Input const &input)
{
	//Don't change anything
	return {};
}

Run::Task * Run::Coast::complete(Input const &input)
{
	if(input.driveSpeed <= config::run::minimumspeed)
		return new (malloc(sizeof(CoastRamp))) SpeedMatch(input);
	return nullptr;
}


void Run::Coast::displayUpdate(Display &disp)
{
	disp.bottomline.reset(new (malloc(sizeof(DL_Coasting))) DL_Coasting);
}

Run::Output Run::CoastRamp::update(Input const &input)
{
	Output out;
	out.motor0DutyCycle = f.currentValue(input.time);
	out.output[Output::Element::Motor0DutyCycle] = true;
	if(input.driveSpeed >= config::run::maximumspeed)
		out.motor0DutyCycle = 0;
	return out;
}

Run::Task * Run::CoastRamp::complete(Input const &input)
{
	if(input.driveSpeed >= config::run::maximumspeed)
		return new(malloc(sizeof(Disengage))) Disengage(input, Disengage::Servo::Servo0);
	return nullptr;
}

void Run::CoastRamp::displayUpdate(Display &disp)
{
	disp.bottomline.reset(new (malloc(sizeof(DL_Ramping))) DL_Ramping);
}

 Run::CoastRamp::CoastRamp(Input const &input)
{
	f.startup(0, 1, input.time, input.time + config::run::coastramptime);
}

Run::Output Run::SpeedMatch::update(Input const &input)
{
	Output out;
	out.motor0DutyCycle = f.currentValue(input.time);
	out.output[Output::Element::Motor0DutyCycle] = true;
	if(withinError(input.motor0Speed, input.driveSpeed, config::run::errorrange))
		out.motor0DutyCycle = 0;
	return out;
}

Run::Task * Run::SpeedMatch::complete(Input const &input)
{
	if(withinError(input.motor0Speed, input.driveSpeed, config::run::errorrange))
		return new(malloc(sizeof(Engage))) Engage(input, Engage::Servo::Servo0, Engage::PostTask::CoastRamp);
	return nullptr;
}

Run::Output Run::OPCheck::update(Input const &input)
{
	Output out;
	if((input.opState == false) && (previousOPState == true)) {
		//5 beeps to let the user know (in case they forgot)
		runtime::vbBuzzermanager->registerSequence([](Buzzer &buz) {
			for(unsigned int i = 0; i < 5; i++) {
				buz.start();
				vTaskDelay(msToTicks(500));
				buz.stop();
				vTaskDelay(msToTicks(500));
			}
		});
	}
	if(input.opState == false) {
		out.motor0DutyCycle = 0;
		out.motor1DutyCycle = 0;
	}
	previousOPState = input.opState;
	return out;
}

Run::Task * Run::OPCheck::complete(Input const &input)
{
	return nullptr;
}

Run::Output Run::BatteryCheck::update(Input const &input)
{
	Output out;
	if(input.bms0data.current > config::run::alertbmscurrent) {
		out.motor0DutyCycle = 0;
		out.motor1DutyCycle = 0;
		//Turn on the BMS led
		runtime::bms0->setLEDState(true);
	}
	if(input.bms0data.current > config::run::alertbmscurrent) {
		//Alert the user
		runtime::vbBuzzermanager->registerSequence([](Buzzer &buz) {
			for(unsigned int i = 0; i < 6; i++) {
				buz.start();
				vTaskDelay(msToTicks(50));
				buz.stop();
				vTaskDelay(msToTicks(50));
			}
		});
	}
	return out;
}
