/*
 * dep_instance.h
 *
 * Created: 23/01/2018 10:29:11 PM
 *  Author: teddy
 */ 

#pragma once

#include "bms.h"
#include "motor.h"
#include "encoder.h"
#include "servo.h"
#include "emc1701.h"
#include "sensor.h"

namespace runtime {
	extern BMS			 *bms0;
	extern BMS			 *bms1;
	extern Motor		 *motor0;
	//extern Motor		 *motor1;
	extern Encoder		 *encoder0;
	//extern Encoder		 *encoder1;
	//extern Encoder		 *encoder2;
	//extern ServoF		 *servo0;
	//extern ServoF		 *servo1;
	//extern EMC1701       *emc1701_servo0;
	//extern EMC1701       *emc1701_servo1;
	//extern EMC1701       *emc1701_5v;
	//extern EMC1701       *emc1701_3v3;
	//extern Sensor<float> *sensor_servo0_current;
	//extern Sensor<float> *sensor_servo1_current;
	//extern Sensor<float> *sensor_servo0_voltage;
	//extern Sensor<float> *sensor_servo1_voltage;
	//extern Sensor<float> *sensor_3v3_current;
	//extern Sensor<float> *sensor_5v_current;
	//extern Sensor<float> *sensor_3v3_voltage;
	//extern Sensor<float> *sensor_5v_voltage;

	void dep_init();
}
