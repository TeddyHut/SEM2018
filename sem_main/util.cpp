/*
 * util.cpp
 *
 * Created: 21/01/2018 9:47:27 PM
 *  Author: teddy
 */

#include "instance.h"

void debugbreak()
{
	if(runtime::buzzer != nullptr)
		runtime::buzzer->stop();
	if(runtime::redLED != nullptr)
		runtime::redLED->setLEDState(true);
	if(runtime::greenLED != nullptr)
		runtime::greenLED->setLEDState(false);
	__asm__("BKPT");
}
