/*
 * spimanager.cpp
 *
 * Created: 23/01/2018 6:36:23 PM
 *  Author: teddy
 */ 

#include <port.h>

#include "../../main_config.h"

#include "spimanager.h"

void SPIManager::addJob(Job const &job)
{
	if(xQueueSendToBack(que_pendingJobs, &job, 0) == errQUEUE_FULL) {
		//Error
	}
}

void SPIManager::init()
{
	if((que_pendingJobs = xQueueCreate(config::spimanager::pendingQueueSize, sizeof(Job))) == NULL) {
		//Error
	}
	FreeRTOSTask::init(config::spimanager::taskName, config::spimanager::taskStackDepth, config::spimanager::taskPriority);
}

void SPIManager::task_main()
{
	//Wait for child to finish initializing
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	while(true) {
		Job job;
		xQueueReceive(que_pendingJobs, &job, portMAX_DELAY);
		//Pull SS line low
		port_pin_set_output_level(job.ss_pin, false); 
		spi_transceive_buffer_wait(&spi_instance, static_cast<uint8_t *>(job.outbuffer), static_cast<uint8_t *>(job.inbuffer), job.len);
		port_pin_set_output_level(job.ss_pin, true);
		if(job.sem_complete != NULL)
			xSemaphoreGive(job.sem_complete);
	}
}

void SPIManager0::init()
{
	SPIManager::init();
	spi_config config;
	spi_get_config_defaults(&config);
	config.character_size = SPI_CHARACTER_SIZE_8BIT;
	config.data_order = SPI_DATA_ORDER_MSB;
	//Not sure why this is generator 4? TODO: Check
	config.generator_source = GCLK_GENERATOR_4;
	config.master_slave_select_enable = false;
	config.mode = SPI_MODE_MASTER;
	config.mode_specific.master.baudrate = ::config::spimanager::baudrate;
	config.mux_setting = SPI_SIGNAL_MUX_SETTING_D;
	config.pinmux_pad0 = PINMUX_PA16D_SERCOM3_PAD0;
	config.pinmux_pad1 = PINMUX_PA17D_SERCOM3_PAD1;
	config.pinmux_pad2 = PINMUX_UNUSED;
	config.pinmux_pad3 = PINMUX_PA19D_SERCOM3_PAD3;
	config.receiver_enable = true;
	config.run_in_standby = false;
	config.select_slave_low_detect_enable = false;
	config.transfer_mode = SPI_TRANSFER_MODE_0;
	spi_init(&spi_instance, SERCOM3, &config);
	spi_enable(&spi_instance);
	xTaskNotifyGive(m_task);
}
