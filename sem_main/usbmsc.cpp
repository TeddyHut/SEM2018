/*
 * usbmsc.cpp
 *
 * Created: 12/09/2018 2:14:07 PM
 *  Author: teddy
 */ 

#include <cstring>
#include <cstdlib>
#include <ctrl_access.h>
#include <usb.h>

#include "usbmsc.h"
#include "main_config.h"
#include "util.h"

extern "C" {
void usb_host_connection_event(uhc_device_t *const deviceInfo, bool const present)
{
	//Since the USB host has been set to a higher priority than the FreeRTOS kernel, that means we can't use FreeRTOS functions (from memory)
	if(present)
		USBMSC::wakeCause = USBMSC::WakeCause::Connected;
	else
		USBMSC::wakeCause = USBMSC::WakeCause::Disconnected;
}
}

TaskHandle_t USBMSC::task;

volatile USBMSC::WakeCause USBMSC::wakeCause = USBMSC::WakeCause::None;

void USBMSC::init()
{
	if(xTaskCreate(taskFunction, config::usbmsc::taskName, config::usbmsc::taskStackDepth, this, config::usbmsc::taskPriority, &task) == pdFAIL)
		debugbreak();
}

bool USBMSC::isReady() const
{
	return ready;
}

bool USBMSC::isError() const
{
	return iserror;
}

char const *const USBMSC::lastError() const
{
	return lasterror;
}

char const *const USBMSC::name() const
{
	return fileName;
}

void USBMSC::reload(int const settingsIndex /*= -1*/)
{
	if(settingsIndex == RELOAD_NEXT)
		this->settingsIndex++;
	else
		this->settingsIndex = settingsIndex;
	this->wakeCause = WakeCause::Reload;
}

void USBMSC::taskFunction(void *const usbmsc)
{
	static_cast<USBMSC *>(usbmsc)->task_main();
}

void USBMSC::task_main()
{
	uhc_start();
	while(true) {
		//Wait for USB device to connect
		while(true) {
			vTaskDelay(msToTicks(100));
			if(wakeCause != WakeCause::None) {
				iserror = false;
				ready = false;
				if (wakeCause == WakeCause::Disconnected) {
					wakeCause = WakeCause::None;
					continue;
				}
				break;
			}
		}
		//If it's a new connection
		if(wakeCause == WakeCause::Connected) {
			//This is needed for it to have time to get ready
			vTaskDelay(msToTicks(500));
			//When it connects, attempt to mount drive
			std::memset(&fs, 0, sizeof fs);
			if(FR_OK != f_mount(LUN_ID_USB, &fs)) {
				wakeCause = WakeCause::None;
				iserror = true;
				strcpy(lasterror, "mount failed");
				continue;
			}
		}
		//If the log file was already open, close it
		if(wakeCause == WakeCause::Reload) {
			f_close(&file);
		}
		wakeCause = WakeCause::None;

		//Attempt to open settings file
		FIL settingsFile;
		char settingsName[32];
		if(settingsIndex >= 0)
			rpl_snprintf(settingsName, sizeof settingsName, "/set%02i.txt", settingsIndex);
		else
			rpl_snprintf(settingsName, sizeof settingsName, "/settings.txt");
		auto res = f_open(&settingsFile, settingsName, FA_OPEN_EXISTING | FA_READ);
		if(res != FR_OK) {
			iserror = true;
			strcpy(lasterror, "open set failed");
			f_mount(LUN_ID_USB, &fs);
			continue;
		}

		//Read strings from settings file
		constexpr size_t strlen = 16;
		char sampleFrequency[strlen];
		char motorFrequency[strlen];
		char startupramptime[strlen];
		char coastramptime[strlen];
		char cruiseMin[strlen];
		char cruiseMax[strlen];
		char wheelRadius[strlen];
		char wheelSamplePoints[strlen];

		bool getserror = false;
		for(auto str : {sampleFrequency, motorFrequency, startupramptime,
				coastramptime, cruiseMin, cruiseMax,
				wheelRadius, wheelSamplePoints}) {
			if(nullptr == f_gets(str, strlen, &settingsFile)) {
				getserror = true;
				break;
			}
		}

		//Close the file
		f_close(&settingsFile);
		
		//There has been some sort of error
		if(getserror) {
			iserror = true;
			strcpy(lasterror, "gets failed");
			//Unmount the drive
			f_mount(LUN_ID_USB, &fs);
			continue;
		}

		//Fill out settings struct
		settings.sampleFrequency = std::strtof(sampleFrequency, nullptr);
		settings.motorFrequency = std::strtof(motorFrequency, nullptr);
		settings.startupramptime = std::strtof(startupramptime, nullptr);
		settings.coastramptime = std::strtof(coastramptime, nullptr);
		settings.cruiseMin = std::strtof(cruiseMin, nullptr);
		settings.cruiseMax = std::strtof(cruiseMax, nullptr);
		settings.wheelRadius = std::strtof(wheelRadius, nullptr);
		settings.wheelSamplePoints = std::atoi(wheelSamplePoints);

		//Keep attempting to open logNN files until we find a new one
		for(unsigned int index = 0; true; index++) {
			rpl_snprintf(fileName, sizeof fileName, "/log%02u.csv", index);
			auto res = f_open(&file, fileName, FA_CREATE_NEW | FA_WRITE);
			//File opened successfully
			if(res == FR_OK) {
				ready = true;
				break;
			}
			//Some other error that isn't it already existing
			else if (res != FR_EXIST) {
				iserror = true;
				strcpy(lasterror, "open failed");
				//Unmount drive
				f_mount(LUN_ID_USB, &fs);
			}
		}
	}
}
