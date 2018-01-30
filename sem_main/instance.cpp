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
		volatile signed char *volatile name = pcTaskName;
		debugbreak();
	}
}

using GPIOPin_OP_t = GPIOPinHW<PIN_PA05A_EIC_EXTINT5, MUX_PA05A_EIC_EXTINT5, 5>;

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
GPIOPin					*runtime::opPresence		= nullptr;

void runtime::init()
{
	//Set system interruot priorities (for interrupts that need to use FreeRTOS API)
	//LCD
	system_interrupt_set_priority(SYSTEM_INTERRUPT_MODULE_SERCOM0, SYSTEM_INTERRUPT_PRIORITY_LEVEL_2);
	//SPI Manager
	system_interrupt_set_priority(SYSTEM_INTERRUPT_MODULE_SERCOM3, SYSTEM_INTERRUPT_PRIORITY_LEVEL_2);
	//ManualSerial
	system_interrupt_set_priority(SYSTEM_INTERRUPT_MODULE_SERCOM5, SYSTEM_INTERRUPT_PRIORITY_LEVEL_2);
	//Extenal pins
	system_interrupt_set_priority(SYSTEM_INTERRUPT_MODULE_EIC, SYSTEM_INTERRUPT_PRIORITY_LEVEL_2);
	//Encoders
	system_interrupt_set_priority(SYSTEM_INTERRUPT_MODULE_EVSYS, SYSTEM_INTERRUPT_PRIORITY_LEVEL_0);
	system_interrupt_set_priority(SYSTEM_INTERRUPT_MODULE_TCC2, SYSTEM_INTERRUPT_PRIORITY_LEVEL_0);

	viewerboard			= new (pvPortMalloc(sizeof(ViewerBoard))			)	ViewerBoard;
	mainspi				= new (pvPortMalloc(sizeof(SPIManager0))			)	SPIManager0;
	buzzer				= new (pvPortMalloc(sizeof(Buzzer0))				)	Buzzer0;
	vbBuzzer			= new (pvPortMalloc(sizeof(ViewerBoard::Buzzer))	)	ViewerBoard::Buzzer(*viewerboard);
	buzzermanager		= new (pvPortMalloc(sizeof(BuzzerManager))		)	BuzzerManager;
	vbBuzzermanager		= new (pvPortMalloc(sizeof(BuzzerManager))		)	BuzzerManager;
	greenLED			= new (pvPortMalloc(sizeof(PortLED))				)	PortLED;
	redLED				= new (pvPortMalloc(sizeof(PortLED))				)	PortLED;
	vbLED				= new (pvPortMalloc(sizeof(ViewerBoard::LED))		)	ViewerBoard::LED(*viewerboard);
	greenLEDmanager		= new (pvPortMalloc(sizeof(LEDManager))			)	LEDManager;
	redLEDmanager		= new (pvPortMalloc(sizeof(LEDManager))			)	LEDManager;
	vbLEDmanager		= new (pvPortMalloc(sizeof(LEDManager))			)	LEDManager;
	opPresence			= new (pvPortMalloc(sizeof(GPIOPin_OP_t)))			GPIOPin_OP_t;

	viewerboard								->init();
	static_cast<SPIManager0 *>(mainspi)		->init();
	static_cast<Buzzer0 *>(buzzer)			->init();
	buzzermanager							->init(buzzer);
	vbBuzzermanager							->init(vbBuzzer);
	static_cast<PortLED *>(greenLED)		->init(PIN_PA28);
	static_cast<PortLED *>(redLED)			->init(PIN_PA27);
	greenLEDmanager							->init(greenLED);
	redLEDmanager							->init(redLED);
	vbLEDmanager							->init(vbLED);
}
