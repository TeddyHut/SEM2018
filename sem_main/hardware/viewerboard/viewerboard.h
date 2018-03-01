/*
 * lcd.h
 *
 * Created: 21/01/2018 5:39:18 PM
 *  Author: teddy
 */ 

#pragma once

#include <cstdint>
#include <cinttypes>
#include <array>

#include <usart.h>
#include <usart_interrupt.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include "../../utility/freertos_task.h"
#include "../feedback/valueitem.h"

class ViewerBoard : public utility::FreeRTOSTask {
public:
	//Cache instructions
	void clearDisplay();
	void setPosition(uint8_t const pos);
	void writeText(const char str[], unsigned int const len);
	void writeText(const char str[]);
	void setBuzzerState(bool const state);
	void setLEDState(bool const state);
	void setBacklightState(bool const state);

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
			BacklightOn,
			BacklightOff,
		};
	};

	static void writeComplete_callback(usart_module *const module);
	
	void task_main() override;
	static TaskHandle_t task;

	void alloc_buffer();

	using Buffer_t = std::array<char, 40>;
	Buffer_t *pendingBuffer;
	unsigned int pos;
	QueueHandle_t que_pendingBuffers;
	usart_module instance;
};

namespace viewerboard {
	class ViewerBoardItem {
	public:
		void init(ViewerBoard *const vb);
	protected:
		ViewerBoard *vb = nullptr;
	};
	class Buzzer : public ValueItem<bool>, public ViewerBoardItem {
	private:
		void valueItem_setValue(bool const value) override;
	};

	class NotifyLED : public ValueItem<bool>, public ViewerBoardItem {
	private:
		void valueItem_setValue(bool const value) override;
	};

	class BacklightLED : public ValueItem<bool>, public ViewerBoardItem {
	private:
		void valueItem_setValue(bool const value) override;
	};
}
