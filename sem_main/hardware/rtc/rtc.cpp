/*
 * rtc.cpp
 *
 * Created: 1/03/2018 10:19:59 AM
 *  Author: teddy
 */ 

#include "rtc.h"

void RTCCount::init()
{
		rtc_count_config config;
		rtc_count_get_config_defaults(&config);
		config.clear_on_match = false;
		config.continuously_update = true;
		config.mode = RTC_COUNT_MODE_32BIT;
		//By default is clocked by the OSC32, div by 32 for 1024
		config.prescaler = RTC_COUNT_PRESCALER_DIV_32;
		rtc_count_init(&rtc_instance, RTC, &config);
		rtc_count_set_count(&rtc_instance, 0);
		rtc_count_enable(&rtc_instance);
}

float RTCCount::value()
{
	return rtc_count_get_count(&rtc_instance) / 1024.0f;
}
