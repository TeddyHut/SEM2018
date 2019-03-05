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

using namespace program;

template <typename T>
constexpr bool withinError(T const value, T const target, T const error) {
	return (value < (target + (target * error))) && (value > (target - (target * error)));
}

void setoutputDefaults(Output &out) {
	out.motorDutyCycle = 0;
	out.output[Output::Element::MotorDutyCycle] = true;
}

void program::init()
{
	
}

void program::Task::displayUpdate(Display &disp) {}
program::Task::Task(TaskIdentity const id /*= TaskIdentity::None*/) : id(id) {}

program::Idle::Idle(bool const countdown) : Task(TaskIdentity::Idle), doCountdown(countdown) {}
program::Output program::Idle::update(Input const &input)
{
	Output out;
	out.motorDutyCycle = 0;
	
	out.programstate = Input::State::Idle;

	if(runtime::usbmsc->isReady()) {
		out.motorPWMFrequency = runtime::usbmsc->settings.motorFrequency;
		runtime::encoder0->setBufferSize(runtime::usbmsc->settings.encoderBufferSize);
		out.output[Output::Element::MotorPWMFrequency] = true;
	}

	out.output[Output::Element::MotorDutyCycle] = true;
	out.output[Output::Element::State] = true;
	return out;
}
program::Task *program::Idle::complete(Input const &input)
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
				runtime::vbBuzzermanager->registerSequence([](Buzzer &b) {
					b.setState(true);
					vTaskDelay(200);
					b.setState(false);
				});
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
		return new (pvPortMalloc(sizeof(Startup))) Startup(input);
	}
	previousOPState = input.opState;
	return nullptr;
}
void program::Idle::displayUpdate(Display &disp)
{
	enum class Alloc {
		None,
		Idle,
		USSB,
		Settings,
	} alloc = Alloc::None;
	
	//If first cycle, USB won't have been registered yet, so set display to "Connect USB"
	if(!disp.topline)
		alloc = Alloc::USSB;
	else {
		//If the button is held (as in the countdown is happening), change the display to show the countdown
		if(countdown != -1 && disp.topline->id != DisplayLine::ID::Idle)
			alloc = Alloc::Idle;
		//If the USB hasn't been connected
		else if(countdown == -1 && !runtime::usbmsc->isReady() && disp.topline->id != DisplayLine::ID::USSB)
			alloc = Alloc::USSB;
		//If the USB has been connected, show settings
		else if(countdown == -1 && runtime::usbmsc->isReady() && disp.topline->id != DisplayLine::ID::Settings)
			alloc = Alloc::Settings;
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
	case Alloc::Settings:
		disp.topline.reset(new (pvPortMalloc(sizeof(DL_Settings))) DL_Settings);
	}

	//Always show the battery statistics on the bottom line
	if(!disp.bottomline)
		disp.bottomline.reset(new (pvPortMalloc(sizeof(DL_Battery))) DL_Battery);	
}


program::Startup::Startup(Input const &input) : Task(TaskIdentity::Startup)
{
	char str[196];
	//Write settings to log file
	rpl_snprintf(str, sizeof str, "SampleFrequency,%5.2f,\nMotorFrequency,%5.0f,\nStartRampTime,%4.1f,\nCoastRampTime,%4.1f,\n",
		runtime::usbmsc->settings.sampleFrequency,
		runtime::usbmsc->settings.motorFrequency,
		runtime::usbmsc->settings.startupramptime,
		runtime::usbmsc->settings.coastramptime
		);
	f_puts(str, &runtime::usbmsc->file);
	f_sync(&runtime::usbmsc->file);
	
	rpl_snprintf(str, sizeof str, "CruiseMin(kmh-ms),%3.1f,%4.2f,\nCruiseMax(kmh-ms),%3.1f,%4.2f,\nWheelRadius,%.3f,\nMagnets,%u,\nSpeedBufferSize,%u,\n",
		runtime::usbmsc->settings.cruiseMin, kmhToMs(runtime::usbmsc->settings.cruiseMin),
		runtime::usbmsc->settings.cruiseMax, kmhToMs(runtime::usbmsc->settings.cruiseMax),
		runtime::usbmsc->settings.wheelRadius,
		runtime::usbmsc->settings.wheelSamplePoints,
		runtime::usbmsc->settings.encoderBufferSize
		);
	f_puts(str, &runtime::usbmsc->file);
	f_sync(&runtime::usbmsc->file);

	//Write cell voltages (battery 0) to log file
	rpl_snprintf(str, sizeof str, "Cells0 [0-5],%4.2f,%4.2f,%4.2f,%4.2f,%4.2f,%4.2f,\n",
		input.bms0data.cellVoltage[0],
		input.bms0data.cellVoltage[1],
		input.bms0data.cellVoltage[2],
		input.bms0data.cellVoltage[3],
		input.bms0data.cellVoltage[4],
		input.bms0data.cellVoltage[5]
		);
	f_puts(str, &runtime::usbmsc->file);
	f_sync(&runtime::usbmsc->file);

	//Write cell voltages (battery 1) to log file
	rpl_snprintf(str, sizeof str, "Cells1 [0-5],%4.2f,%4.2f,%4.2f,%4.2f,%4.2f,%4.2f,\n",
		input.bms1data.cellVoltage[0],
		input.bms1data.cellVoltage[1],
		input.bms1data.cellVoltage[2],
		input.bms1data.cellVoltage[3],
		input.bms1data.cellVoltage[4],
		input.bms1data.cellVoltage[5]
		);
	f_puts(str, &runtime::usbmsc->file);
	f_sync(&runtime::usbmsc->file);

	//Write battery temperature to log file
	rpl_snprintf(str, sizeof str, "BatteryTemps [0-1],%4.1f,%4.1f,\n",
		input.bms0data.temperature, input.bms1data.temperature);

	f_puts("Sample,Time(s),Mode,Current(A),Battery0(V),Battery1(V),Temp0(C),Temp1(C),Power(W),Energy(J),DutyCycle,Velocity(ms),Distance(m),OP,\n", &runtime::usbmsc->file);
	f_sync(&runtime::usbmsc->file);

	f.startup(0, 1, input.time, input.time + runtime::usbmsc->settings.startupramptime);
	//Run 3 quick beeps when ramping (for whatever reason, for starting 4 sounds like 3)
	runtime::vbBuzzermanager->registerSequence([](Buzzer &buz) {
		for(unsigned int i = 0; i < 4; i++) {
			buz.setState(true);
			vTaskDelay(msToTicks(75));
			buz.setState(false);
			vTaskDelay(msToTicks(50));
		}
	});
}
program::Output program::Startup::update(Input const &input)
{
	Output out;
	out.motorDutyCycle = f.currentValue(input.time);
	
	out.programstate = Input::State::Running;

	out.output.set(Output::Element::State);
	out.output.set(Output::Element::MotorDutyCycle);
	return out;
}
program::Task * program::Startup::complete(Input const &input)
{
	if(input.vehicleSpeedMs >= kmhToMs(runtime::usbmsc->settings.cruiseMax))
		return new (pvPortMalloc(sizeof(Coast))) Coast;
	return nullptr;
}
void program::Startup::displayUpdate(Display &disp)
{
	if(disp.topline->id != DisplayLine::ID::SpeedEnergy)
		disp.topline.reset(new (pvPortMalloc(sizeof(DL_SpeedEnergy))) DL_SpeedEnergy);
	if(disp.bottomline->id != DisplayLine::ID::Ramping)
		disp.bottomline.reset(new (pvPortMalloc(sizeof(DL_Ramping))) DL_Ramping);
}


program::Coast::Coast() : Task(TaskIdentity::Coast) {
	//Run 2 quick beeps when coasting
	runtime::vbBuzzermanager->registerSequence([](Buzzer &buz) {
		for(unsigned int i = 0; i < 2; i++) {
			buz.setState(true);
			vTaskDelay(msToTicks(75));
			buz.setState(false);
			vTaskDelay(msToTicks(50));
		}
	});
}

program::Output program::Coast::update(Input const &input)
{
	Output out;
	out.motorDutyCycle = 0;
	out.output[Output::Element::MotorDutyCycle] = true;
	return out;
}
program::Task * program::Coast::complete(Input const &input)
{
	if(input.vehicleSpeedMs <= kmhToMs(runtime::usbmsc->settings.cruiseMin))
		return new (pvPortMalloc(sizeof(CoastRamp))) CoastRamp(input);
	return nullptr;
}
void program::Coast::displayUpdate(Display &disp)
{
	if(disp.topline->id != DisplayLine::ID::SpeedEnergy)
		disp.topline.reset(new (pvPortMalloc(sizeof(DL_SpeedEnergy))) DL_SpeedEnergy);
	if(disp.bottomline->id != DisplayLine::ID::Coasting)
		disp.bottomline.reset(new (pvPortMalloc(sizeof(DL_Coasting))) DL_Coasting);
}


program::CoastRamp::CoastRamp(Input const &input) : Task(TaskIdentity::CoastRamp)
{
	//Run 3 quick beeps when ramping
	runtime::vbBuzzermanager->registerSequence([](Buzzer &buz) {
		for(unsigned int i = 0; i < 3; i++) {
			buz.setState(true);
			vTaskDelay(msToTicks(75));
			buz.setState(false);
			vTaskDelay(msToTicks(50));
		}
	});
	f.startup(0, 1, input.time, input.time + config::run::coastramptime);
}
program::Output program::CoastRamp::update(Input const &input)
{
	Output out;
	out.motorDutyCycle = f.currentValue(input.time);
	
	out.output[Output::Element::MotorDutyCycle] = true;
	return out;
}

program::Task * program::CoastRamp::complete(Input const &input)
{
	if(input.vehicleSpeedMs >= kmhToMs(runtime::usbmsc->settings.cruiseMax))
		return new (pvPortMalloc(sizeof(Coast))) Coast;
	return nullptr;
}

void program::CoastRamp::displayUpdate(Display &disp)
{
	if(disp.topline->id != DisplayLine::ID::SpeedEnergy)
		disp.topline.reset(new (pvPortMalloc(sizeof(DL_SpeedEnergy))) DL_SpeedEnergy);
	if(disp.bottomline->id != DisplayLine::ID::Ramping)
		disp.bottomline.reset(new (pvPortMalloc(sizeof(DL_Ramping))) DL_Ramping);
}


program::OPCheck::OPCheck() : Task(TaskIdentity::OPCheck)
{
	if((sem_buzzerComplete = xSemaphoreCreateBinary()) == NULL)
		debugbreak();
}
program::Output program::OPCheck::update(Input const &input)
{
	Output out;
	pm_changedProgramState = false;
	if(input.programstate == Input::State::Running) {
		auto buzzerSequence = [](Buzzer &buz) {
			buz.start();
			vTaskDelay(msToTicks(500));
			buz.stop();
			vTaskDelay(msToTicks(500));
		};
		//If the user just released the button, start a timeout
		if(input.opState == false && previousOPState == true)
			errorTime = input.time;
		//Give the semaphore if the buzzer isn't in the queue and the user just released the button
		if((input.opState == false) && (previousOPState == true) && !buzzerInQueue)
			xSemaphoreGive(sem_buzzerComplete);
		if(xSemaphoreTake(sem_buzzerComplete, 0) == pdTRUE) {
			if(input.opState == false) {
				runtime::vbBuzzermanager->registerSequence(buzzerSequence, sem_buzzerComplete);
				buzzerInQueue = true;
			}
			else
				buzzerInQueue = false;
		}
		if(input.opState == false) {
			setoutputDefaults(out);
		}
	}
	if((!input.opState) && ((input.time) - errorTime >= config::run::stoptimeout) && input.programstate == Input::State::Running) {
		out.programstate = Input::State::Finished;
		out.output[Output::Element::State] = true;
		pm_changedProgramState = true;
	}
	previousOPState = input.opState;
	return out;
}
program::Task * program::OPCheck::complete(Input const &input)
{
	return nullptr;
}
bool program::OPCheck::changedProgramState() const
{
	return pm_changedProgramState;
}

program::Finished::Finished(float const time, float const energy, float const distance) : Task(TaskIdentity::Finished), time(time), energy(energy), distance(distance) {
	f_close(&runtime::usbmsc->file);
}
program::Output program::Finished::update(Input const &input)
{
	Output out;
	setoutputDefaults(out);
	return out;
}
program::Task * program::Finished::complete(Input const &input)
{
	if(input.opState) {
		runtime::usbmsc->reload();
		return new (pvPortMalloc(sizeof(Idle))) Idle;
	}
	return nullptr;
}
void program::Finished::displayUpdate(Display &disp)
{
	if(disp.topline->id != DisplayLine::ID::Finished)
		disp.topline.reset(new (pvPortMalloc(sizeof(DL_Finished))) DL_Finished(time, energy, distance));
	if(disp.bottomline->id != DisplayLine::ID::Battery)
		disp.bottomline.reset(new (pvPortMalloc(sizeof(DL_Battery))) DL_Battery);
}
