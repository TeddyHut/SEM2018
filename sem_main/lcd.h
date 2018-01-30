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
#include <array>
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
	void writeText(const char str[], unsigned int const len);
	void writeText(const char str[]);
	void setBuzzerState(bool const state);
	void setLEDState(bool const state);

	//Send cached instructions
	void send();

	void init();
private:
	struct InputCommand {
		enum e : uint8_t {
			ClearDisplay = 1,
			SetPosition,
			StartBuzzer,
			StopBuzzer,
			LEDOn,
			LEDOff,
		};
	};

	static void taskFunction(void *const viewerBoard);
	static void writeComplete_callback(usart_module *const module);
	
	void task_main();
	static TaskHandle_t task;

	void alloc_buffer();

	using Buffer_t = std::array<char, 40>;
	Buffer_t *pendingBuffer;
	unsigned int pos;
	QueueHandle_t que_pendingBuffers;
	usart_module instance;
};
