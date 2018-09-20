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
#include <cmath>
#include <type_traits>

using namespace Run;

template <typename T>
constexpr bool withinError(T const value, T const target, T const error) {
	return (value < (target + (target * error))) && (value > (target - (target * error)));
}

void setoutputDefaults(Output &out) {
	out.motorDutyCycle = 0;
	out.output[Output::Element::MotorDutyCycle] = true;
}

void Run::Task::displayUpdate(Display &disp)
{
}

 Run::Task::Task(TaskIdentity const id /*= TaskIdentity::None*/) : id(id) {}

 Run::Idle::Idle(bool const countdown) : Task(TaskIdentity::Idle), doCountdown(countdown) {}

Run::Output Run::Idle::update(Input const &input)
{
	Output out;
	out.motorDutyCycle = 0;
	out.output[Output::Element::MotorDutyCycle] = true;
	out.started = false;
	out.output[Output::Element::Started] = true;
	return out;
}

Run::Task *Run::Idle::complete(Input const &input)
{
	if(runtime::usbmsc->isReady()) {
		//Button has just been pressed
		if(previousOPState == false && input.opState == true) {
			countdown = 5;
			lastCount = xTaskGetTickCount();
		}
		//Button is released
		else if(previousOPState == true && input.opState == false) {
			countdown = -1;
		}
		//Countdown is running
		else if(countdown != -1) {
			//One second has passed since last countdown
			if(xTaskGetTickCount() - lastCount >= msToTicks(1000)) {
				//Register a quick beep
				//runtime::vbBuzzermanager->registerSequence([](Buzzer &b) {
				//	b.setState(true);
				//	vTaskDelay(200);
				//	b.setState(false);
				//});
				lastCount = xTaskGetTickCount();
				countdown--;
			}
		}
		if(!doCountdown)
			countdown = 0;
	}
	else
		countdown = -1;
	//If countdown has finished, move onto next state
	if(countdown == 0) {
		switch(runtime::usbmsc->settings.testtype) {
		case USBMSC::Settings::TestType::DutyCycle:
			return new (pvPortMalloc(sizeof(Run::DutyCycle))) Run::DutyCycle;
		case USBMSC::Settings::TestType::Frequency:
			return new (pvPortMalloc(sizeof(Run::Frequency))) Run::Frequency;
		default:
			return nullptr;
		}
	}
	previousOPState = input.opState;
	return nullptr;
}

void Run::Idle::displayUpdate(Display &disp)
{
	enum class Alloc {
		None,
		Idle,
		USSB,
	} alloc = Alloc::None;

	if(!disp.topline)
		alloc = Alloc::USSB;
	else {
		if(countdown != -1 && disp.topline->id != DisplayLine::ID::Idle)
			alloc = Alloc::Idle;
		else if(countdown == -1 && disp.topline->id != DisplayLine::ID::USSB)
			alloc = Alloc::USSB;
	}
	
	switch(alloc) {
	default:
		break;
	case Alloc::Idle:
		disp.topline.reset(new (pvPortMalloc(sizeof(DL_Idle_Top))) DL_Idle_Top(countdown));
		break;
	case Alloc::USSB:
		disp.topline.reset(new (pvPortMalloc(sizeof(DL_USB))) DL_USB);
		break;
	}

	enum class BottomAlloc {
		None,
		Settings,
		Battery,
	} bottomAlloc = BottomAlloc::None;
	if(!disp.bottomline)
		bottomAlloc = BottomAlloc::Battery;
	else {
		if(runtime::usbmsc->settings.testtype == USBMSC::Settings::TestType::None && disp.bottomline->id != DisplayLine::ID::Battery)
			bottomAlloc = BottomAlloc::Battery;
		else if(runtime::usbmsc->settings.testtype != USBMSC::Settings::TestType::None && disp.bottomline->id != DisplayLine::ID::Settings)
			bottomAlloc = BottomAlloc::Settings;
	}

	switch(bottomAlloc) {
	default:
		break;
	case BottomAlloc::Settings:
		disp.bottomline.reset(new (pvPortMalloc(sizeof(DL_Settings))) DL_Settings);
		break;
	case BottomAlloc::Battery:
		disp.bottomline.reset(new (pvPortMalloc(sizeof(DL_Battery))) DL_Battery);
		break;
	}
}

void Run::init()
{
	
}

 Run::Pull::Pull() : Task(TaskIdentity::Pull) {}

Run::Output Run::Pull::update(Input const &input)
{
	Output out;
	out.started = true;
	out.output[Output::Element::Started] = true;
	out.motorDutyCycle = runtime::usbmsc->settings.dutyCycle[0];
	out.output[Output::Element::MotorDutyCycle] = true;

	return out;
}

Run::Task * Run::Pull::complete(Input const &input)
{
	return nullptr;
}

void Run::Pull::displayUpdate(Display &disp)
{

}

 Run::DutyCycle::DutyCycle() : Task(TaskIdentity::DutyCycle)
{
	//Create a linear function
	f1.startup(
		runtime::usbmsc->settings.dutyCycle[0], runtime::usbmsc->settings.dutyCycle[1],
		0, runtime::usbmsc->settings.dutyCycleDivisions);
	char str[196];
	rpl_snprintf(str, sizeof str, "DutyCycle,\nVoltage,%f,Q-Current,%f,\nFrequency,%f,Hz,\nRange,%f,%f,\nDivisions,%u,\nDiv Time,%f,\n",
		runtime::bms0->get_data().voltage + runtime::bms1->get_data().voltage,
		std::max(std::max(runtime::bms0->get_data().current, 0.0f), std::max(runtime::bms1->get_data().current, 0.0f)),
		runtime::usbmsc->settings.frequency[0],
		runtime::usbmsc->settings.dutyCycle[0], runtime::usbmsc->settings.dutyCycle[1],
		runtime::usbmsc->settings.dutyCycleDivisions,
		runtime::usbmsc->settings.holdTime);
	f_puts(str, &runtime::usbmsc->file);
	f_puts("Sample,Time,DutyCycle,Speed,Ticks,Current\n", &runtime::usbmsc->file);
	f_sync(&runtime::usbmsc->file);
}

Run::Output Run::DutyCycle::update(Input const &input)
{
	Output out;
	if(!initialised) {
		initialised = true;
		lastStageTime = input.time;
		out.started = true;
		out.output[Output::Element::Started] = true;
		out.motorPWMFrequency = runtime::usbmsc->settings.frequency[0];
		out.output[Output::Element::MotorPWMFrequency] = true;
		out.output[Output::Element::MotorDutyCycle] = true;
	}
	out.motorDutyCycle = f1(stage);
	if(input.time >= (lastStageTime + runtime::usbmsc->settings.holdTime)) {
		lastStageTime = input.time;
		stage++;	
		out.output[Output::Element::MotorDutyCycle] = true;
	}
	if(input.logCycle) {
		char str[64];
		rpl_snprintf(str, sizeof str, "%u,%5f,%5f,%5f,%u,%5f,\n",
			input.samples,
			input.startTime,
			out.motorDutyCycle,
			input.motorSpeed,
			input.motorTicks,
			std::max(std::max(input.bms0data.current, 0.0f), std::max(input.bms1data.current, 0.0f)));
		f_puts(str, &runtime::usbmsc->file);
	}
	return out;
}

Run::Task * Run::DutyCycle::complete(Input const &input)
{
	if((stage - 1) > f1.get_time_destination()) {
		f_sync(&runtime::usbmsc->file);
		return new (pvPortMalloc(sizeof(Run::Delay))) Run::Delay;
	}
	return nullptr;
}

void Run::DutyCycle::displayUpdate(Display &disp)
{
	if(disp.topline->id != DisplayLine::ID::Samples)
		disp.topline.reset(new (pvPortMalloc(sizeof(DL_Samples))) DL_Samples);
	if(disp.bottomline->id != DisplayLine::ID::DutyCycle)
		disp.bottomline.reset(new (pvPortMalloc(sizeof(DL_DutyCycle))) DL_DutyCycle);
}

 Run::Frequency::Frequency() : Task(TaskIdentity::Frequency)
{
	f1.startup(
		runtime::usbmsc->settings.frequency[0], runtime::usbmsc->settings.frequency[1],
		0, runtime::usbmsc->settings.frequencyDivisions);
	char str[196];
	rpl_snprintf(str, sizeof str, "Frequency,\nVoltage,%f,Q-Current,%f,\nDutyCycle,%f,%%,\nRange,%f,%f,\nDivisions,%u,\nDiv Time,%f,\n",
		runtime::bms0->get_data().voltage + runtime::bms1->get_data().voltage,
		std::max(std::max(runtime::bms0->get_data().current, 0.0f), std::max(runtime::bms1->get_data().current, 0.0f)),
		runtime::usbmsc->settings.dutyCycle[0],
		runtime::usbmsc->settings.frequency[0], runtime::usbmsc->settings.frequency[1],
		runtime::usbmsc->settings.frequencyDivisions,
		runtime::usbmsc->settings.holdTime);
	f_puts(str, &runtime::usbmsc->file);
	f_puts("Sample,Time,Frequency,Speed,Ticks,Current\n", &runtime::usbmsc->file);
	f_sync(&runtime::usbmsc->file);
}

Run::Output Run::Frequency::update(Input const &input)
{
	Output out;
	if(!initialised) {
		initialised = true;
		lastStageTime = input.time;
		out.started = true;
		out.output[Output::Element::Started] = true;
		out.motorDutyCycle = runtime::usbmsc->settings.dutyCycle[0];
		out.output[Output::Element::MotorDutyCycle] = true;
		out.output[Output::Element::MotorPWMFrequency] = true;
	}
	out.motorPWMFrequency = f1(stage);
	if(input.time >= (lastStageTime + runtime::usbmsc->settings.holdTime)) {
		lastStageTime = input.time;
		stage++;
		out.output[Output::Element::MotorPWMFrequency] = true;
	}
	if(input.logCycle) {
		char str[64];
		rpl_snprintf(str, sizeof str, "%u,%5f,%5f,%5f,%u,%5f,\n",
		input.samples,
		input.startTime,
		out.motorPWMFrequency,
		input.motorSpeed,
		input.motorTicks,
		std::max(std::max(input.bms0data.current, 0.0f), std::max(input.bms1data.current, 0.0f)));
		f_puts(str, &runtime::usbmsc->file);
	}
	return out;
}

Run::Task * Run::Frequency::complete(Input const &input)
{
	if((stage - 1) > f1.get_time_destination()) {
		f_sync(&runtime::usbmsc->file);
		return new (pvPortMalloc(sizeof(Run::Delay))) Run::Delay;
	}
	return nullptr;
}

void Run::Frequency::displayUpdate(Display &disp)
{
	if(disp.topline->id != DisplayLine::ID::Samples)
		disp.topline.reset(new (pvPortMalloc(sizeof(DL_Samples))) DL_Samples);
	if(disp.bottomline->id != DisplayLine::ID::DutyCycle)
		disp.bottomline.reset(new (pvPortMalloc(sizeof(DL_Frequency))) DL_Frequency);
}

 Run::Delay::Delay() : Task(TaskIdentity::Delay) {
	//Load the next settings file
	runtime::usbmsc->reload(USBMSC::RELOAD_NEXT);
 }

Run::Output Run::Delay::update(Input const &input)
{
	if(!initialised) {
		startTime = input.time;
		initialised = true;
	}
	Output out;
	out.motorDutyCycle = 0;
	out.output[Output::Element::MotorDutyCycle] = true;
	out.started = false;
	out.output[Output::Element::Started] = true;
	return out;
}

Run::Task * Run::Delay::complete(Input const &input)
{
	if(input.time - startTime >= config::run::testRestTime)
		return new (pvPortMalloc(sizeof(Run::Idle))) Run::Idle(false);
	return nullptr;
}

void Run::Delay::displayUpdate(Display &disp)
{
	if(initialised) {
		if(disp.topline->id != DisplayLine::ID::Delay)
			disp.topline.reset(new (pvPortMalloc(sizeof(DL_Delay))) DL_Delay(startTime + config::run::testRestTime));
		if(disp.bottomline->id != DisplayLine::ID::Battery)
			disp.bottomline.reset(new (pvPortMalloc(sizeof(DL_Battery))) DL_Battery);
	}	
}
