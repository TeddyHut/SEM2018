/*
 * config.h
 *
 * Created: 18/01/2018 4:10:34 PM
 *  Author: teddy
 */ 

 //! NOTE TO SELF: Add encoder clock frequency

#pragma once

#if 1
#ifndef __cplusplus
#define vsnprintf rpl_vsnprintf
#define snprintf rpl_snprintf
#define vasprintf rpl_vasprintf
#define asprintf rpl_asprintf
#endif
#define HAVE_VSNPRINTF					0
#define HAVE_SNPRINTF					0
#define HAVE_VASPRINTF					0
#define HAVE_ASPRINTF					0
#define HAVE_STDARG_H					1
#define HAVE_STDDEF_H					1
#define HAVE_STDINT_H					1
#define HAVE_STDLIB_H					1
#define HAVE_FLOAT_H					1
#define HAVE_INTTYPES_H					1
#define HAVE_LOCALE_H					0
#define HAVE_LOCALECONV					0
#define HAVE_LCONV_DECIMAL_POINT		0
#define HAVE_LCONV_THOUSANDS_SEP		0
#define HAVE_LONG_DOUBLE				0
#define HAVE_LONG_LONG_INT				0
#define HAVE_UNSIGNED_LONG_LONG_INT		0
#define HAVE_INTMAX_T					1
#define HAVE_UINTMAX_T					1
#define HAVE_UINTPTR_T					1
#define HAVE_PTRDIFF_T					1
#define HAVE_VA_COPY					1
#define HAVE___VA_COPY					1
#define TEST_SNPRINTF					0
#define NEED_MEMCPY						0

#endif
#ifdef __cplusplus

#include <FreeRTOS.h>
#include <cstddef>
#include <cinttypes>
#include <tcc.h>

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
 		constexpr float servo0restposition = 72;
		constexpr float servo0engagedposition = 86;
		constexpr float servo1restposition = 105;
		constexpr float servo1engagedposition = 91;

		constexpr float minimumspeed = kmhToMs(22);
		constexpr float maximumspeed = kmhToMs(28);

		constexpr float ramuptime = 10;
		constexpr float servoengagetime = 0.1;
		constexpr float servodisengagetime = 0.1;
		constexpr float matchramptime = 5;
		constexpr float coastramptime = 10;
		constexpr float errorrange = 0.005;

		constexpr float alertvoltage = 3.2;
		constexpr float shutdownvoltage = 3.05;
		constexpr float alertbmscurrent = 5;
		constexpr float shutdownbmscurrent = 7;
		constexpr float alertmotorcurrent = 4;
		constexpr float shotdownmotorcurrent = 6;

		constexpr TickType_t displaycycleperiod = msToTicks(5000);
		constexpr TickType_t displayrefreshrate = msToTicks(100);
		constexpr TickType_t refreshRate = msToTicks(1000.0f / 60.0f);
		constexpr TickType_t speedmatchcorrecttimeout = msToTicks(2000);

		constexpr size_t buttonBufferSize = 10;
	}
	namespace hardware {
		constexpr float motortogeartickratio = 0.482; //Enoder0.getinterval / encoder2.getinterval
		constexpr float motorteeth = 12;
		constexpr float wheelradius = 0.43 / 2;
	}
	namespace adpcontrol {
		constexpr uint16_t taskStackDepth = 256;
		constexpr char const * taskName = "ADPControl";
		constexpr unsigned int taskPriority = 1;
	}
	namespace servo {
		constexpr float clockFrequency = 39999900;
		//48000000;
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
		constexpr float clockFrequency = 8000000;
		constexpr float period = (1.0f / 100000.0f);
	}
	namespace buzzermanager {
		constexpr size_t sequenceQueueSize = 2;
	}
	namespace ledmanager {
		constexpr size_t sequenceSqueueSize = 2;
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
}

#endif
