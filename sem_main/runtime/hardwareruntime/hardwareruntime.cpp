/*
 * instance.cpp
 *
 * Created: 20/01/2018 4:31:50 PM
 *  Author: teddy
 */

#include "hardwareruntime.h"

#include "../../hardware/twimanager/twimanager.h"
#include "../../hardware/spimanager/spimanager.h"
#include "../../hardware/adcmanager/adcmanager.h"
#include "../../hardware/encoder/encoder.h"
#include "../../hardware/motor/motor.h"
#include "../../hardware/emc1701/emc1701.h"
#include "../../hardware/bms/bms.h"
#include "../../hardware/buzzer/buzzer.h"
#include "../../hardware/servo/servo.h"
#include "../../hardware/motorcurrent/motorcurrent.h"
#include "../../hardware/rtc/rtc.h"
#include "../../hardware/gpiooutput/gpiooutput.h"
#include "../../hardware/gpioinput/gpioinput.h"

extern "C" {
	void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName) {
		volatile signed char *volatile name = pcTaskName;
		//Error
	}
}

using GPIOPin_OP_t = GPIOPinHW<PIN_PA05A_EIC_EXTINT5, MUX_PA05A_EIC_EXTINT5, 5>;

//Pointers
Sensor<float>    *hardwareruntime::sensor_rtc_time;
Sensor<float>    *hardwareruntime::sensor_current_motor       [2];
Sensor<float>    *hardwareruntime::sensor_current_servo       [2];
Sensor<float>    *hardwareruntime::sensor_voltage_servo       [2];
Sensor<float>    *hardwareruntime::sensor_voltage_batteryCell [2][6];
Sensor<float>    *hardwareruntime::sensor_current_battery     [2];
Sensor<float>    *hardwareruntime::sensor_temperature_battery [2];
Sensor<float>    *hardwareruntime::sensor_speed_motor         [2];
Sensor<float>    *hardwareruntime::sensor_speed_wheel;
Sensor<bool>     *hardwareruntime::sensor_op_state;
ViewerBoard      *hardwareruntime::viewerboard_driver;
ValueItem<float> *hardwareruntime::motor                      [2];
ValueItem<float> *hardwareruntime::servo                      [2];
ValueItem<bool>  *hardwareruntime::servopower                 [2];
ValueItem<bool>  *hardwareruntime::mainbuzzer;
ValueItem<bool>  *hardwareruntime::redled;
ValueItem<bool>  *hardwareruntime::greenled;
ValueItem<bool>  *hardwareruntime::viewerboardnotifyled;
ValueItem<bool>  *hardwareruntime::viewerboardbacklightled;
ValueItem<bool>  *hardwareruntime::viewerboardbuzzer;

//Static
RTCCount                 sensor_rtc_time;
MotorCurrent             sensor_current_motor       [2];
emc1701::sensor::Current sensor_current_servo       [2];
emc1701::sensor::Voltage sensor_voltage_servo       [2];
bms::sensor::CellVoltage sensor_voltage_batteryCell [2][6];
bms::sensor::Current     sensor_current_battery     [2];
bms::sensor::Temperature sensor_temperature_battery [2];
encoder::sensor::Speed   sensor_speed_motor         [2];
encoder::sensor::Speed   sensor_speed_wheel;
gpiopin::sensor::State   sensor_op_state;
ViewerBoard      viewerboard_driver;

TCMotor0 motor0;
TCMotor1 motor1;
TCCServo0 servo0;
TCCServo1 servo1;
GPIOOutput servopower[2];
Buzzer0 mainbuzzer;
GPIOOutput redled;
GPIOOutput greenled;
viewerboard::NotifyLED viewerboardnotifyled;
viewerboard::BacklightLED viewerboardbacklightled;
viewerboard::Buzzer viewerboardbuzzer;

TWIManager0 twimanager_main;
SPIManager0 spimanager_main;
ADCManager adcmanager_main;
EMC1701 emc1701_servo[2];
BMS bms_battery[2];
Encoder0 encoder_motor0(encoder::motor_speedConvert);
Encoder1 encoder_motor1(encoder::motor_speedConvert);
Encoder2 encoder_wheel(encoder::motor_driveTrainConvert);
GPIOPin_OP_t gpiopin_op;

void emc1701_config() {
	using namespace emc1701;
	emc1701::Config conf;
	conf.config_maskAll						    = true;
	conf.config_temperatureDisabled			    = false;
	conf.config_alertComparatorModeEnabled	    = false;
	conf.config_voltageMeasurementDisabled	    = false;
	conf.channelMask_Vsense					    = false;
	conf.channelMask_Vsrc					    = false;
	conf.channelMask_peak					    = false;
	conf.channelMask_temperature			    = false;
	conf.consecutiveAlert_timeout			    = false;
	conf.voltageSamplingConfig_peak_alert_therm = false;
	conf.conversionRate = val_ConversionRate::per125ms;
	conf.consecutiveAlert_therm = val_ConsecutiveAlert::_4;
	conf.consecutiveAlert_alert = val_ConsecutiveAlert::_1;
	conf.voltageQueue = val_VoltageQueue::_1;
	conf.voltageAveraging = val_VoltageAveraging::_1;
	conf.currentQueue = val_CurrentQueue::_1;
	conf.currentAveraging = val_CurrentAveraging::_1;
	conf.currentSampleTime = val_CurrentSampleTime::_82ms;
	conf.currentVoltageRange = val_CurrentVoltageRange::_10mV;
	conf.peakDetectionVoltageThreshold = val_PeakDetectionVoltageThreshold::_85mV;
	conf.peakDetectionDuration = val_PeakDetectionDuration::_1000us;

	conf.highLimit_temperature = 85;
	conf.lowLimit_temperature = -128;
	conf.criticalLimit_temperature = 100;
	conf.criticalHysteresis_temperature = 10;

	conf.highLimit_Vsense = 127;
	conf.lowLimit_Vsense = -127;
	conf.criticalLimit_Vsense = 127;
	conf.criticalHysteresis_Vsense = 0x0a;
	
	conf.highLimit_Vsrc = 0xff;
	conf.lowLimit_Vsrc = 0x00;
	conf.criticalLimit_Vsrc = 0xff;
	conf.criticalHysteresis_Vsrc = 0x0a;
	for(unsigned int i = 0; i < 2; i++) {
		emc1701_servo[i].setConfig(conf);
	}
}

void hardwareruntime::init()
{
	//Set system interrupt priorities (for interrupts that need to use FreeRTOS API)
	//LCD
	system_interrupt_set_priority(SYSTEM_INTERRUPT_MODULE_SERCOM0, SYSTEM_INTERRUPT_PRIORITY_LEVEL_2);
	//TWI Manager
	system_interrupt_set_priority(SYSTEM_INTERRUPT_MODULE_SERCOM1, SYSTEM_INTERRUPT_PRIORITY_LEVEL_2);
	//SPI Manager
	system_interrupt_set_priority(SYSTEM_INTERRUPT_MODULE_SERCOM3, SYSTEM_INTERRUPT_PRIORITY_LEVEL_2);
	//ManualSerial/ADPControl
	system_interrupt_set_priority(SYSTEM_INTERRUPT_MODULE_SERCOM5, SYSTEM_INTERRUPT_PRIORITY_LEVEL_2);
	//Extenal pins
	system_interrupt_set_priority(SYSTEM_INTERRUPT_MODULE_EIC, SYSTEM_INTERRUPT_PRIORITY_LEVEL_2);
	//ADC
	system_interrupt_set_priority(SYSTEM_INTERRUPT_MODULE_ADC, SYSTEM_INTERRUPT_PRIORITY_LEVEL_2);
	//Encoders
	system_interrupt_set_priority(SYSTEM_INTERRUPT_MODULE_EVSYS, SYSTEM_INTERRUPT_PRIORITY_LEVEL_0);
	system_interrupt_set_priority(SYSTEM_INTERRUPT_MODULE_TCC2, SYSTEM_INTERRUPT_PRIORITY_LEVEL_0);

	twimanager_main.init();
	spimanager_main.init();
	adcmanager_main.init();
	::viewerboard_driver.init();

	::emc1701_servo[0].init(&twimanager_main, config::emc1701::address_servo0);
	::emc1701_servo[1].init(&twimanager_main, config::emc1701::address_servo1);
	::bms_battery[0].init(&spimanager_main, config::pins::bms_ss0, config::bms::refreshRate);
	::bms_battery[1].init(&spimanager_main, config::pins::bms_ss1, config::bms::refreshRate);

	//ValueItem assignment to drivers
	::servopower[0].init(config::pins::servopower0);
	::servopower[1].init(config::pins::servopower1);
	::mainbuzzer.init();
	::redled.init(config::pins::redled);
	::greenled.init(config::pins::greenled);
	::viewerboardnotifyled.init(&::viewerboard_driver);
	::viewerboardbacklightled.init(&::viewerboard_driver);
	::viewerboardbuzzer.init(&::viewerboard_driver);
	
	::encoder::init();
	//Sensors assignment to drivers
	::sensor_rtc_time.init();
	::sensor_current_motor[0].init(&adcmanager_main, config::motorcurrent::input_motor0);
	::sensor_current_motor[1].init(&adcmanager_main, config::motorcurrent::input_motor1);
	for(unsigned int i = 0; i < 2; i++) {
		::sensor_current_servo[i].init(&::emc1701_servo[i]);
		::sensor_voltage_servo[i].init(&::emc1701_servo[i]);
		::sensor_current_battery[i].init(&::bms_battery[i]);
		::sensor_temperature_battery[i].init(&::bms_battery[i]);
	}
	for(unsigned int j = 0; j < 2; j++) {
		for(unsigned int i = 0; i < 6; i++) {
			::sensor_voltage_batteryCell[j][i].init(&::bms_battery[j], i);
		}
	}
	::sensor_speed_motor[0].init(&::encoder_motor0);
	::sensor_speed_motor[1].init(&::encoder_motor1);
	::sensor_speed_wheel.init(&encoder_wheel);
	::sensor_op_state.init(&gpiopin_op);

	//ValueItem assignment to pointers
	motor[0] = &::motor0;
	motor[1] = &::motor1;
	servopower[0] = &::servopower[0];
	mainbuzzer = &::mainbuzzer;
	redled = &::redled;
	greenled = &::greenled;
	viewerboardnotifyled = &::viewerboardnotifyled;
	viewerboardbacklightled = &::viewerboardbacklightled;
	viewerboardbuzzer = &::viewerboardbuzzer;

	//Sensors assignment to pointers
	sensor_rtc_time = &::sensor_rtc_time;
	for(unsigned int i = 0; i < 2; i ++) {
		sensor_current_motor[i] = &::sensor_current_motor[i];
		sensor_current_servo[i] = &::sensor_current_servo[i];
		sensor_voltage_servo[i] = &::sensor_voltage_servo[i];
		sensor_current_battery[i] = &::sensor_current_battery[i];
		sensor_temperature_battery[i] = &::sensor_temperature_battery[i];
		sensor_speed_motor[i] = &::sensor_speed_motor[i];
	}
	sensor_speed_wheel = &::sensor_speed_wheel;
	sensor_op_state = &::sensor_op_state;

	emc1701_config();
}
