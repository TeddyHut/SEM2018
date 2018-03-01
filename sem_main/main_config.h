/*
 * main_config.h
 *
 * Created: 24/02/2018 5:37:38 PM
 *  Author: teddy
 */ 

#pragma once

#include <cstddef>
#include <cinttypes>

#include <FreeRTOS.h>

#include <tcc.h>
#include <adc.h>

#include "utility/conversions.h"

#define BOARD_NUMBER 1

#if (BOARD_NUMBER == 1)
#define PATCH_SERVO1_ENABLE_PA23 1
#define PATCH_ENCODER2_TO_ENCODER1 1
#else
#define PATCH_SERVO1_ENABLE_PA23 0
#define PATCH_ENCODER2_TO_ENCODER1 1
#endif

#if (PATCH_SERVO1_ENABLE_PA23 == 1)
#define SERVO1_ENABLE_PIN PIN_PA23
#else
#define SERVO1_ENABLE_PIN PIN_PA09
#endif

#define ENABLE_MANUALSERIAL 0
#define ENABLE_ADPCONTROL 1

namespace config {
	namespace run {
 		constexpr float servo0restposition = 90;
		constexpr float servo0engagedposition = 108;
		constexpr float servo1restposition = 136;
		constexpr float servo1engagedposition = 118;

		constexpr float minimumspeed = utility::conversions::kmhToMs(22.0f);
		constexpr float maximumspeed = utility::conversions::kmhToMs(28.0f);

		constexpr float ramuptime = 10;
		constexpr float servoengagetime = 0.1;
		constexpr float servodisengagetime = 0.1;
		constexpr float matchramptime = 5;
		constexpr float coastramptime = 10;
		constexpr float errorrange = 0.05f;

		constexpr float alertvoltage = 3.2;
		constexpr float shutdownvoltage = 3.05;
		constexpr float alertbmscurrent = 5;
		constexpr float shutdownbmscurrent = 7;
		constexpr float alertmotorcurrent = 4;
		constexpr float shotdownmotorcurrent = 6;

		constexpr TickType_t displaycycleperiod = utility::conversions::msToTicks(5000.0f);
		constexpr TickType_t displayrefreshrate = utility::conversions::msToTicks(100.0f);
		constexpr TickType_t refreshRate = utility::conversions::msToTicks(1000.0f / 60.0f);
		constexpr float speedmatchcorrecttimeout = 1.5f;

		constexpr size_t buttonBufferSize = 10;
	}
	namespace hardware {
		constexpr float motortogeartickratio = 0.482; //Enoder0.getinterval / encoder2.getinterval
		constexpr float motorteeth = 12;
		constexpr float wheelradius = 0.43 / 2;
		constexpr float vcc = 3.3f;
	}
	namespace adpcontrol {
		constexpr uint16_t taskStackDepth = 256;
		constexpr char const * taskName = "ADP";
		constexpr unsigned int taskPriority = 1;
	}
	namespace servo {
		constexpr float clockFrequency = 39999900;
		constexpr float period = 0.02;
		constexpr float servominpulse = 0.000544;
		constexpr float servomaxpulse = 0.0024;
		constexpr float midpoint0 = ((servomaxpulse + servominpulse) / 2.0f);
		constexpr float deviation0 = ((servomaxpulse - servominpulse) / 2.0f);
		constexpr float midpoint1 = ((servomaxpulse + servominpulse) / 2.0f);
		constexpr float deviation1 = ((servomaxpulse - servominpulse) / 2.0f);

		constexpr float minAngle = 0;
		constexpr float maxAngle = 180;
	}
	namespace motor {
		constexpr float clockFrequency = 8000000;
		constexpr float period = (1.0f / 100000.0f);
	}
	namespace motorcurrent {
		constexpr adc_positive_input input_motor0 = ADC_POSITIVE_INPUT_PIN1;
		constexpr adc_positive_input input_motor1 = ADC_POSITIVE_INPUT_PIN0;
	}
	namespace buzzer {
		constexpr float clockFrequency = 8000000;
		constexpr float period = 1 / 2400.0f;
	}
	namespace encoder {
		constexpr size_t bufferSize = 10;
		constexpr tcc_clock_prescaler prescaeSetting = TCC_CLOCK_PRESCALER_DIV1;
		constexpr gclk_generator clockSource = GCLK_GENERATOR_3;
		//1 second of no ticks results in an output of zero
		constexpr unsigned int zerotimeout = motor::clockFrequency * 1;
	}
	namespace viewerboard {
		constexpr size_t baudrate = 9600;
		constexpr size_t pendingQueueSize = 1;
		constexpr char const * taskName = "VRB";
		constexpr uint16_t taskStackDepth = 96;
		constexpr unsigned int taskPriority = 1;
		constexpr size_t transmitTimeout = 100;
	}
	namespace bms {
		constexpr char const * taskName = "BMS";
		constexpr uint16_t taskStackDepth = 64;
		constexpr unsigned int taskPriority = 1;
		constexpr size_t refreshRate = utility::conversions::msToTicks(100.0f);
	}
	namespace spimanager {
		constexpr size_t baudrate = 100000;
		constexpr char const * taskName = "SPI";
		constexpr uint16_t taskStackDepth = 64;
		constexpr unsigned int taskPriority = 1;
		constexpr size_t pendingQueueSize = 2;
	}
	namespace twimanager {
		constexpr char const * taskName = "TWI";
		constexpr uint16_t taskStackDepth = 64;
		constexpr unsigned int taskPriority = 1;
		constexpr size_t pendingQueueSize = 2;
	}
	namespace adcmanager {
		constexpr char const * taskName = "ADC";
		constexpr uint16_t taskStackDepth = 32;
		constexpr unsigned int taskPriority = 1;
		constexpr size_t pendingQueueSize = 2;
	}
	namespace manualserial {
		constexpr char const * taskName = "MAN";
		constexpr uint16_t taskStackDepth = 128;
		constexpr unsigned int taskPriority = 1;
	}
	namespace emc1701 {
		constexpr char const * taskName = "EMC";
		constexpr uint16_t taskStackDepth = 32;
		constexpr unsigned int taskPriority = 1;
		constexpr size_t refreshRate = utility::conversions::msToTicks(125.0f);

		constexpr float referenceVoltage = 0.01f;
		constexpr float currentResistor = 0.02f;
		constexpr uint8_t address_servo0 = 0b1001110;
		constexpr uint8_t address_servo1 = 0b1001111;
		constexpr uint8_t address_3v3 = 0b1001100;
		constexpr uint8_t address_5v = 0b1001101;
	}
	namespace pins {
		constexpr unsigned int servopower0 = PIN_PA07;
		constexpr unsigned int servopower1 = SERVO1_ENABLE_PIN;
		constexpr unsigned int redled = PIN_PA27;
		constexpr unsigned int greenled = PIN_PA28;
		constexpr unsigned int bms_ss0 = PIN_PA11;
		constexpr unsigned int bms_ss1 = PIN_PB08;
		constexpr unsigned int motor0 = PIN_PA18;
		constexpr unsigned int motor1 = PIN_PA22;
	}
}
