/*
 * main_config.h
 *
 * Created: 24/02/2018 5:37:38 PM
 *  Author: teddy
 */ 

#pragma once

#include <FreeRTOS.h>
#include <cstddef>
#include <cinttypes>
#include <tcc.h>

//#define MANUAL_MODE

constexpr TickType_t msToTicks(float const ms) {
	return (ms / 1000.0f) * configTICK_RATE_HZ;
}

constexpr float kmhToMs(float const kmh) {
	return kmh / 3.6;
}

constexpr float msToKmh(float const ms) {
	return ms * 3.6;
}

namespace config {
	namespace run {
		constexpr float stoptimeout = 5;

		constexpr TickType_t displaycycleperiod = msToTicks(5000);
		constexpr TickType_t displayrefreshrate = msToTicks(100);
		constexpr TickType_t refreshRate = msToTicks(1000.0f / 120.0f);
		constexpr float syncinterval = 1.0f / 1.0f;

		constexpr size_t buttonBufferSize = 10;
	}
	namespace motor {
		constexpr unsigned int motor0_pin = PIN_PA18;
		constexpr float clockFrequency = 8000000;
		constexpr float period = (1.0f / 100000.0f);
	}
	namespace buzzermanager {
		constexpr size_t sequenceQueueSize = 4;
	}
	namespace ledmanager {
		constexpr size_t sequenceSqueueSize = 2;
	}
	namespace buzzer {
		constexpr float clockFrequency = 8000000;
		constexpr float period = 1 / 2400.0f;
	}
	namespace encoder {
		constexpr size_t bufferSize = 6;
		constexpr tcc_clock_prescaler prescaeSetting = TCC_CLOCK_PRESCALER_DIV1;
		constexpr gclk_generator clockSource = GCLK_GENERATOR_3;
		//1 second of no ticks results in an output of zero
		constexpr unsigned int zerotimeout = motor::clockFrequency * 1;
	}
	namespace viewerboard {
		constexpr size_t baudrate = 9600;
		constexpr size_t pendingQueueSize = 1;
		constexpr char const * taskName = "ViewerBoard";
		constexpr uint16_t taskStackDepth = 96;
		constexpr unsigned int taskPriority = 1;
		constexpr size_t transmitTimeout = 100;
	}
	namespace feedbackmanager {
		constexpr char const * taskName = "FeedbackManager";
		constexpr uint16_t taskStackDepth = 52;
		constexpr unsigned int taskPriority = 1;
	}
	namespace bms {
		constexpr char const * taskName = "BMS";
		constexpr uint16_t taskStackDepth = 96;
		constexpr unsigned int taskPriority = 1;
		constexpr size_t refreshRate = msToTicks(100);
		constexpr unsigned int bms0_ss_pin = PIN_PA11;
		constexpr unsigned int bms1_ss_pin = PIN_PB08;
	}
	namespace spimanager {
		constexpr size_t baudrate = 100000;
		constexpr char const * taskName = "SPIManager";
		constexpr uint16_t taskStackDepth = 64;
		constexpr unsigned int taskPriority = 1;
		constexpr size_t pendingQueueSize = 4;
	}
	namespace led {
		constexpr unsigned int greenLED_pin = PIN_PA28;
		constexpr unsigned int redLED_pin = PIN_PA27;
	}
	namespace usbmsc {
		constexpr char const * taskName = "USBMSC";
		constexpr uint16_t taskStackDepth = 512;
		constexpr unsigned int taskPriority = 1;
	}
}
