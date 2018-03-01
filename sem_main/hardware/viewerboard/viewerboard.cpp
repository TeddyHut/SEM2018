/*
 * lcd.cpp
 *
 * Created: 21/01/2018 5:38:54 PM
 *  Author: teddy
 */ 

#include <cstring>
#include <algorithm>

#include "../../main_config.h"

#include "viewerboard.h"

TaskHandle_t ViewerBoard::task = NULL;

void ViewerBoard::clearDisplay()
{
	(*pendingBuffer)[pos++] = InputCommand::ClearDisplay;
}

void ViewerBoard::setPosition(uint8_t const pos)
{
	(*pendingBuffer)[this->pos++] = InputCommand::SetPosition;
	(*pendingBuffer)[this->pos++] = pos;
}

void ViewerBoard::writeText(const char str[], unsigned int const len)
{
	std::memcpy(pendingBuffer->data() + pos, str, len);
	pos += len;
}

void ViewerBoard::writeText(const char str[])
{
	writeText(str, std::strlen(str));
}

void ViewerBoard::setBuzzerState(bool const state)
{
	(*pendingBuffer)[pos++] = state ? InputCommand::StartBuzzer : InputCommand::StopBuzzer;
}

void ViewerBoard::setLEDState(bool const state)
{
	(*pendingBuffer)[pos++] = state ? InputCommand::LEDOn : InputCommand::LEDOff;
}

void ViewerBoard::setBacklightState(bool const state)
{
	(*pendingBuffer)[pos++] = state ? InputCommand::BacklightOn : InputCommand::BacklightOff;
}


void ViewerBoard::send()
{
	//Finish instruction
	(*pendingBuffer)[pos++] = 0xff;
	(*pendingBuffer)[pendingBuffer->size() - 1] = pos;
	if(xQueueSendToBack(que_pendingBuffers, &pendingBuffer, 0) == errQUEUE_FULL) {
		pendingBuffer->~Buffer_t();
		vPortFree(pendingBuffer);
	}
	alloc_buffer();
}

void ViewerBoard::init()
{
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

	alloc_buffer();

	if((que_pendingBuffers = xQueueCreate(::config::viewerboard::pendingQueueSize, sizeof(Buffer_t *))) == NULL) {
		//Error
	}
	FreeRTOSTask::init(::config::viewerboard::taskName, ::config::viewerboard::taskStackDepth, ::config::viewerboard::taskPriority);
	task = FreeRTOSTask::m_task;
}

void ViewerBoard::writeComplete_callback(usart_module *const module)
{
	BaseType_t prio = 0;
	vTaskNotifyGiveFromISR(task, &prio);
	portYIELD_FROM_ISR(prio);
}

void ViewerBoard::task_main()
{
	while(true) {
		//Wait for items in queue
		Buffer_t *buffer = nullptr;
		xQueueReceive(que_pendingBuffers, &buffer, portMAX_DELAY);
		//Make the transfer
		usart_write_buffer_job(&instance, reinterpret_cast<uint8_t *>(buffer->data()), (*buffer)[buffer->size() - 1]);
		//Wait for transfer to complete
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		//Delete the buffer
		buffer->~Buffer_t();
		vPortFree(buffer);
	}
}

void ViewerBoard::alloc_buffer()
{
	pendingBuffer = new (pvPortMalloc(sizeof(Buffer_t))) Buffer_t;
	//Start instruction
	(*pendingBuffer)[0] = 0xfe;
	//+1 to give room for start instruction
	pos = 1;
}

void viewerboard::ViewerBoardItem::init(ViewerBoard *const vb)
{
	this->vb = vb;
}

void viewerboard::Buzzer::valueItem_setValue(bool const value)
{
	if(vb != nullptr)
		vb->setBuzzerState(value);
}

void viewerboard::NotifyLED::valueItem_setValue(bool const value)
{
	if(vb != nullptr)
		vb->setLEDState(value);
}

void viewerboard::BacklightLED::valueItem_setValue(bool const value)
{
	if(vb != nullptr)
		vb->setBacklightState(value);
}
