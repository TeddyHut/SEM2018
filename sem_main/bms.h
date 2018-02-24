/*
 * bms.h
 *
 * Created: 23/01/2018 5:28:35 PM
 *  Author: teddy
 */ 

#pragma once
#include <array>
#include <spi.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include "config.h"
#include "instance.h"

namespace bmsstatic {
	void init();
}

constexpr float acs711_outV(float const current, float const vcc) {
	return (current * ((vcc - 0.6f) / (12.5f * 2.0f))) + (vcc / 2);
}

constexpr float acs711_current(float const outV, float const vcc) {
	return ((12.5f * 2.0f) / (vcc - 0.6f)) * (outV - (vcc / 2.0f));
}

class BMS {
public:
	struct Data {
		bool connected = false;
		float temperature = 0;
		float current = 0;
		float voltage = 0;
		std::array<float, 6> cellVoltage{6};

		//These should be necessary with std::array. TODO: Remove
		Data() = default;
		Data(Data const &p);
		Data(Data &&p);
		Data &operator=(Data const &p);
		Data &operator=(Data &&p);
	private:
		//Should have just overloaded the same function...
		void make_copy(Data const &p);
		void make_move(Data &&p);
	};
	
	//Returns a copy so that it's thread safe
	Data get_data() const;
	//Sets the refresh rate for when data should be retrieved from the BMS
	void set_refreshRate(size_t const ticks);

	void setLEDState(bool const state);

	void init(SPIManager *const spiManager, uint8_t const pin, size_t const refreshRate = config::bms::refreshRate);
private:
	static void taskFunction(void *const bms);
	
	void task_main();
	TaskHandle_t task;
	SemaphoreHandle_t sem_spicallback;

	SPIManager *spiManager = nullptr;
	uint8_t spi_pin;
	//Being lazy and not using a mutex for this one
	size_t refreshRate;
	
	uint8_t cmdOutbuf;
	uint8_t cmdInbuf;

	mutable SemaphoreHandle_t mtx_bmsdata;
	Data bmsdata;
};
