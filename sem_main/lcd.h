/*
 * lcd.h
 *
 * Created: 21/01/2018 5:39:18 PM
 *  Author: teddy
 */ 

#pragma once
#include <cstdint>
#include <cinttypes>
#include <string>
#include <vector>
#include <usart.h>
#include <usart_interrupt.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <portable.h>
#include "leds.h"
#include "buzzer.h"

class ViewerBoard {
public:
	class LED : public ::LED {
	public:
		void setLEDState(bool const state) override;
		LED(ViewerBoard &vb);
	private:
		ViewerBoard &vb;
	};
	class Buzzer : public ::Buzzer {
	public:
		void setState(bool const state) override;
		Buzzer(ViewerBoard &vb);
	private:
		ViewerBoard &vb;
	};
	//Cache instructions
	void clearDisplay();
	void setPosition(uint8_t const pos);
	void writeText(std::string const &str);
	void setBuzzerState(bool const state);
	void setLEDState(bool const state);

	//Send cached instructions
	void send();

	ViewerBoard();
private:
	enum class InputCommand : uint8_t {
		ClearDisplay = 1,
		SetPosition,
		StartBuzzer,
		StopBuzzer,
		LEDOn,
		LEDOff,
	};

	static void taskFunction(void *const viewerBoard);
	static void writeComplete_callback(usart_module *const module);
	
	void task_main();
	static SemaphoreHandle_t sem_transferComplete;
	SemaphoreHandle_t sem_pending;
	TaskHandle_t task;

	std::vector<uint8_t> cachedBuffer;
	std::vector<std::vector<uint8_t>> pendingBuffers;
	usart_module instance;
};
