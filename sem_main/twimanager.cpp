/*
 * twimanager.cpp
 *
 * Created: 10/02/2018 8:59:57 AM
 *  Author: teddy
 */ 

#include "twimanager.h"
#include "util.h"
#include "main_config.h"

void TWIManager::addJob(Job const &job, TickType_t const waitTime /*= 0*/)
{
	if(xQueueSendToBack(que_pendingJobs, &job, waitTime) == errQUEUE_FULL) {
		//Run non-critical error
		//warning();
	}
}

void TWIManager::init()
{
	if((que_pendingJobs = xQueueCreate(config::twimanager::pendingQueueSize, sizeof(Job))) == NULL)
		debugbreak();
	if(xTaskCreate(taskFunction, config::twimanager::taskName, config::twimanager::taskStackDepth, this, config::twimanager::taskPriority, &task) != pdPASS)
		debugbreak();
}

void TWIManager::taskFunction(void *twimanager)
{
	static_cast<TWIManager *>(twimanager)->task_main();
}

void TWIManager::task_main()
{
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	while(true) {
		Job job;
		xQueueReceive(que_pendingJobs, &job, portMAX_DELAY);
		//Create a packet
		i2c_master_packet packet;
		packet.address = job.address;
		packet.high_speed = false;
		packet.ten_bit_address = false;
		switch (job.mode) {
		case Job::Mode::Write:
			packet.data = static_cast<uint8_t *>(job.outbuffer);
			packet.data_length = job.writelen;
			i2c_master_write_packet_job(&twi_instance, &packet);
			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
			break;
		case Job::Mode::WriteRead:
			packet.data = static_cast<uint8_t *>(job.outbuffer);
			packet.data_length = job.writelen;
			i2c_master_write_packet_job_no_stop(&twi_instance, &packet);
			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
			//Fallthrough intentional
		case Job::Mode::Read:
			packet.data = static_cast<uint8_t *>(job.inbuffer);
			packet.data_length = job.readlen;
			i2c_master_read_packet_job(&twi_instance, &packet);
			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
			break;
		default:
			break;
		}
		if(job.sem_complete != NULL)
			xSemaphoreGive(job.sem_complete);
	}
}

void TWIManager0::twi_callback(i2c_master_module *const module)
{
	BaseType_t prio = 0;
	vTaskNotifyGiveFromISR(callbackTask, &prio);
	portYIELD_FROM_ISR(prio);
}

TaskHandle_t TWIManager0::callbackTask = NULL;

void TWIManager0::init()
{
	TWIManager::init();
	callbackTask = TWIManager::task;
	i2c_master_config config;
	i2c_master_get_config_defaults(&config);
	config.baud_rate = I2C_MASTER_BAUD_RATE_100KHZ;
	config.generator_source = GCLK_GENERATOR_1;
	config.pinmux_pad0 = PINMUX_PA00D_SERCOM1_PAD0;
	config.pinmux_pad1 = PINMUX_PA01D_SERCOM1_PAD1;
	config.transfer_speed = I2C_MASTER_SPEED_STANDARD_AND_FAST;
	i2c_master_init(&twi_instance, SERCOM1, &config);
	i2c_master_register_callback(&twi_instance, twi_callback, I2C_MASTER_CALLBACK_READ_COMPLETE);
	i2c_master_register_callback(&twi_instance, twi_callback, I2C_MASTER_CALLBACK_WRITE_COMPLETE);
	i2c_master_enable_callback(&twi_instance, I2C_MASTER_CALLBACK_READ_COMPLETE);
	i2c_master_enable_callback(&twi_instance, I2C_MASTER_CALLBACK_WRITE_COMPLETE);
	i2c_master_enable(&twi_instance);
}
