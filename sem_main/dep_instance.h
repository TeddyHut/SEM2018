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

namespace runtime {
	extern BMS			*bms0;
	extern BMS			*bms1;
	extern Motor		*motor0;
	extern Motor		*motor1;
	extern Encoder		*encoder0;
	extern Encoder		*encoder1;
	extern Encoder		*encoder2;
	extern ServoF		*servo0;
	extern ServoF		*servo1;

	void dep_init();
}
