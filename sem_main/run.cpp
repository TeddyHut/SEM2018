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
	out.motor0DutyCycle = 0;
	out.motor1DutyCycle = 0;
	out.servo0Position = config::run::servo0restposition;
	out.servo1Position = config::run::servo1restposition;
	out.servo0Power = true;
	out.servo1Power = true;
	out.output[Output::Element::Motor0DutyCycle] = true;
	out.output[Output::Element::Motor1DutyCycle] = true;
	out.output[Output::Element::Servo0Position] = true;
	out.output[Output::Element::Servo1Position] = true;
	out.output[Output::Element::Servo0Power] = true;
	out.output[Output::Element::Servo1Power] = true;
}

void Run::Task::displayUpdate(Display &disp)
{
}

 Run::Task::Task(TaskIdentity const id /*= TaskIdentity::None*/) : id(id)
{
}

 Run::Idle::Idle() : Task(TaskIdentity::Idle)
{
}

Run::Output Run::Idle::update(Input const &input)
{
	Output out;
	out.servo0Position = config::run::servo0restposition;
	out.servo1Position = config::run::servo1restposition;
	out.motor0DutyCycle = 0;
	out.motor1DutyCycle = 0;
	out.started = false;
	//Set output flags
	out.output[Output::Element::Motor0DutyCycle] = true;
	out.output[Output::Element::Motor1DutyCycle] = true;
	out.output[Output::Element::Servo0Position] = true;
	out.output[Output::Element::Servo1Position] = true;
	out.output[Output::Element::Started] = true;
	return out;
}

Run::Task *Run::Idle::complete(Input const &input)
{
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
	//If countdown has finished, move onto next state
	if(countdown == 0)
		return new (pvPortMalloc(sizeof(Engage))) Engage(input, Engage::Servo::Both, Engage::PostTask::Startup);
	previousOPState = input.opState;
	return nullptr;
}

void Run::Idle::displayUpdate(Display &disp)
{
	disp.topline.reset(new (pvPortMalloc(sizeof(DL_Idle_Top))) DL_Idle_Top(countdown));
	disp.bottomline.reset(new (pvPortMalloc(sizeof(DL_Battery))) DL_Battery);
}

Run::Output Run::Engage::update(Input const &input)
{
	Output out;
	if((servo == Engage::Servo::Servo0) || (servo == Engage::Servo::Both)) {
		out.servo0Position = f0.currentValue(input.time);
		out.servo0Power = true;
		out.output[Output::Element::Servo0Position] = true;
		out.output[Output::Element::Servo0Power] = true;
	}
	if((servo == Engage::Servo::Servo1) || (servo == Engage::Servo::Both)) {
		out.servo1Position = f1.currentValue(input.time);
		out.servo1Power = true;
		out.output[Output::Element::Servo1Position] = true;
		out.output[Output::Element::Servo1Power] = true;
	}
	return out;
}

Run::Task * Run::Engage::complete(Input const &input)
{
	if(input.time >= f0.get_time_destination()) {
		if(post == PostTask::Startup)
			return new (pvPortMalloc(sizeof(Startup))) Startup(input);
		else
			return new (pvPortMalloc(sizeof(CoastRamp))) CoastRamp(input);
	}
	return nullptr;
}

Run::Engage::Engage(Input const &input, Servo const servo, PostTask const post) : Task(TaskIdentity::Engage), servo(servo), post(post)
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
	f0.startup(config::run::servo0restposition, config::run::servo0engagedposition, input.time, input.time + config::run::servoengagetime);
	f1.startup(config::run::servo1restposition, config::run::servo1engagedposition, input.time, input.time + config::run::servoengagetime);
}

Run::Output Run::Startup::update(Input const &input)
{
	Output out;
	out.motor0DutyCycle = f.currentValue(input.time);
	out.motor1DutyCycle = f.currentValue(input.time);
	if(input.vehicleSpeed >= config::run::maximumspeed) {
		out.motor0DutyCycle = 0;
		out.motor1DutyCycle = 0;
	}
	out.output.set(Output::Element::Motor0DutyCycle);
	out.output.set(Output::Element::Motor1DutyCycle);
	out.started = true;
	out.output.set(Output::Element::Started);
	return out;
}

Run::Task * Run::Startup::complete(Input const &input)
{
	if(input.vehicleSpeed >= config::run::maximumspeed)
		return new (pvPortMalloc(sizeof(Disengage))) Disengage(input, Disengage::Servo::Both);
	return nullptr;
}

void Run::Startup::displayUpdate(Display &disp)
{
	disp.topline.reset(new (pvPortMalloc(sizeof(DL_SpeedEnergy))) DL_SpeedEnergy);
	disp.bottomline.reset(new (pvPortMalloc(sizeof(DL_Ramping))) DL_Ramping);
}

 Run::Startup::Startup(Input const &input) : Task(TaskIdentity::Startup)
{
	f.startup(0, 1, input.time, input.time + config::run::ramuptime);
}

Run::Output Run::Disengage::update(Input const &input)
{
	//Literally the same thing as Engage, maybe should just make the same class?
	Output out;
	if((servo == Disengage::Servo::Servo0) || (servo == Disengage::Servo::Both)) {
		out.servo0Position = f0.currentValue(input.time);
		out.servo0Power = true;
		out.output[Output::Element::Servo0Position] = true;
		out.output[Output::Element::Servo0Power] = true;
	}
	if((servo == Disengage::Servo::Servo1) || (servo == Disengage::Servo::Both)) {
		out.servo1Position = f1.currentValue(input.time);
		out.servo1Power = true;
		out.output[Output::Element::Servo1Position] = true;
		out.output[Output::Element::Servo1Power] = true;
	}
	return out;
}

Run::Task * Run::Disengage::complete(Input const &input)
{
	//Also basically the same as engage
	if(input.time >= f0.get_time_destination() + 0.1f)
		return new (pvPortMalloc(sizeof(Coast))) Coast;
	return nullptr;
}

 Run::Disengage::Disengage(Input const &input, Servo const servo) : Task(TaskIdentity::Disengage), servo(servo)
{
	f0.startup(config::run::servo0engagedposition, config::run::servo0restposition, input.time, input.time + config::run::servodisengagetime);
	f1.startup(config::run::servo1engagedposition, config::run::servo1restposition, input.time, input.time + config::run::servodisengagetime);
}

 Run::Coast::Coast() : Task(TaskIdentity::Coast)
{
}

Run::Output Run::Coast::update(Input const &input)
{
	//Stop servo power
	Output out;
	out.servo0Power = false;
	out.servo1Power = false;
	out.output[Output::Element::Servo0Power] = true;
	out.output[Output::Element::Servo1Power] = true;
	return out;
}

Run::Task * Run::Coast::complete(Input const &input)
{
	if(input.vehicleSpeed <= config::run::speedMatchSpeed)
		return new (pvPortMalloc(sizeof(SpeedMatch))) SpeedMatch(input);
	return nullptr;
}

void Run::Coast::displayUpdate(Display &disp)
{
	disp.bottomline.reset(new (pvPortMalloc(sizeof(DL_Coasting))) DL_Coasting);
}

Run::Output Run::CoastRamp::update(Input const &input)
{
	Output out;
	out.motor0DutyCycle = f.currentValue(input.time);
	out.output[Output::Element::Motor0DutyCycle] = true;
	if(input.vehicleSpeed >= config::run::maximumspeed)
		out.motor0DutyCycle = 0;
	return out;
}

Run::Task * Run::CoastRamp::complete(Input const &input)
{
	if(input.vehicleSpeed >= config::run::maximumspeed)
		return new(pvPortMalloc(sizeof(Disengage))) Disengage(input, Disengage::Servo::Servo0);
	return nullptr;
}

void Run::CoastRamp::displayUpdate(Display &disp)
{
	disp.bottomline.reset(new (pvPortMalloc(sizeof(DL_Ramping))) DL_Ramping);
}

 Run::CoastRamp::CoastRamp() : Task(TaskIdentity::CoastRamp)
{
}

 Run::CoastRamp::CoastRamp(Input const &input)
{
	f.startup(0, 1, input.time, input.time + config::run::coastramptime);
}

Run::Output Run::SpeedMatch::update(Input const &input)
{
	Output out;
	if(checkComplete(input))
		out.motor0DutyCycle = 0;
	else {
		out.motor0DutyCycle = std::min(
		std::max(static_cast<float>(-(2.0f / M_PI) * std::atan((input.motor0Speed - input.driveSpeed) / 500.0f)), 0.0f),
			f.currentValue(input.time));
	}
	out.output[Output::Element::Motor0DutyCycle] = true;
	
	return out;
}

Run::Task * Run::SpeedMatch::complete(Input const &input)
{
	if(checkComplete(input))
		return new(pvPortMalloc(sizeof(Engage))) Engage(input, Engage::Servo::Servo0, Engage::PostTask::CoastRamp);
	return nullptr;
}

 Run::SpeedMatch::SpeedMatch(Input const &input) : Task(TaskIdentity::SpeedMatch)
{
	f.startup(0, 1, input.time, input.time + config::run::matchramptime);
}

bool Run::SpeedMatch::checkComplete(Input const &input)
{
	if(completed) return true;
	if(input.vehicleSpeed < config::run::minimumspeed) {
		completed = true;
		return true;
	}
	if(withinError(input.motor0Speed, input.driveSpeed, config::run::errorrange)) {
		if((input.time - correctEntry) >= config::run::speedmatchcorrecttimeout) {
			completed = true;
			return true;
		}
	}
	else
		correctEntry = input.time;
	return false;
}

Run::Output Run::OPCheck::update(Input const &input)
{
	Output out;
	if(input.started && !finished) {
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
	previousOPState = input.opState;
	return out;
}

Run::Task * Run::OPCheck::complete(Input const &input)
{
	if((!input.opState) && ((input.time) - errorTime >= config::run::stoptimeout) && input.started && !finished) {
		finished = true;
		return new(pvPortMalloc(sizeof(Finished))) Finished(input.startTime, input.totalEnergyUsage, input.startDistance);
	}
	return nullptr;
}

 Run::OPCheck::OPCheck() : Task(TaskIdentity::OPCheck)
{
	if((sem_buzzerComplete = xSemaphoreCreateBinary()) == NULL)
		debugbreak();
}

Run::Output Run::BatteryCheck::update(Input const &input)
{
	Output out;
	//if(input.bms0data.current > config::run::alertbmscurrent) {
	//	out.motor0DutyCycle = 0;
	//	out.motor1DutyCycle = 0;
	//	//Turn on the BMS led
	//	runtime::bms0->setLEDState(true);
	//}
	//if(input.bms0data.current > config::run::alertbmscurrent) {
	//	//Alert the user
	//	runtime::vbBuzzermanager->registerSequence([](Buzzer &buz) {
	//		for(unsigned int i = 0; i < 6; i++) {
	//			buz.start();
	//			vTaskDelay(msToTicks(50));
	//			buz.stop();
	//			vTaskDelay(msToTicks(50));
	//		}
	//	});
	//}
	return out;
}

Run::Task * Run::BatteryCheck::complete(Input const &input)
{
	return nullptr;
}

Run::Output Run::MotorCheck::update(Input const &input)
{
	return {};
}

Run::Task * Run::MotorCheck::complete(Input const &input)
{
	return nullptr;
}

void Run::Display::printDisplay(Input const &input) const
{
	char str[17];
	memset(str, 0, 17);
	runtime::viewerboard->setPosition(0);
	topline->get_text(str, input);
	runtime::viewerboard->writeText(str, 16);
	memset(str, 0, 17);
	runtime::viewerboard->setPosition(40);
	bottomline->get_text(str, input);
	runtime::viewerboard->writeText(str, 16);
	runtime::viewerboard->send();
}

void callback_tcc2_compare_speedmatch_update(tcc_module	*const module) {
	//static unsigned int cycle = 0;
	//if(cycle == 0)
		runtime::motor0->setDutyCycle(std::max(static_cast<float>(-(2.0f / M_PI) * std::atan((runtime::encoder0->getSpeed() - runtime::encoder2->getSpeed()) / 500.0f)), 0.0f));
	//cycle = (++cycle == 2) ? 0 : cycle;
}

void Run::init()
{
	tcc_set_compare_value(&tccEncoder::instance, TCC_MATCH_CAPTURE_CHANNEL_0, 0x0aff);
	tcc_register_callback(&tccEncoder::instance, callback_tcc2_compare_speedmatch_update, TCC_CALLBACK_CHANNEL_0);
}

 Run::Finished::Finished(float const time, float const energy, float const distance) : time(time), energy(energy), distance(distance) {}

Run::Output Run::Finished::update(Input const &input)
{
	Output out;
	setoutputDefaults(out);
	return out;
}

Run::Task * Run::Finished::complete(Input const &input)
{
	return nullptr;
}

void Run::Finished::displayUpdate(Display &disp)
{
	disp.topline.reset(new (pvPortMalloc(sizeof(DL_Finished))) DL_Finished(time, energy, distance));
	disp.bottomline.reset(new (pvPortMalloc(sizeof(DL_Battery))) DL_Battery);
}
