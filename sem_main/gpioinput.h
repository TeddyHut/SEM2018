/*
 * gpioinput.h
 *
 * Created: 25/01/2018 1:29:13 PM
 *  Author: teddy
 */ 

#pragma once

#include <cinttypes>
#include <semphr.h>
#include <port.h>

class GPIOPin {
public:
	virtual bool state() const = 0;
	GPIOPin(SemaphoreHandle_t const sem_changed = NULL) : sem_changed(sem_changed) {}
	SemaphoreHandle_t sem_changed;
};

template <uint32_t gpio_pin, uint32_t gpio_pin_mux, uint8_t extint_channel>
class GPIOPinHW : public GPIOPin {
public:
	static void callback();
	bool state() const override;
	GPIOPin(SemaphoreHandle_t const sem_changed = NULL, extint_pull const pull = EXTINT_PULL_UP, extint_detect const detectioncriteria = EXTINT_DETECT_RISING, bool const wakeifsleeping = false);
};

template <uint32_t gpio_pin, uint32_t gpio_pin_mux, uint8_t extint_channel>
bool GPIOPin<gpio_pin, gpio_pin_mux, extint_channel>::state()
{
	return port_pin_get_input_level(gpio_pin);
}

template <uint32_t gpio_pin, uint32_t gpio_pin_mux, uint8_t extint_channel>
void GPIOPinHW<gpio_pin, gpio_pin_mux, extint_channel>::callback()
{
	BaseType_t higherPrio;
	xSemaphoreGiveFromISR(sem_changed, &higherPrio);
	portYIELD_FROM_ISR(higherPrio);
}

template <uint32_t gpio_pin, uint32_t gpio_pin_mux, uint8_t extint_channel>
GPIOPinHW<gpio_pin, gpio_pin_mux, extint_channel>::GPIOPinHW(SemaphoreHandle_t const sem_changed, extint_pull const pull, extint_detect const detectioncriteria, bool const wakeifsleeping)
 : GPIOPin(sem_changed)
{
	extint_chan_conf config;
	config.detection_criteria = detectioncriteria;
	config.filter_input_signal = false;
	config.gpio_pin = gpio_pin;
	config.gpio_pin_mux = gpio_pin_mux;
	config.gpio_pin_pull = pull;
	config.wake_if_sleeping = wakeifsleeping;
	extint_chan_set_config(extint_channel, &config);
	extint_register_callback(callback, extint_channel, EXTINT_CALLBACK_TYPE_DETECT);
	extint_chan_enable_callback(extint_channel, ,EXTINT_CALLBACK_TYPE_DETECT);
}
