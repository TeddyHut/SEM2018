/*
 * runmanagement.cpp
 *
 * Created: 25/01/2018 7:07:37 PM
 *  Author: teddy
 */ 

#include "runmanagement.h"
#include "run.h"
#include "instance.h"
#include "dep_instance.h"
#include <rtc_count.h>
#include <cmath>
#include <bitset>
#include <task.h>

using namespace Run;

constexpr float driveIntervalToMs(float const interval) {
	float const sec_interval = (interval / config::motor::clockFrequency);
	return (config::hardware::wheelradius * 2 * M_PI * sec_interval) * 8;
}

//Prevents noise/button hardware error on the OP presence button
class ButtonBuffer {
public:
	unsigned int pos = 0;
	std::bitset<config::run::buttonBufferSize> buf;
	bool pressed(bool const state) {
		//All values in buffer must be clear to register it being released
		buf.set(pos++, state);
		if(pos >= buf.size())
			pos = 0;
		return buf.any();
	}
};

rtc_module rtc_instance;

void initRTC() {
	rtc_count_config config;
	rtc_count_get_config_defaults(&config);
	config.clear_on_match = false;
	config.continuously_update = true;
	config.mode = RTC_COUNT_MODE_32BIT;
	//By default is clocked by the OSC32, div by 32 for 1024
	config.prescaler = RTC_COUNT_PRESCALER_DIV_32;
	rtc_count_init(&rtc_instance, RTC, &config);
	rtc_count_set_count(&rtc_instance, 0);
	rtc_count_enable(&rtc_instance);
}

void fillInput(Input &input, Output const &prevOutput) {
	static ButtonBuffer opBuffer;
	//Get difference in time since last cycle
	double timeDifference = (rtc_count_get_count(&rtc_instance) / 1024.0f) - input.time;
	input.time += timeDifference;

	//Time based calculations
	input.distance += input.vehicleSpeed * timeDifference;
	//Energy in W/s (joules) / 3600 to get W/h
	input.totalEnergyUsage += (((input.bms0data.current * input.bms0data.voltage) + (input.bms1data.current * input.bms1data.voltage)) * timeDifference) / 3600.0f;
	input.motor0EnergyUsage += (input.motor0Current * input.voltage * timeDifference) / 3600.0f;
	input.motor1EnergyUsage += (input.motor1Current * input.voltage * timeDifference) / 3600.0f;

	//From previous
	input.motor0DutyCycle = prevOutput.motor0DutyCycle;
	input.motor1DutyCycle = prevOutput.motor1DutyCycle;
	input.servo0Position = prevOutput.servo0Position;
	input.servo1Position = prevOutput.servo1Position;
	
	//From sensors
	input.bms0data = runtime::bms0->get_data();
	input.bms1data = runtime::bms1->get_data();
	input.motor0Speed = runtime::encoder0->getSpeed();
	input.motor1Speed = runtime::encoder1->getSpeed();
	input.driveSpeed = runtime::encoder2->getSpeed();
	input.opState = opBuffer.pressed(!runtime::opPresence->state());

	//Sensor calculations
	input.voltage = input.bms0data.voltage + input.bms1data.voltage;
	input.vehicleSpeed = driveIntervalToMs(runtime::encoder2->getSpeed());
	//input.vehicleSpeed = msToKmh(driveIntervalToMs(runtime::encoder2->getAverageInterval()));
}

void processOutput(Output const &output) {
	if(output.output[Output::Element::Motor0DutyCycle])
		runtime::motor0->setDutyCycle(output.motor0DutyCycle);
	if(output.output[Output::Element::Motor1DutyCycle])
		runtime::motor1->setDutyCycle(output.motor1DutyCycle);
	if(output.output[Output::Element::Servo0Position])
		runtime::servo0->setPosition(output.servo0Position);
	if(output.output[Output::Element::Servo1Position])
		runtime::servo1->setPosition(output.servo1Position);
}

void mergeOutput(Output &dest, Output const &src) {
	if(src.output[Output::Element::Motor0DutyCycle])
		dest.motor0DutyCycle = src.motor0DutyCycle;
	if(src.output[Output::Element::Motor1DutyCycle])
		dest.motor1DutyCycle = src.motor1DutyCycle;
	if(src.output[Output::Element::Servo0Position])
		dest.servo0Position = src.servo0Position;
	if(src.output[Output::Element::Servo1Position])
		dest.servo1Position = src.servo1Position;
	dest.output |= src.output;
}

void runmanagement::run()
{
	initRTC();
	Display disp;
	Input input;
	Output output;
	Task *currentTask = new (pvPortMalloc(sizeof(Idle))) Idle;
	Task *opCheck = new (pvPortMalloc(sizeof(OPCheck))) OPCheck;
	Task *batteryCheck = new (pvPortMalloc(sizeof(BatteryCheck))) BatteryCheck;
	Task *motorCheck = new (pvPortMalloc(sizeof(MotorCheck))) MotorCheck;

	currentTask->displayUpdate(disp);
	TickType_t previousWakeTime = xTaskGetTickCount();
	TickType_t displayUpdateTime = xTaskGetTickCount();
	while(true) {
		fillInput(input, output);
		mergeOutput(output, currentTask->update(input));
		Task *returnTask = currentTask->complete(input);
		//Current task should be changed
		if(returnTask != nullptr) {
			currentTask->~Task();
			vPortFree(currentTask);
			currentTask = returnTask;
			currentTask->displayUpdate(disp);
		}
		//Perform overriding checks
		for(auto &&element : {opCheck, batteryCheck, motorCheck}) {
			mergeOutput(output, element->update(input));
			element->displayUpdate(disp);
		}
		processOutput(output);
		//Reset change flags
		output.output.reset();
		//Print display
		if(xTaskGetTickCount() - displayUpdateTime >= config::run::displayrefreshrate) {
			displayUpdateTime = xTaskGetTickCount();
			disp.printDisplay(input);
		}
		//Set an external LED if the person isn't holding the OP
		runtime::greenLED->setLEDState(!input.opState);
		vTaskDelayUntil(&previousWakeTime, config::run::refreshRate);
	}
}
