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

#include "../../main_config.h"
#include "../../utility/freertos_task.h"
#include "../spimanager/spimanager.h"
#include "../sensor.h"

class BMS : public utility::FreeRTOSTask {
public:
	struct Data {
		uint16_t adc_temperature = 0;
		uint16_t adc_current = 0;
		uint16_t adc_voltage[6] = {0};
	};
	
	//Returns a copy so that it's thread safe
	Data get_data() const;
	//Sets the refresh rate for when data should be retrieved from the BMS
	void set_refreshRate(size_t const ticks);

	void setLEDState(bool const state);

	void init(SPIManager *const spiManager, uint8_t const pin, size_t const refreshRate = config::bms::refreshRate);
private:
	void task_main() override;

	SPIManager *spiManager = nullptr;
	TickType_t refreshRate;

	Data bmsdata;
	mutable SemaphoreHandle_t mtx_bmsdata;
	SemaphoreHandle_t sem_spicallback;

	uint8_t spi_pin;
	uint8_t cmdOutbuf;
	uint8_t cmdInbuf;
};

namespace bms {
	void init();

	namespace sensor {
		class BMSSensor {
		public:
			void init(BMS *const parent);
		protected:
			float get_vcc() const;
			BMS *parent = nullptr;
		};
		class CellVoltage : public Sensor<float>, public BMSSensor {
		public:
			void init(BMS *const parent, uint8_t const cellIndex);
			float value() override;
		private:
			uint8_t cellIndex = 0;
		};
		class Current : public Sensor<float>, public BMSSensor {
		public:
			float value() override;
		};
		class Temperature : public Sensor<float>, public BMSSensor {
		public:
			float value() override;
		};
	}
}
