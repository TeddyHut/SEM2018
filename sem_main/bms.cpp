/*
 * bms.cpp
 *
 * Created: 23/01/2018 5:28:56 PM
 *  Author: teddy
 */ 

#include "bms.h"
#include "util.h"
#include "spimanager.h"
#include <cinttypes>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <cstdlib>
#include <cstring>

using namespace adcutil;

namespace {
	struct DataIndexes {
		enum e {
			TemperatureADC = 0,
			CurrentADC,
			Cell0ADC,
			Cell1ADC,
			Cell2ADC,
			Cell3ADC,
			Cell4ADC,
			Cell5ADC,
			_size,
		};
	};
	struct Instruction {
		enum : uint8_t {
			Normal = 0,
			ReceiveData,
			FireRelay,
			LEDOn,
			LEDOff,
		};
	};

	constexpr size_t bufsize = DataIndexes::_size * 2 + 2; //Plus 2 is for BMS processing at start
	using RecvBuffer_t = std::array<uint8_t, bufsize>;
	
	float milliVoltsToDegreesC(float const m) {
		//Taken from temperature sensor datasheet
		float result = 2230.8 - m;
		result *= (4 * 0.00433);
		result += std::pow(-13.582, 2);
		result = 13.582 - sqrt(result);
		result /= (2 * -0.00433);
		return result + 30;
	}

	float calculateTemperature(uint16_t const value) {
		return milliVoltsToDegreesC(vdiv_input(adc_voltage<uint16_t, 1024>(value, 2.5f), vdiv_factor(5.1f, 7.5f)) * 1000.0f);
	}

	constexpr float current_opamp_transformation_outV_bms(float const inV, float const vcc) {
		return (inV * (5.0f / 6.0f)) - (vcc / 3.0f);
	}

	constexpr float current_opamp_transformation_inV_bms(float const outV, float const vcc) {
		return (6.0f / 5.0f) * (outV + (vcc / 3.0f));
	}

	template <typename buffer_t>
	uint16_t retreiveFromBuffer(buffer_t const &buffer, DataIndexes::e const pos, size_t const offset = 2) {
		return (buffer[pos * 2 + offset] |
		(buffer[pos * 2 + 1 + offset] << 8));
	}
}

BMS::Data BMS::get_data() const
{
	xSemaphoreTake(mtx_bmsdata, portMAX_DELAY);
	//This is where std::lock_guard (I think) comes in really handy
	Data rtrn = bmsdata;
	xSemaphoreGive(mtx_bmsdata);
	return rtrn;
}

void BMS::set_refreshRate(size_t const ticks)
{
	refreshRate = ticks;
}

void BMS::setLEDState(bool const state)
{
	cmdOutbuf = state ? Instruction::LEDOn : Instruction::LEDOff;
	SPIManager::Job job;
	job.inbuffer = &cmdInbuf;
	job.outbuffer = &cmdOutbuf;
	job.sem_complete = NULL;
	job.ss_pin = spi_pin;
	job.len = 1;
	spiManager->addJob(job);
}

void BMS::init(SPIManager *const spiManager, uint8_t const pin, size_t const refreshRate /*= config::bms::refreshRate*/)
{
	this->spiManager = spiManager;
	this->spi_pin = pin;
	this->refreshRate = refreshRate;
	port_config config;
	port_get_config_defaults(&config);
	config.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(pin, &config);
	port_pin_set_output_level(pin, true);

	if((sem_spicallback = xSemaphoreCreateBinary()) == NULL)
		debugbreak();
	if((mtx_bmsdata = xSemaphoreCreateMutex()) == NULL)
		debugbreak();
	if(xTaskCreate(taskFunction, config::bms::taskName, config::bms::taskStackDepth, this, config::bms::taskPriority, &task) == pdFAIL)
		debugbreak();	
}

void BMS::taskFunction(void *const bms)
{
	static_cast<BMS *>(bms)->task_main();
}

void BMS::task_main()
{
	TickType_t previousWakeTime = xTaskGetTickCount();
	while(true) {
		//Setup buffers for spimanager job
		uint8_t sendbuf[bufsize];
		std::memset(sendbuf, 0, bufsize);
		uint8_t recvbuf[bufsize];
		//Request BMS data
		sendbuf[0] = Instruction::ReceiveData;

		//Make a request to SPIManager
		SPIManager::Job job;
		job.outbuffer = sendbuf;
		job.inbuffer = recvbuf;
		job.len = bufsize;
		job.sem_complete = sem_spicallback;
		job.ss_pin = spi_pin;
		spiManager->addJob(job);

		//Wait for bmsdata to be available and for the transfer to have completed
		xSemaphoreTake(sem_spicallback, portMAX_DELAY);
		xSemaphoreTake(mtx_bmsdata, portMAX_DELAY);

		//Calculate the temperature and first cell voltage (which uses a voltage divider instead of the opamps)
		bmsdata.temperature = calculateTemperature(retreiveFromBuffer(recvbuf, DataIndexes::TemperatureADC));
		bmsdata.cellVoltage[0] = vdiv_input(adc_voltage<uint16_t, 1024>(retreiveFromBuffer(recvbuf, DataIndexes::Cell0ADC), 2.5f), vdiv_factor(10, 5.6));
		//If the cellVoltage is greater than one a connection to the BMS was made (not a great way to check a connection. TOOD: Signature)
		bmsdata.connected = bmsdata.cellVoltage[0] > 1.0f;

		//Calculate BMS current
		float vcc = bmsdata.cellVoltage[0];
		uint16_t current_adc = retreiveFromBuffer(recvbuf, DataIndexes::CurrentADC);
		float opamp_outV = adc_voltage<uint16_t, 1024>(current_adc, 1.1f);
		float opamp_inV = current_opamp_transformation_inV_bms(opamp_outV, vcc);
		bmsdata.current = acs711_current(opamp_inV, vcc);

		//Calculate other cell voltages
		for(unsigned int i = DataIndexes::Cell1ADC; i < DataIndexes::_size; i++) {
			volatile uint16_t adcVal = retreiveFromBuffer(recvbuf, static_cast<DataIndexes::e>(i));
			//These use op amps which is why it's just 5.6 / 10
			bmsdata.cellVoltage[i - DataIndexes::Cell1ADC + 1] = vdiv_input(adc_voltage<uint16_t, 1024>(adcVal, 2.5f), 5.6f / 10.0f);
		}
		
		//Calculate total voltage from cell voltages (std::accumulate wasn't working?)
		float total = 0;
		for(unsigned int i = 0; i < bmsdata.cellVoltage.size(); i++) {
			total += bmsdata.cellVoltage[i];
		}
		bmsdata.voltage = total;
		
		//Not using bmsdata anymore
		xSemaphoreGive(mtx_bmsdata);
		//Wait until next update
		vTaskDelayUntil(&previousWakeTime, config::bms::refreshRate);
	}
}

void bmsstatic::init()
{
	//Set the digital isolator enable inputs to high (both on same pin)
	port_config config;
	port_get_config_defaults(&config);
	config.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(PIN_PB09, &config);
	port_pin_set_output_level(PIN_PB09, true);
}

BMS::Data & BMS::Data::operator=(Data const &p)
{
	make_copy(p);
	return *this;
}

BMS::Data & BMS::Data::operator=(Data &&p)
{
	make_move(std::move(p));
	return *this;
}

 BMS::Data::Data(Data const &p)
{
	make_copy(p);
}

 BMS::Data::Data(Data &&p)
{
	make_move(std::move(p));
}

void BMS::Data::make_copy(Data const &p)
{
	connected = p.connected;
	temperature = p.temperature;
	current = p.current;
	voltage = p.voltage;
	std::copy(p.cellVoltage.begin(), p.cellVoltage.end(), cellVoltage.begin());
}

void BMS::Data::make_move(Data &&p)
{
	//Doh. Data is on stack, can't move.
	make_copy(p);
}
