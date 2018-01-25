/*
 * instance.cpp
 *
 * Created: 20/01/2018 4:31:50 PM
 *  Author: teddy
 */ 

#include "instance.h"
#include "util.h"

extern "C" {
	void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName) {
		volatile signed char *name = pcTaskName;
		debugbreak();
	}
}

Buzzer					*runtime::buzzer			= nullptr;
Buzzer					*runtime::vbBuzzer			= nullptr;
BuzzerManager			*runtime::buzzermanager		= nullptr;
BuzzerManager			*runtime::vbBuzzermanager	= nullptr;
ViewerBoard				*runtime::viewerboard		= nullptr;
LED						*runtime::greenLED			= nullptr;
LED						*runtime::redLED			= nullptr;
LED						*runtime::vbLED				= nullptr;
LEDManager				*runtime::greenLEDmanager	= nullptr;
LEDManager				*runtime::redLEDmanager		= nullptr;
LEDManager				*runtime::vbLEDmanager		= nullptr;
SPIManager				*runtime::mainspi			= nullptr;

void runtime::init()
{
	viewerboard			= new ViewerBoard;
	mainspi				= new SPIManager0;
	buzzer				= new Buzzer0;
	vbBuzzer			= new ViewerBoard::Buzzer(*viewerboard);
	buzzermanager		= new BuzzerManager(*buzzer);
	vbBuzzermanager		= new BuzzerManager(*vbBuzzer);
	greenLED			= new PortLED(PIN_PA28);
	redLED				= new PortLED(PIN_PA27);
	vbLED				= new ViewerBoard::LED(*viewerboard);
	greenLEDmanager		= new LEDManager(*greenLED);
	redLEDmanager		= new LEDManager(*redLED);
	vbLEDmanager		= new LEDManager(*vbLED);
}
