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

using namespace Run;

constexpr float current_opamp_transformation_outV_motor(float const inV, float const vcc) {
	return (inV * ((5.1 * (2 * (1 + 5.1) + 10)) / ((1 + 5.1) * (2 * 1 + 10)))) - (vcc * (5.1 / (2 * 1 + 10)));
}

constexpr float current_opamp_transformation_inV_motor(float const outV, float const vcc) {
	return (outV + (vcc * (5.1 / (2 * 1 + 10)))) / ((5.1 * (2 * (1 + 5.1) + 10)) / ((1 + 5.1) * (2 * 1 + 10)));
}

constexpr float driveIntervalToMs(float const interval) {
	if(interval <= 0)
		return 0;
	float const sec_interval = (interval / config::motor::clockFrequency);
	//Speed = distance / time
	//Distance = circumference / 8
	//Time = sec_interval
	return ((config::hardware::wheelradius * 2 * M_PI) / 8) / sec_interval;
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

enum class MotorIndex {
	Motor0,
	Motor1,
};

float get_motor_current(MotorIndex const index) {
	if(index == MotorIndex::Motor0) {
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

void fillInput(Input &input, Output const &prevOutput) {
	static ButtonBuffer opBuffer;
	//Get difference in time since last cycle
	float timeDifference = (rtc_count_get_count(&rtc_instance) / 1024.0f) - input.time;
	input.time += timeDifference;

	//Time based calculations
	input.distance += input.vehicleSpeed * timeDifference;
	//Energy in W/s (joules) / 3600 to get W/h
	input.totalEnergyUsage += (((input.bms0data.current * input.bms0data.voltage) + (input.bms1data.current * input.bms1data.voltage)) * timeDifference) / 3600.0f;
	//This for when only one BMS is working
	//input.totalEnergyUsage += (((input.bms1data.current * input.bms1data.voltage) + (input.bms1data.current * input.bms1data.voltage)) * timeDifference) / 3600.0f;
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
	//input.motor0Current = get_motor_current(MotorIndex::Motor0);
	//input.motor1Current = get_motor_current(MotorIndex::Motor1);
	input.servo0Current = runtime::sensor_servo0_current->value();
	input.servo1Current = runtime::sensor_servo1_current->value();
	input.servo0Voltage = runtime::sensor_servo0_voltage->value();
	input.servo1Voltage = runtime::sensor_servo1_voltage->value();
	input.v3v3Current   = runtime::sensor_3v3_current->value();
	input.v3v3Voltage   = runtime::sensor_3v3_current->value();
	input.v5Current     = runtime::sensor_5v_current->value();
	input.v5Voltage     = runtime::sensor_5v_voltage->value();

	input.driveSpeed = runtime::encoder2->getSpeed();
	input.opState = opBuffer.pressed(!runtime::opPresence->state());

	//Sensor calculations
	input.voltage = input.bms0data.voltage + input.bms1data.voltage;
	input.vehicleSpeed = driveIntervalToMs(runtime::encoder2->getAverageInterval());
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
	if(output.output[Output::Element::Servo0Power])
		port_pin_set_output_level(config::servopower::servo0_power_pin, output.servo0Power);
	if(output.output[Output::Element::Servo1Power])
		port_pin_set_output_level(config::servopower::servo1_power_pin, output.servo1Power);

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
	if(src.output[Output::Element::Servo0Power])
		dest.servo0Power = src.servo0Power;
	if(src.output[Output::Element::Servo1Power])
		dest.servo1Power = src.servo1Power;
	dest.output |= src.output;
}

void runmanagement::run()
{
	initRTC();
	initADC();
	Run::init();
	Display disp;
	Input input;
	Output output;
	Task *currentTask = new (pvPortMalloc(sizeof(Idle))) Idle;
	Task *opCheck = new (pvPortMalloc(sizeof(OPCheck))) OPCheck;
	Task *batteryCheck = new (pvPortMalloc(sizeof(BatteryCheck))) BatteryCheck;
	Task *motorCheck = new (pvPortMalloc(sizeof(MotorCheck))) MotorCheck;

#if (ENABLE_ADPCONTROL == 1)
	ADPControl adpcontrol;
	adpcontrol.init();
#endif

	currentTask->displayUpdate(disp);
	TickType_t previousWakeTime = xTaskGetTickCount();
	TickType_t displayUpdateTime = xTaskGetTickCount();
	while(true) {
		fillInput(input, output);
		input.currentID = currentTask->id;
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
#if (ENABLE_MANUALSERIAL == 1)
		if(currentTask->id != TaskIdentity::Idle)
#endif	
		processOutput(output);
		//Reset change flags
		output.output.reset();
		//Print display
		if(xTaskGetTickCount() - displayUpdateTime >= config::run::displayrefreshrate) {
			displayUpdateTime = xTaskGetTickCount();
			disp.printDisplay(input);
#if (ENABLE_ADPCONTROL == 1)
			mergeOutput(output, adpcontrol.update(input));
#endif
		}
		//Set an external LED if the person isn't holding the OP
		runtime::greenLED->setLEDState(!input.opState);
		vTaskDelayUntil(&previousWakeTime, config::run::refreshRate);
	}
}