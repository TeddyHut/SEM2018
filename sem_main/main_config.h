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

#define PATCH_ENCODER0_TO_ENCODER2 1

#define ENABLE_MANUALSERIAL 0
#define ENABLE_ADPCONTROL 0

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
 		constexpr float servo0restposition = 90;
		constexpr float servo0engagedposition = 108;
		constexpr float servo1restposition = 136;
		constexpr float servo1engagedposition = 118;

		constexpr float minimumspeed = kmhToMs(22);
		constexpr float speedMatchSpeed = kmhToMs(24.5);
		constexpr float maximumspeed = kmhToMs(27);

		constexpr float ramuptime = 10;
		constexpr float servoengagetime = 0.1;
		constexpr float servodisengagetime = 0.1;
		constexpr float stoptimeout = 5;
		constexpr float matchramptime = 1.5;
		constexpr float coastramptime = 10;
		constexpr float errorrange = 0.15f;

		constexpr float alertvoltage = 3.2;
		constexpr float shutdownvoltage = 3.05;
		constexpr float alertbmscurrent = 5;
		constexpr float shutdownbmscurrent = 7;
		constexpr float alertmotorcurrent = 4;
		constexpr float shotdownmotorcurrent = 6;

		constexpr TickType_t displaycycleperiod = msToTicks(5000);
		constexpr TickType_t displayrefreshrate = msToTicks(100);
		constexpr TickType_t refreshRate = msToTicks(1000.0f / 120.0f);
		constexpr float loginterval = 1.0f / 45.0f;
		constexpr float syncinterval = 1.0f / 1.0f;
		constexpr float testRestTime = 5.0f;
		constexpr float speedmatchcorrecttimeout = 1.5f;

		constexpr size_t buttonBufferSize = 10;
	}
	namespace hardware {
		constexpr float motortogeartickratio = 0.482; //Enoder0.getinterval / encoder2.getinterval
		constexpr float motorteeth = 1;//12;
	}
	namespace adpcontrol {
		constexpr uint16_t taskStackDepth = 256;
		constexpr char const * taskName = "ADPControl";
		constexpr unsigned int taskPriority = 1;
	}
	namespace servo {
		constexpr float clockFrequency = 48000000;
		constexpr float period = 0.02;
		constexpr float servominpulse = 0.000544;
		constexpr float servomaxpulse = 0.0024;
		constexpr float midpoint0 = ((servomaxpulse + servominpulse) / 2.0f);
		constexpr float deviation0 = ((servomaxpulse - servominpulse) / 2.0f);
		constexpr float midpoint1 = ((servomaxpulse + servominpulse) / 2.0f);
		constexpr float deviation1 = ((servomaxpulse - servominpulse) / 2.0f);
		
		//For old servos:
		/*
		constexpr float midpoint0 = ((0.00155 + 0.001125) / 2.0f);
		constexpr float deviation0 = ((0.00155 - 0.001125) / 2.0f);
		constexpr float midpoint1 = ((0.0008 + 0.00048) / 2.0f);
		constexpr float deviation1 =((0.0008 - 0.00048) / 2.0f);
		*/
		constexpr float minAngle = 0;
		constexpr float maxAngle = 180;
		constexpr float restAngle = 0;
		constexpr float engagedAngle = 0;
	}
	namespace motor {
		constexpr unsigned int motor0_pin = PIN_PA18;
		constexpr unsigned int motor1_pin = PIN_PA22;
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
		constexpr size_t pendingQueueSize = 2;
	}
	namespace twimanager {
		constexpr char const * taskName = "TWIManager";
		constexpr uint16_t taskStackDepth = 64;
		constexpr unsigned int taskPriority = 1;
		constexpr size_t pendingQueueSize = 2;
	}
	namespace led {
		constexpr unsigned int greenLED_pin = PIN_PA28;
		constexpr unsigned int redLED_pin = PIN_PA27;
	}
	namespace manualserial {
		constexpr char const * taskName = "ManualSerial";
		constexpr uint16_t taskStackDepth = 256;
		constexpr unsigned int taskPriority = 1;
	}
	namespace emc1701 {
		constexpr char const * taskName = "EMC1701";
		constexpr uint16_t taskStackDepth = 32;
		constexpr unsigned int taskPriority = 1;
		constexpr size_t refreshRate = msToTicks(125);

		constexpr float referenceVoltage = 0.01f;
		constexpr float currentResistor = 0.02f;
		constexpr uint8_t address_servo0 = 0b1001110;
		constexpr uint8_t address_servo1 = 0b1001111;
		constexpr uint8_t address_3v3 = 0b1001100;
		constexpr uint8_t address_5v = 0b1001101;
	}
	namespace servopower {
		constexpr unsigned int servo_regulatorEnable_pin = PIN_PA23;
		constexpr unsigned int servo0_power_pin = PIN_PA07;
		constexpr unsigned int servo1_power_pin = SERVO1_ENABLE_PIN;
	}
	namespace usbmsc {
		constexpr char const * taskName = "USBMSC";
		constexpr uint16_t taskStackDepth = 512;
		constexpr unsigned int taskPriority = 1;
	}
}
