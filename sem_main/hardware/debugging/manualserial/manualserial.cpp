/*
 * manualserial.cpp
 *
 * Created: 26/01/2018 1:13:22 PM
 *  Author: teddy
 */ 

#include "manualserial.h"
#include "main_config.h"
#include "util.h"
#include "instance.h"
#include "dep_instance.h"
#include <usart.h>
#include <usart_interrupt.h>

TaskHandle_t ManualSerial::task = NULL;

void ManualSerial::init()
{
	usart_config config;
	usart_get_config_defaults(&config);
	config.baudrate = 9600;
	config.character_size = USART_CHARACTER_SIZE_8BIT;
	config.clock_polarity_inverted = false;
	config.collision_detection_enable = false;
	config.data_order = USART_DATAORDER_LSB;
	config.encoding_format_enable = false;
	config.generator_source = GCLK_GENERATOR_1;
	config.mux_setting = USART_RX_1_TX_0_XCK_1;
	config.parity = USART_PARITY_NONE;
	config.pinmux_pad0 = PINMUX_PB02D_SERCOM5_PAD0;
	config.pinmux_pad1 = PINMUX_PB03D_SERCOM5_PAD1;
	config.pinmux_pad2 = PINMUX_UNUSED;
	config.pinmux_pad3 = PINMUX_UNUSED;
	config.receiver_enable = true;
	config.sample_rate = USART_SAMPLE_RATE_8X_ARITHMETIC;
	//Might be supposed to be true
	config.start_frame_detection_enable = false;
	config.stopbits = USART_STOPBITS_1;
	config.transfer_mode = USART_TRANSFER_ASYNCHRONOUSLY;
	config.transmitter_enable = true;
	config.use_external_clock = false;
	usart_init(&instance, SERCOM5, &config);

	usart_register_callback(&instance, callback, USART_CALLBACK_BUFFER_RECEIVED);
	usart_enable_callback(&instance, USART_CALLBACK_BUFFER_RECEIVED);
	usart_enable(&instance);

	//Start task
	if(xTaskCreate(taskFunction, ::config::manualserial::taskName, ::config::manualserial::taskStackDepth, this, ::config::manualserial::taskPriority, &task) == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY)
		debugbreak();
}

void ManualSerial::callback(usart_module *const module)
{
	BaseType_t higherPrio;
	vTaskNotifyGiveFromISR(task, &higherPrio);
	portYIELD_FROM_ISR(higherPrio);
}

void ManualSerial::taskFunction(void *manualserial)
{
	static_cast<ManualSerial *>(manualserial)->task_main();
}

uint16_t getFromBuffer(uint8_t const buffer[], unsigned int const pos) {
	return (buffer[pos * 2]) | (buffer[pos * 2 + 1] << 8);
};

void ManualSerial::task_main()
{
	char str[16];
	constexpr unsigned int buffersize = (4 * sizeof(uint16_t));
	uint8_t buffer[buffersize];
	while(true) {
		usart_read_buffer_job(&instance, buffer, buffersize);
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		runtime::motor0->setDutyCycle(getFromBuffer(buffer, 0) / static_cast<float>(0xffff));
		runtime::motor1->setDutyCycle(getFromBuffer(buffer, 1) / static_cast<float>(0xffff));
		runtime::servo0->setPosition(getFromBuffer(buffer, 2) - 180);
		runtime::servo1->setPosition(getFromBuffer(buffer, 3) - 180);
		//memset(str, ' ', 16);
		//runtime::viewerboard->setPosition(0);
		//rpl_snprintf(str, 16, "%7.0f %7.0f", runtime::encoder0->getSpeed(), runtime::encoder1->getSpeed());
		//runtime::viewerboard->writeText(str, 16);
		//runtime::viewerboard->setPosition(40);
		//rpl_snprintf(str, 16, "%7.0f %f", runtime::encoder2->getSpeed(), runtime::encoder0->getSpeed() / runtime::encoder2->getSpeed());
		//runtime::viewerboard->writeText(str, 16);
	}
}
