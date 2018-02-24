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
	
	bms0         = new (pvPortMalloc(sizeof(BMS      ))) BMS;
	bms1         = new (pvPortMalloc(sizeof(BMS      ))) BMS;
	motor0       = new (pvPortMalloc(sizeof(TCMotor0 ))) TCMotor0;
	motor1       = new (pvPortMalloc(sizeof(TCMotor1 ))) TCMotor1;
	encoder0     = new (pvPortMalloc(sizeof(Encoder0 ))) Encoder0(tccEncoder::motor_speedConvert);
	encoder1     = new (pvPortMalloc(sizeof(Encoder1 ))) Encoder1(tccEncoder::motor_speedConvert);
	encoder2     = new (pvPortMalloc(sizeof(Encoder2 ))) Encoder2(tccEncoder::motor_driveTrainConvert);
	servo0       = new (pvPortMalloc(sizeof(TCCServo0))) TCCServo0;
	servo1       = new (pvPortMalloc(sizeof(TCCServo1))) TCCServo1;

	bms0->init(runtime::mainspi, config::bms::bms0_ss_pin);
	bms1->init(runtime::mainspi, config::bms::bms1_ss_pin);

	tccEncoder::init();
}
