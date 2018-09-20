/*
 * usbmsc.h
 *
 * Created: 12/09/2018 2:14:21 PM
 *  Author: teddy
 */ 

#pragma once

#include <FreeRTOS.h>
#include <task.h>
#include <uhc.h>
#include "ff.h"

class USBMSC {
public:
	struct Settings {
		enum class TestType {
			None,
			Torque,
			DutyCycle,
			Frequency,
		} testtype = TestType::None;
		float holdTime = 0;
		float springConstant = 0;
		float dutyCycle[2] = {0, 0};
		float frequency[2] = {0, 0};
		unsigned int dutyCycleDivisions = 0;
		unsigned int frequencyDivisions = 0;
	};
	enum class WakeCause {
		None,
		Connected,
		Disconnected,
		Reload,
	};
	friend void usb_host_connection_event(uhc_device_t *const, bool const);
	static constexpr int RELOAD_NEXT = -2;

	void init();
	bool isReady() const;
	bool isError() const;
	char const *const lastError() const;
	char const *const name() const;
	void reload(int const settingsIndex = -1);
	FIL file;
	Settings settings;
private:
	static void taskFunction(void *const usbmsc);
	static TaskHandle_t task;
	static volatile WakeCause wakeCause;
	
	void task_main();
	FATFS fs;
	bool ready = false;
	bool iserror = false;
	char lasterror[16];
	char fileName[16];
	//Less than zero means to just load settings.txt
	int settingsIndex = 5;
};
