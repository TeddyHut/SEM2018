/*
 * dep_instance.cpp
 *
 * Created: 23/01/2018 10:29:25 PM
 *  Author: teddy
 */ 

#include "dep_instance.h"

BMS			*runtime::bms0;
BMS			*runtime::bms1;
Motor		*runtime::motor0;
Motor		*runtime::motor1;
Encoder		*runtime::encoder0;
Encoder		*runtime::encoder1;
Encoder		*runtime::encoder2;
ServoF		*runtime::servo0;
ServoF		*runtime::servo1;

void runtime::dep_init()
{
	bmsstatic::init();
	
	bms0		 = new BMS(config::bms::bms0_ss_pin);
	bms1		 = new BMS(config::bms::bms1_ss_pin);
	motor0		 = new TCMotor0;
	motor1		 = new TCMotor1;
	encoder0	 = new Encoder0(tccEncoder::motor_speedConvert);
	encoder1	 = new Encoder1(tccEncoder::motor_speedConvert);
	encoder2	 = new Encoder2(tccEncoder::motor_driveTrainConvert);
	servo0		 = new TCCServo0;
	servo1		 = new TCCServo1;

	tccEncoder::init();
}
