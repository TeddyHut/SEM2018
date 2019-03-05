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
#include "adpcontrol.h"
#include "util.h"
#include <rtc_count.h>
#include <cmath>
#include <bitset>
#include <task.h>
#include <adc.h>

using namespace program;

constexpr float current_opamp_transformation_outV_motor(float const inV, float const vcc) {
	return (inV * ((5.1 * (2 * (1 + 5.1) + 10)) / ((1 + 5.1) * (2 * 1 + 10)))) - (vcc * (5.1 / (2 * 1 + 10)));
}

constexpr float current_opamp_transformation_inV_motor(float const outV, float const vcc) {
	return (outV + (vcc * (5.1 / (2 * 1 + 10)))) / ((5.1 * (2 * (1 + 5.1) + 10)) / ((1 + 5.1) * (2 * 1 + 10)));
}

float driveIntervalToMs(float const interval) {
	if(interval <= 0)
		return 0;
	float const sec_interval = (interval / config::motor::clockFrequency);
	//Speed = distance / time
	//Distance = circumference / 8
	//Time = sec_interval
	return ((runtime::usbmsc->settings.wheelRadius * 2 * M_PI) / runtime::usbmsc->settings.wheelSamplePoints) / sec_interval;
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

adc_module adc_instance;

void initADC() {
	adc_config config;
	adc_get_config_defaults(&config);
	config.clock_source = GCLK_GENERATOR_1;
	config.clock_prescaler = ADC_CLOCK_PRESCALER_DIV8;
	config.reference = ADC_REFERENCE_INTVCC1;
	config.resolution = ADC_RESOLUTION_12BIT;
	config.gain_factor = ADC_GAIN_FACTOR_1X;
	config.negative_input = ADC_NEGATIVE_INPUT_GND;
	//config.accumulate_samples = ADC_ACCUMULATE_SAMPLES_4;
	//config.divide_result = ADC_DIVIDE_RESULT_4;
	config.left_adjust = false;
	config.differential_mode = false;
	config.freerunning = false;
	config.reference_compensation_enable = false;
	config.sample_length = 16;
	adc_init(&adc_instance, ADC, &config);
	adc_enable(&adc_instance);
}

/*
float get_motor_current(MotorIndex const index) {
	if(index == MotorIndex::motor) {
		adc_set_positive_input(&adc_instance, ADC_POSITIVE_INPUT_PIN1);
	}
	else if (index == MotorIndex::Motor1) {
		adc_set_positive_input(&adc_instance, ADC_POSITIVE_INPUT_PIN0);
	}
	adc_start_conversion(&adc_instance);
	uint16_t result = 0;
	while(adc_read(&adc_instance, &result) == STATUS_BUSY);
	volatile float opamp_outV = adcutil::adc_voltage<uint16_t, 4096>(result, (1.0f / 1.48f) * 3.3f);
	volatile float opamp_inV = current_opamp_transformation_inV_motor(opamp_outV, 3.3f);
	volatile float current = acs711_current(opamp_inV, 3.3f);
	//return current;
	return 0;	
}
*/

void fillInput(Input &input, Output const &prevOutput) {
	static ButtonBuffer opBuffer;
	static float logTimeout = 0;
	static float syncTimeout = 0;
	static bool previousStarted = false;

	//Reset states
	input.logCycle = false;

	//Determine whether device started
	if(prevOutput.output[Output::Element::State])
		input.programstate = prevOutput.programstate;

	//If finished, reset for next time
	if(input.programstate != Input::State::Running) {
		logTimeout = 0;
		syncTimeout = 0;
		previousStarted = false;
		input.totalEnergyUsage = 0;
		input.startDistance = 0;
		input.startTime = 0;
	}

	//Get difference in time since last cycle
	float timeDifference = (rtc_count_get_count(&rtc_instance) / 1024.0f) - input.time;
	input.time += timeDifference;

	//Time based calculations
	float distanceDifference = input.vehicleSpeedMs * timeDifference;
	input.distance += distanceDifference;

	//If has started, add calculations to started values
	if(input.programstate == Input::State::Running) {
		//Always log if just started
		if(previousStarted == false) {
			previousStarted = true;
			input.logCycle = runtime::usbmsc->isReady();
		}
		input.startTime += timeDifference;
		input.startDistance += distanceDifference;

		if((logTimeout += timeDifference) >= 1.0f / runtime::usbmsc->settings.sampleFrequency) {
			logTimeout -= (1.0f / runtime::usbmsc->settings.sampleFrequency);
			//Only log of the USB is still connected
			input.logCycle = runtime::usbmsc->isReady();
			input.samples++;
		}
		if((syncTimeout += timeDifference) >= config::run::syncinterval) {
			syncTimeout -= config::run::syncinterval;
			//Only sync if USB is still connected
			if(runtime::usbmsc->isReady())
				f_sync(&runtime::usbmsc->file);
		}

		//Energy in W/s (joules) / 3600 to get W/h
		//input.totalEnergyUsage += (((input.bms0data.current * input.bms0data.voltage) + (input.bms1data.current * input.bms1data.voltage)) * timeDifference) / 3600.0f;
		input.totalEnergyUsage += input.calculationCurrent * input.voltage * timeDifference / 3600.0f;
	}
	else {
		input.samples = 0;
	}

	//This for when only one BMS is working
	//input.totalEnergyUsage += (((input.bms1data.current * input.bms1data.voltage) + (input.bms1data.current * input.bms1data.voltage)) * timeDifference) / 3600.0f;
	input.motorEnergyUsage += (input.motorCurrent * input.voltage * timeDifference) / 3600.0f;

	//From previous
	input.motorDutyCycle = prevOutput.motorDutyCycle;
	input.motorPWMFrequency = prevOutput.motorPWMFrequency;
	
	//From sensors
	input.bms0data = runtime::bms0->get_data();
	input.bms1data = runtime::bms1->get_data();
	input.motorRPS = runtime::encoder0->getSpeed();
	input.motorTicks = runtime::encoder0->getSampleTotal();

	input.wheelRPS = runtime::encoder0->getSpeed();
	input.wheelTicks = runtime::encoder0->getSampleTotal();

	input.opState = opBuffer.pressed(!runtime::opPresence->state());

	//Sensor calculations
	input.voltage = input.bms0data.voltage + input.bms1data.voltage;
	input.calculationCurrent = std::max(input.bms0data.current, 0.0f);
	input.vehicleSpeedMs = driveIntervalToMs(runtime::encoder0->getAverageInterval());
}

void processOutput(Output const &output) {
	if(output.output[Output::Element::MotorDutyCycle])
		runtime::motor0->setDutyCycle(output.motorDutyCycle);
	if(output.output[Output::Element::MotorPWMFrequency])
		runtime::motor0->setPeriod(1.0f / output.motorPWMFrequency);
}

void mergeOutput(Output &dest, Output const &src) {
	if(src.output[Output::Element::MotorDutyCycle])
		dest.motorDutyCycle = src.motorDutyCycle;
	if(src.output[Output::Element::MotorPWMFrequency])
		dest.motorPWMFrequency = src.motorPWMFrequency;
	if(src.output[Output::Element::State])
		dest.programstate = src.programstate;
	dest.output |= src.output;
}

void makeLogEntries(Input const &input, Task *const currenttask) {
	if(input.programstate == Input::State::Running && input.logCycle) {
		char strmode[16];
		switch(currenttask->id) {
		default:
			strcpy(strmode, "Unknown");
			break;
		case TaskIdentity::Startup:
			strcpy(strmode, "Startup");
			break;
		case TaskIdentity::Coast:
			strcpy(strmode, "Coast");
			break;
		case TaskIdentity::CoastRamp:
			strcpy(strmode, "CoastRamp");
			break;
		case TaskIdentity::Finished:
			strcpy(strmode, "Finished");
			break;
		}
		char str[196];
		rpl_snprintf(str, sizeof str, "%u,%.3f,%s,%.3f,%4.1f,%4.1f,%4.1f,%4.1f,%.3f,%.3f,%.2f,%.3f,%.2f,%u,\n",
			input.samples, //Samples
			input.startTime, //Runtime
			strmode, //Mode
			input.calculationCurrent, //Current
			input.bms0data.voltage, //Bat0 voltage
			input.bms1data.voltage, //Bat1 voltage
			input.bms0data.temperature, //Bat0 temp
			input.bms1data.temperature, //Bat1 temp
			input.calculationCurrent * (input.bms0data.voltage + input.bms1data.voltage), //Power (W) (V*I)
			input.totalEnergyUsage * 3600.0f, //Total energy usage is in Wh, convert to J
			input.motorDutyCycle, //MotorDuty
			input.vehicleSpeedMs, //Velocity
			input.startDistance, //Distance
			input.opState ? 1 : 0 //OP
			);
		f_puts(str, &runtime::usbmsc->file);
	}
}

void runmanagement::run()
{
	initRTC();
	//initADC();
	//Run::init();
	Display disp;
	Input input;
	Output output;

	Task *currentTask = new (pvPortMalloc(sizeof(Idle))) Idle;
	OPCheck *opCheck = new (pvPortMalloc(sizeof(OPCheck))) OPCheck;

	auto allocateNewTask = [&](Task *const returnTask) {
		currentTask->~Task();
		vPortFree(currentTask);
		currentTask = returnTask;
	};
	currentTask->displayUpdate(disp);

	TickType_t previousWakeTime = xTaskGetTickCount();
	TickType_t displayUpdateTime = xTaskGetTickCount();
	while(true) {
		fillInput(input, output);
		//Reset change flags
		output.output.reset();

		input.currentID = currentTask->id;
		mergeOutput(output, currentTask->update(input));
		Task *returnTask = currentTask->complete(input);
		//Current task should be changed
		if(returnTask != nullptr) {
			allocateNewTask(returnTask);
		}
		//Perform overriding checks
		for(auto &&element : {opCheck}) {
			mergeOutput(output, element->update(input));
			auto returnTask = element->complete(input);
			if(returnTask != nullptr) {
				allocateNewTask(returnTask);
			element->displayUpdate(disp);
			}
		}
		if(opCheck->changedProgramState()) {
			switch(output.programstate) {
			case Input::State::Idle:
				break;
			case Input::State::Running:
				break;
			case Input::State::Finished:
				allocateNewTask(new(pvPortMalloc(sizeof(Finished))) Finished(input.startTime, input.totalEnergyUsage, input.startDistance));
				break;
			}
		}
		currentTask->displayUpdate(disp);

		makeLogEntries(input, currentTask);
		
		processOutput(output);
		//Print display
		if(xTaskGetTickCount() - displayUpdateTime >= config::run::displayrefreshrate) {
			displayUpdateTime = xTaskGetTickCount();
			disp.printDisplay(input);
		}
		vTaskDelayUntil(&previousWakeTime, config::run::refreshRate);
	}
}
