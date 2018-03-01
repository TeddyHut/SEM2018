/*
 * bms.cpp
 *
 * Created: 23/01/2018 5:28:56 PM
 *  Author: teddy
 */ 

#include <cinttypes>
#include <cstdlib>
#include <cstring>

#include "../../utility/conversions.h"

#include "bms.h"

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

template <typename buffer_t>
uint16_t retreiveFromBuffer(buffer_t const &buffer, DataIndexes::e const pos, size_t const offset = 2) {
	return (buffer[pos * 2 + offset] |
	(buffer[pos * 2 + 1 + offset] << 8));
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

	if((sem_spicallback = xSemaphoreCreateBinary()) == NULL) {
		//Error
	}
	if((mtx_bmsdata = xSemaphoreCreateMutex()) == NULL) {
		//Error
	}
	FreeRTOSTask::init(::config::bms::taskName, ::config::bms::taskStackDepth, ::config::bms::taskPriority);
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

		bmsdata.adc_temperature = retreiveFromBuffer(static_cast<uint8_t *>(job.inbuffer), DataIndexes::TemperatureADC);
		bmsdata.adc_current     = retreiveFromBuffer(static_cast<uint8_t *>(job.inbuffer), DataIndexes::CurrentADC);
		bmsdata.adc_voltage[0]  = retreiveFromBuffer(static_cast<uint8_t *>(job.inbuffer), DataIndexes::Cell0ADC);
		bmsdata.adc_voltage[1]  = retreiveFromBuffer(static_cast<uint8_t *>(job.inbuffer), DataIndexes::Cell1ADC);
		bmsdata.adc_voltage[2]  = retreiveFromBuffer(static_cast<uint8_t *>(job.inbuffer), DataIndexes::Cell2ADC);
		bmsdata.adc_voltage[3]  = retreiveFromBuffer(static_cast<uint8_t *>(job.inbuffer), DataIndexes::Cell3ADC);
		bmsdata.adc_voltage[4]  = retreiveFromBuffer(static_cast<uint8_t *>(job.inbuffer), DataIndexes::Cell4ADC);
		bmsdata.adc_voltage[5]  = retreiveFromBuffer(static_cast<uint8_t *>(job.inbuffer), DataIndexes::Cell5ADC);
		
		//Not using bmsdata anymore
		xSemaphoreGive(mtx_bmsdata);
		//Wait until next update
		vTaskDelayUntil(&previousWakeTime, config::bms::refreshRate);
	}
}

void bms::init()
{
	//Set the digital isolator enable inputs to high (both on same pin)
	port_config config;
	port_get_config_defaults(&config);
	config.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(PIN_PB09, &config);
	port_pin_set_output_level(PIN_PB09, true);
}

void bms::sensor::BMSSensor::init(BMS *const parent)
{
	this->parent = parent;
}

float bms::sensor::BMSSensor::get_vcc() const
{
	return utility::conversions::vdiv_input(
		utility::conversions::adc_voltage<uint16_t, 1024>(parent->get_data().adc_voltage[0], 2.5f),
			utility::conversions::vdiv_factor(10.0f, 5.6f));
}

void bms::sensor::CellVoltage::init(BMS *const parent, uint8_t const cellIndex)
{
	BMSSensor::init(parent);
	this->cellIndex = cellIndex;
}

float bms::sensor::CellVoltage::value()
{
	if(cellIndex == 0)
		return get_vcc();
	return utility::conversions::vdiv_input(
		utility::conversions::adc_voltage<uint16_t, 1024>(parent->get_data().adc_voltage[cellIndex], 2.5f), 5.6f / 10.0f);
}

float bms::sensor::Current::value()
{
	auto vcc = get_vcc();
	float opamp_outV = utility::conversions::adc_voltage<uint16_t, 1024>(parent->get_data().adc_current, 1.1f);
	float opamp_inV = utility::conversions::bms_current_opamp_transformation_inV(opamp_outV, vcc);
	return utility::conversions::acs711_current(opamp_inV, vcc);
}

float bms::sensor::Temperature::value()
{
	return utility::conversions::temperature_degrees(
		utility::conversions::vdiv_input(
			utility::conversions::adc_voltage<uint16_t, 1024>(parent->get_data().adc_temperature, 1.1f),
			utility::conversions::vdiv_factor(5.1f, 7.5f)) * 1000.0f);
}
