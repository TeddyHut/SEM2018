/*
 * instance.h
 *
 * Created: 20/01/2018 4:32:43 PM
 *  Author: teddy
 */ 

#pragma once

#include "../../hardware/feedback/valueitem.h"
#include "../../hardware/viewerboard/viewerboard.h"
#include "../../hardware/sensor.h"

namespace hardwareruntime {
	void init();
	extern Sensor<float>    *sensor_rtc_time;
	extern Sensor<float>    *sensor_current_motor       [2];
	extern Sensor<float>    *sensor_current_servo       [2];
	extern Sensor<float>    *sensor_voltage_servo       [2];
	extern Sensor<float>    *sensor_voltage_batteryCell [2][6];
	extern Sensor<float>    *sensor_current_battery     [2];
	extern Sensor<float>    *sensor_temperature_battery [2];
	extern Sensor<float>    *sensor_speed_motor         [2];
	extern Sensor<float>    *sensor_speed_wheel;
	extern Sensor<float>    *sensor_speed_wheel_ms;
	extern Sensor<bool>     *sensor_op_state;
	extern ViewerBoard      *viewerboard_driver;
	extern ValueItem<float> *motor                      [2];
	extern ValueItem<float> *servo                      [2];
	extern ValueItem<bool>  *servopower                 [2];
	extern ValueItem<bool>  *mainbuzzer;
	extern ValueItem<bool>  *redled;
	extern ValueItem<bool>  *greenled;
	extern ValueItem<bool>  *viewerboardnotifyled;
	extern ValueItem<bool>  *viewerboardbacklightled;
	extern ValueItem<bool>  *viewerboardbuzzer;
}
