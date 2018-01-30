/*
 * adpcontrol.cpp
 *
 * Created: 30/01/2018 5:15:20 PM
 *  Author: teddy
 */ 

#include "adpcontrol.h"

extern "C" {

usart_module instance;

static void adp_interface_send_start(void) {}
static void adp_interface_send_stop(void) {}
static void adp_interface_transceive(uint8_t *tx_data, uint8_t *rx_data, uint16_t length) {
	usart_write_buffer_wait(&instance, tx_data, length);
	usart_read_buffer_wait(&instance, rx_data, length);
}

static void adp_interface_init(void) {
	//This is just copied from ManualSerial, should just put it in a function
	usart_config config;
	usart_get_config_defaults(&config);
	config.baudrate = 115200;
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

	usart_enable(&instance);
}

}

void ADPControl::init()
{
	adp_init();
	//Start task
	//if(xTaskCreate(taskFunction, ::config::manualserial::taskName, ::config::manualserial::taskStackDepth, this, ::config::manualserial::taskPriority, &task) == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY)
	//debugbreak();
}

ADPControl::ADPControl() : Task(Identity::ADPControl)
{
}
