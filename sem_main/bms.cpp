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
		float result = 2230.8 - m;
		result *= (4 * 0.00433);
		result += std::pow(-13.528, 2);
		result = 13.582 - sqrt(result);
		result /= (2 * -0.00433);
		return result + 30;
	}

	//Gets the temperature from the result of the temperature conversion
	float calculateTemperature(uint16_t const value) {
		return milliVoltsToDegreesC(vdiv_input(adc_voltage<uint16_t, 1024>(value, 1.1f), vdiv_factor(5.1f, 7.5f)) * 1000.0f);
	}

	constexpr float current_outV_error(float const vcc) {
		return (0.015914 * vcc) - 0.03719;
	}
	
	constexpr float current_outV(float const current, float const vref) {
		return (current * ((vref - 0.6f) / (12.5f * 2.0f))) - current_outV_error(vref);
	}

	constexpr float current_current(float const outV, float const vref) {
		float m = ((vref - 0.6f) / (12.5f * 2.0f));
		return (outV + current_outV_error(vref)) / m;
	}

	//Gets the current from the result of the current conversion
	constexpr float calculateCurrent(uint16_t const value, float const vref) {
		return current_current(adc_voltage<uint16_t, 1024>(value, 1.5f), vref);
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
	runtime::mainspi->addJob(job);
}

void BMS::init(uint8_t const pin, size_t const refreshRate /*= config::bms::refreshRate*/)
{
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
		//std::vector<uint8_t> sendbuf(bufsize, 0);
		//std::fill(sendbuf.begin(), sendbuf.end(), 0);
		//RecvBuffer_t recvbuf;
		//std::vector<uint8_t> recvbuf(bufsize);
		//Used malloc for testing. Haven't bothered chaning back (still works). For some reason new and malloc seem to be overwriting each other and causing all sorts of problems.
		uint8_t sendbuf[bufsize];
		std::memset(sendbuf, 0, bufsize);
		uint8_t recvbuf[bufsize];
		sendbuf[0] = Instruction::ReceiveData;
		SPIManager::Job job;
		job.outbuffer = sendbuf;
		job.inbuffer = recvbuf;
		job.len = bufsize;
		job.sem_complete = sem_spicallback;
		job.ss_pin = spi_pin;
		runtime::mainspi->addJob(job);
		xSemaphoreTake(sem_spicallback, portMAX_DELAY);
		xSemaphoreTake(mtx_bmsdata, portMAX_DELAY);
		bmsdata.temperature = calculateTemperature(retreiveFromBuffer(recvbuf, DataIndexes::TemperatureADC));
		bmsdata.cellVoltage[0] = vdiv_input(adc_voltage<uint16_t, 1024>(retreiveFromBuffer(recvbuf, DataIndexes::Cell0ADC), 2.5f), vdiv_factor(10, 5.6));
		//Pretty bad way of checking connection... whatever
		bmsdata.connected = bmsdata.cellVoltage[0] > 1.0f;
		volatile float vcc = bmsdata.cellVoltage[0];
		volatile uint16_t f_adc = retreiveFromBuffer(recvbuf, DataIndexes::CurrentADC);
		volatile float f_outV = adc_voltage<uint16_t, 1024>(f_adc, 1.5f);
		volatile float current = current_current(f_outV, vcc);
		//bmsdata.current = calculateCurrent(currentValue, bmsdata.cellVoltage[0]);
		for(unsigned int i = 1; i < 6; i++) {
			//These use op amps which is why it's just 5.6 / 10
			bmsdata.cellVoltage[i] = vdiv_input(adc_voltage<uint16_t, 1024>(retreiveFromBuffer(recvbuf, static_cast<DataIndexes::e>(i)), 2.5f), 5.6f / 10.0f);
		}
		//Calculate total voltage from cell voltages
		bmsdata.voltage = std::accumulate(bmsdata.cellVoltage.begin(), bmsdata.cellVoltage.end(), 0);
		xSemaphoreGive(mtx_bmsdata);
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
	std::copy(p.cellVoltage.begin(), p.cellVoltage.end(), cellVoltage.begin());
}

void BMS::Data::make_move(Data &&p)
{
	//Doh. Data is on stack, can't move.
	make_copy(p);
}
