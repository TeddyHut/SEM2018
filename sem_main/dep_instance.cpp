/*
 * dep_instance.cpp
 *
 * Created: 23/01/2018 10:29:25 PM
 *  Author: teddy
 */ 

#include "dep_instance.h"
#include "instance.h"

BMS           *runtime::bms0;
BMS           *runtime::bms1;
Motor         *runtime::motor0;
Encoder       *runtime::encoder0;
//Encoder       *runtime::encoder1;
//Encoder       *runtime::encoder2;
//ServoF        *runtime::servo0;
//ServoF        *runtime::servo1;
//EMC1701       *runtime::emc1701_servo0;
//EMC1701       *runtime::emc1701_servo1;
//EMC1701       *runtime::emc1701_5v;
//EMC1701       *runtime::emc1701_3v3;
//Sensor<float> *runtime::sensor_servo0_current;
//Sensor<float> *runtime::sensor_servo1_current;
//Sensor<float> *runtime::sensor_servo0_voltage;
//Sensor<float> *runtime::sensor_servo1_voltage;
//Sensor<float> *runtime::sensor_3v3_current;
//Sensor<float> *runtime::sensor_5v_current;
//Sensor<float> *runtime::sensor_3v3_voltage;
//Sensor<float> *runtime::sensor_5v_voltage;

namespace runtime {
	void emc1701_config();
}

void runtime::dep_init()
{
	bmsstatic::init();
	
	bms0                  = new (pvPortMalloc(sizeof(BMS                 ))) BMS;
	bms1                  = new (pvPortMalloc(sizeof(BMS                 ))) BMS;
	motor0                = new (pvPortMalloc(sizeof(TCMotor0            ))) TCMotor0;
	//motor1                = new (pvPortMalloc(sizeof(TCMotor1            ))) TCMotor1;
	encoder0              = new (pvPortMalloc(sizeof(Encoder0            ))) Encoder0(tccEncoder::motor_speedConvert);
	//encoder1              = new (pvPortMalloc(sizeof(Encoder1            ))) Encoder1(tccEncoder::motor_speedConvert);
	//encoder2              = new (pvPortMalloc(sizeof(Encoder2            ))) Encoder2(tccEncoder::motor_driveTrainConvert);
	//servo0                = new (pvPortMalloc(sizeof(TCCServo0           ))) TCCServo0;
	//servo1                = new (pvPortMalloc(sizeof(TCCServo1           ))) TCCServo1;
	//emc1701_servo0        = new (pvPortMalloc(sizeof(EMC1701             ))) EMC1701;
	//emc1701_servo1        = new (pvPortMalloc(sizeof(EMC1701             ))) EMC1701;
	//emc1701_5v            = new (pvPortMalloc(sizeof(EMC1701             ))) EMC1701;
	//emc1701_3v3           = new (pvPortMalloc(sizeof(EMC1701             ))) EMC1701;
	//sensor_servo0_current = new (pvPortMalloc(sizeof(EMC1701CurrentSensor))) EMC1701CurrentSensor;
	//sensor_servo1_current = new (pvPortMalloc(sizeof(EMC1701CurrentSensor))) EMC1701CurrentSensor;
	//sensor_servo0_voltage = new (pvPortMalloc(sizeof(EMC1701VoltageSensor))) EMC1701VoltageSensor;
	//sensor_servo1_voltage = new (pvPortMalloc(sizeof(EMC1701VoltageSensor))) EMC1701VoltageSensor;
	//sensor_3v3_current    = new (pvPortMalloc(sizeof(EMC1701CurrentSensor))) EMC1701CurrentSensor;
	//sensor_5v_current     = new (pvPortMalloc(sizeof(EMC1701CurrentSensor))) EMC1701CurrentSensor;
	//sensor_3v3_voltage    = new (pvPortMalloc(sizeof(EMC1701VoltageSensor))) EMC1701VoltageSensor;
	//sensor_5v_voltage     = new (pvPortMalloc(sizeof(EMC1701VoltageSensor))) EMC1701VoltageSensor;

	bms0                 ->init(runtime::mainspi, config::bms::bms0_ss_pin);
	bms1                 ->init(runtime::mainspi, config::bms::bms1_ss_pin);
	//emc1701_servo0       ->init(runtime::maintwi, config::emc1701::address_servo0);
	//emc1701_servo1       ->init(runtime::maintwi, config::emc1701::address_servo1);
	//emc1701_5v           ->init(runtime::maintwi, config::emc1701::address_5v);
	//emc1701_3v3          ->init(runtime::maintwi, config::emc1701::address_3v3);
	//static_cast<EMC1701CurrentSensor *>(sensor_servo0_current)->init(emc1701_servo0);
	//static_cast<EMC1701CurrentSensor *>(sensor_servo1_current)->init(emc1701_servo1);
	//static_cast<EMC1701VoltageSensor *>(sensor_servo0_voltage)->init(emc1701_servo0);
	//static_cast<EMC1701VoltageSensor *>(sensor_servo1_voltage)->init(emc1701_servo1);
	//static_cast<EMC1701CurrentSensor *>(sensor_3v3_current)   ->init(emc1701_3v3);
	//static_cast<EMC1701CurrentSensor *>(sensor_5v_current)    ->init(emc1701_5v);
	//static_cast<EMC1701VoltageSensor *>(sensor_3v3_voltage)   ->init(emc1701_3v3);
	//static_cast<EMC1701VoltageSensor *>(sensor_5v_voltage)    ->init(emc1701_5v);
	
	tccEncoder::init();
	//emc1701_config();
}

/*
void runtime::emc1701_config()
{
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
	for(auto &emc : {emc1701_servo0, emc1701_servo1, emc1701_5v, emc1701_3v3}) {
		emc->setConfig(conf);
	}
}
*/
