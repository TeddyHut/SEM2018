/*
 * dep_instance.h
 *
 * Created: 23/01/2018 10:29:11 PM
 *  Author: teddy
 */ 

#pragma once

namespace runtime {

namespace inputindex {
	namespace data_f {
		enum Float {
			RTCTime = 0,
			Current_Motor0,
			Current_Motor1,
			Current_Servo0,
			Current_Servo1,
			Voltage_Servo0,
			Voltage_Servo1,
			Voltage_Battery0_Cell0,
			Voltage_Battery0_Cell1,
			Voltage_Battery0_Cell2,
			Voltage_Battery0_Cell3,
			Voltage_Battery0_Cell4,
			Voltage_Battery0_Cell5,
			Voltage_Battery1_Cell0,
			Voltage_Battery1_Cell1,
			Voltage_Battery1_Cell2,
			Voltage_Battery1_Cell3,
			Voltage_Battery1_Cell4,
			Voltage_Battery1_Cell5,
			Current_Battery0,
			Current_Battery1,
			Temperature_Battery0,
			Temperature_Battery1,
			Speed_Motor0,
			Speed_Motor1,
			Speed_Wheel,
			Speed_Wheel_ms,
			_size,
			};
	}
	namespace data_b {
		enum Bool {
			State_OP = 0,
			_size,
		};
	}

}

struct Input {
	//Floating point data (indexes shown in above enum)
	float data_float[inputindex::data_f::_size];
	//Boolean data
	bool data_bool[inputindex::data_b::_size];
};

}