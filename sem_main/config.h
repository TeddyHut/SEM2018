/*
 * config.h
 *
 * Created: 18/01/2018 4:10:34 PM
 *  Author: teddy
 */ 

#pragma once

#include <FreeRTOS.h>
#include <cstddef>
#include <cinttypes>
#include <tcc.h>

constexpr TickType_t msToTicks(float const ms) {
	return (ms / 1000.0f) * configTICK_RATE_HZ;
}

constexpr float kmhToMs(float const khm) {
	return 0;
}

constexpr float msToKmh(float const ms) {
	return 0;
}

namespace config {
	namespace run {
		constexpr float servorestposition = 0;
		constexpr float servoengagedpositon = 0;

		constexpr float minimumspeed = 30;
		constexpr float maximumspeed = 40;

		constexpr float ramuptime = 10;
		constexpr float servoengagetime = 0.1;
		constexpr float servodisengagetime = 0.1;
		constexpr float matchramptime = 1;
		constexpr float coastramptime = 10;
		constexpr float errorrange = 0.01;

		constexpr float alertvoltage = 3.2;
		constexpr float shutdownvoltage = 3.05;
		constexpr float alertbmscurrent = 5;
		constexpr float shutdownbmscurrent = 7;
		constexpr float alertmotorcurrent = 4;
		constexpr float shotdownmotorcurrent = 6;

		constexpr TickType_t displaycycleperiod = msToTicks(2000);
	}
	namespace servo {
		constexpr float clockFrequency = 8000000;
		constexpr float period = 0.02;
		constexpr float midpoint = 0.0015;
		constexpr float deviation = 0.0015;
		constexpr float minAngle = -180;
		constexpr float maxAngle = 180;
		constexpr float restAngle = 0;
		constexpr float engagedAngle = 0;
	}
	namespace motor {
		constexpr float clockFrequency = 8000000;
		constexpr float period = 0.02;
	}
	namespace buzzermanager {
		constexpr size_t sequenceQueueSize = 4;
	}
	namespace buzzer {
		constexpr float clockFrequency = 8000000;
		constexpr float period = 1 / 2400.0f;
	}
	namespace encoder {
		constexpr size_t bufferSize = 2;
		constexpr tcc_clock_prescaler prescaeSetting = TCC_CLOCK_PRESCALER_DIV1;
		constexpr gclk_generator clockSource = GCLK_GENERATOR_3;
	}
	namespace viewerboard {
		constexpr size_t baudrate = 9600;
		constexpr size_t pendingQueueSize = 2;
		constexpr char const * taskName = "ViewerBoard";
		constexpr uint16_t taskStackDepth = 64;
		constexpr unsigned int taskPriority = 1;
		constexpr size_t transmitTimeout = 100;
	}
	namespace feedbackmanager {
		constexpr char const * taskName = "FeedbackManager";
		constexpr uint16_t taskStackDepth = 64;
		constexpr unsigned int taskPriority = 1;
	}
	namespace bms {
		constexpr char const * taskName = "BMS";
		constexpr uint16_t taskStackDepth = 96;
		constexpr unsigned int taskPriority = 1;
		constexpr size_t refreshRate = 100;
		constexpr unsigned int bms0_ss_pin = PIN_PA11;
		constexpr unsigned int bms1_ss_pin = PIN_PB08;
	}
	namespace spimanager {
		constexpr size_t baudrate = 100000;
		constexpr char const * taskName = "SPIManager";
		constexpr uint16_t taskStackDepth = 64;
		constexpr unsigned int taskPriority = 1;
		constexpr size_t pendingQueueSize = 2;
	}
	namespace led {
		constexpr unsigned int greenLED_pin = PIN_PA28;
		constexpr unsigned int redLED_pin = PIN_PA27;
	}
}
