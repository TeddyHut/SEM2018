/*
 * sem_main.cpp
 *
 * Created: 15/01/2018 11:27:40 AM
 * Author : teddy
 */ 


#include <asf.h>
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <cstdio>
#include <array>
#include "config.h"
#include "instance.h"
#include "dep_instance.h"

void seq(Buzzer &buz) {
	for(uint8_t i = 0; i < 2; i++) {
		buz.start();
		vTaskDelay(msToTicks(100));
		buz.stop();
		vTaskDelay(msToTicks(75));
	}
}

void ledSeq(LED &led) {
	for(uint8_t i = 0; i < 2; i++) {
		led.setLEDState(true);
		vTaskDelay(msToTicks(100));
		led.setLEDState(false);
		vTaskDelay(msToTicks(75));
	}
}

void buzzerSingle(LED &buz) {
	buz.setLEDState(true);
	vTaskDelay(msToTicks(100));
	buz.setLEDState(false);
}

#define IS_MTB_ENABLED \
REG_MTB_MASTER & MTB_MASTER_EN
#define DISABLE_MTB \
REG_MTB_MASTER = REG_MTB_MASTER & ~MTB_MASTER_EN
#define ENABLE_MTB \
REG_MTB_MASTER = REG_MTB_MASTER | MTB_MASTER_EN

__attribute__((aligned(1024)))
volatile char __tracebuffer__[1024];
volatile int __tracebuffersize__ = sizeof(__tracebuffer__);
void InitTraceBuffer()
{
	int index = 0;
	uint32_t mtbEnabled = IS_MTB_ENABLED;
	DISABLE_MTB;
	for(index =0; index<1024; index++)
	{
		__tracebuffer__[index];
		__tracebuffersize__;
	}
	if(mtbEnabled)
	ENABLE_MTB;
}

void activeCallback(TimerHandle_t timer) {
	runtime::vbLEDmanager->registerSequence(buzzerSingle);
}

void main_task(void *const param) {
	runtime::init();
	runtime::dep_init();
	runtime::buzzermanager->registerSequence(seq);
	runtime::vbBuzzermanager->registerSequence(seq);
	runtime::redLEDmanager->registerSequence(ledSeq);
	runtime::greenLEDmanager->registerSequence(ledSeq);
	runtime::vbLEDmanager->registerSequence(ledSeq);
	runtime::viewerboard->clearDisplay();
	runtime::viewerboard->send();
	TimerHandle_t activateBeep = xTimerCreate("beep", msToTicks(5000), pdTRUE, 0, activeCallback);
	xTimerStart(activateBeep, portMAX_DELAY);
	TickType_t previousWakeTime = xTaskGetTickCount();
	while(true) {
		auto data = runtime::bms0->get_data();
		std::string stroutbuf(32, '\0');
		volatile float vcc = data.cellVoltage[0];
		size_t len = std::snprintf(const_cast<char *>(stroutbuf.data()), stroutbuf.size(), "BMS0: %f V", data.cellVoltage[0]);
		stroutbuf.resize(len);
		runtime::viewerboard->setPosition(0);
		runtime::viewerboard->writeText(stroutbuf);
		runtime::viewerboard->send();
		vTaskDelayUntil(&previousWakeTime, msToTicks(config::bms::refreshRate));
	}
}

int main(void)
{
	InitTraceBuffer();
	system_init();
	//system_interrupt_enable_global();

	TaskHandle_t task;
	//volatile size_t hp = xPortGetFreeHeapSize();
	xTaskCreate(main_task, "main", 256, nullptr, 1, &task);

	vTaskStartScheduler();
	debugbreak();
	while(true);
}
