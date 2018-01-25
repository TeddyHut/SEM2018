/*
 * lcd.cpp
 *
 * Created: 21/01/2018 5:38:54 PM
 *  Author: teddy
 */ 

#include "lcd.h"
#include "config.h"
#include <algorithm>

void ViewerBoard::clearDisplay()
{
	cachedBuffer.push_back(static_cast<uint8_t>(InputCommand::ClearDisplay));
}

void ViewerBoard::setPosition(uint8_t const pos)
{
	cachedBuffer.push_back(static_cast<uint8_t>(InputCommand::SetPosition));
	cachedBuffer.push_back(pos);
}

void ViewerBoard::writeText(std::string const &str)
{
	std::copy(str.begin(), str.end(), std::back_inserter(cachedBuffer));
}

void ViewerBoard::setBuzzerState(bool const state)
{
	cachedBuffer.push_back(static_cast<uint8_t>(state ? InputCommand::StartBuzzer : InputCommand::StopBuzzer));
}

void ViewerBoard::setLEDState(bool const state)
{
	cachedBuffer.push_back(static_cast<uint8_t>(state ? InputCommand::LEDOn : InputCommand::LEDOff));
}

void ViewerBoard::send()
{
	cachedBuffer.insert(cachedBuffer.begin(), 0xfe);
	cachedBuffer.push_back(0xff);
	//May as well just move the data there
	pendingBuffers.push_back(std::move(cachedBuffer));
	cachedBuffer.clear();
	xSemaphoreGive(sem_pending);
}

 ViewerBoard::ViewerBoard()
{
	cachedBuffer.reserve(32);
	pendingBuffers.reserve(2);

	usart_config config;
	usart_get_config_defaults(&config);
	config.baudrate = ::config::viewerboard::baudrate;
	config.character_size = USART_CHARACTER_SIZE_8BIT;
	config.data_order = USART_DATAORDER_LSB;
	config.generator_source = GCLK_GENERATOR_1;
	config.mux_setting = USART_RX_0_TX_2_XCK_3;
	config.parity = USART_PARITY_NONE;
	config.pinmux_pad0 = PINMUX_UNUSED;
	config.pinmux_pad1 = PINMUX_UNUSED;
	config.pinmux_pad2 = PINMUX_PA06D_SERCOM0_PAD2;
	config.pinmux_pad3 = PINMUX_UNUSED;
	config.receiver_enable = false;
	config.stopbits = USART_STOPBITS_1;
	config.transfer_mode = USART_TRANSFER_ASYNCHRONOUSLY;
	config.transmitter_enable = true;
	config.sample_rate = USART_SAMPLE_RATE_16X_FRACTIONAL;
	config.use_external_clock = false;
	usart_init(&instance, SERCOM0, &config);

	usart_register_callback(&instance, writeComplete_callback, USART_CALLBACK_BUFFER_TRANSMITTED);
	usart_enable_callback(&instance, USART_CALLBACK_BUFFER_TRANSMITTED);

	usart_enable(&instance);

	if((sem_transferComplete = xSemaphoreCreateBinary()) == NULL) {
		//Error
	}
	if((sem_pending = xSemaphoreCreateCounting(::config::viewerboard::pendingQueueSize, 0)) == NULL) {
		//Error
	}
	if(xTaskCreate(taskFunction, ::config::viewerboard::taskName, ::config::viewerboard::taskStackDepth, this, ::config::viewerboard::taskPriority, &task) == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY) {
		//Error
	}
}

void ViewerBoard::taskFunction(void *const viewerBoard)
{
	static_cast<ViewerBoard *>(viewerBoard)->task_main();
}

void ViewerBoard::writeComplete_callback(usart_module *const module)
{
	BaseType_t highPriority;
	xSemaphoreGiveFromISR(sem_transferComplete, &highPriority);
	portYIELD_FROM_ISR(highPriority);
}

void ViewerBoard::task_main()
{
	while(true) {
		//Wait for a transfer request
		xSemaphoreTake(sem_pending, portMAX_DELAY);
		auto const &buffer = pendingBuffers[0];
		//Make the transfer
		usart_write_buffer_job(&instance, const_cast<uint8_t *>(buffer.data()), buffer.size());
		//Wait for transfer to complete
		if(xSemaphoreTake(sem_transferComplete, ::config::viewerboard::transmitTimeout) == pdFALSE) {
			//Error
		}
		//Delete the buffer
		pendingBuffers.erase(pendingBuffers.begin());
	}
}

SemaphoreHandle_t ViewerBoard::sem_transferComplete;

void ViewerBoard::LED::setLEDState(bool const state)
{
	vb.setLEDState(state);
	vb.send();
}

 ViewerBoard::LED::LED(ViewerBoard &vb) : vb(vb)
{
}

void ViewerBoard::Buzzer::setState(bool const state)
{
	vb.setBuzzerState(state);
	vb.send();
}

 ViewerBoard::Buzzer::Buzzer(ViewerBoard &vb) : vb(vb)
{

}
