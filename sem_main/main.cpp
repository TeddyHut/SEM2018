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
#include "manualserial.h"
#include "runmanagement.h"
#include "emc1701.h"

//"Beep beep" at startup
void buzzerStartup(Buzzer &buz) {
	for(uint8_t i = 0; i < 2; i++) {
		buz.start();
		vTaskDelay(msToTicks(100));
		buz.stop();
		vTaskDelay(msToTicks(75));
	}
}

//LED blinks at startup
void ledStartup(LED &led) {
	for(uint8_t i = 0; i < 2; i++) {
		led.setLEDState(true);
		vTaskDelay(msToTicks(100));
		led.setLEDState(false);
		vTaskDelay(msToTicks(75));
	}
}

void servofinished_beep(Buzzer &buz) {
	buz.start();
	vTaskDelay(msToTicks(100));
	buz.stop();
}

void aliveBlink(LED &buz) {
	buz.setLEDState(true);
	vTaskDelay(msToTicks(100));
	buz.setLEDState(false);
}

void servo0_startup_callback(TimerHandle_t timer) {
	//Servos are initially high impedance and can rely on the pullup resister to stop the fet from triggering
	port_config config;
	port_get_config_defaults(&config);
	config.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(PIN_PA07, &config);
	port_pin_set_output_level(PIN_PA07, false);
	runtime::buzzermanager->registerSequence(servofinished_beep);
}

void servo1_startup_callback(TimerHandle_t timer) {
	port_config config;
	port_get_config_defaults(&config);
	config.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(PIN_PA09, &config);
	port_pin_set_output_level(PIN_PA09, false);
	runtime::buzzermanager->registerSequence(servofinished_beep);
}

void activeCallback(TimerHandle_t timer) {
	runtime::vbLEDmanager->registerSequence(aliveBlink);
}

void main_task(void *const param) {
	//Init runtime
	runtime::init();
	runtime::dep_init();
	
	//Register startup sequence
	runtime::buzzermanager->registerSequence(buzzerStartup);
	runtime::vbBuzzermanager->registerSequence(buzzerStartup);
	runtime::greenLEDmanager->registerSequence(ledStartup);
	runtime::redLEDmanager->registerSequence(ledStartup);
	runtime::vbLEDmanager->registerSequence(ledStartup);
	runtime::viewerboard->clearDisplay();
	runtime::viewerboard->send();

	//Put motors and servos in default positions
	runtime::motor0->setDutyCycle(0);
	runtime::motor1->setDutyCycle(0);
	runtime::servo0->setPosition(config::run::servo0restposition);
	runtime::servo1->setPosition(config::run::servo1restposition);

	ManualSerial manserial;
	manserial.init();
	
	xTimerStart(xTimerCreate("aliveBlink", msToTicks(5000), pdTRUE, 0, activeCallback), portMAX_DELAY);
	xTimerStart(xTimerCreate("servo0startup", msToTicks(5000), pdFALSE, 0, servo0_startup_callback), portMAX_DELAY);
	xTimerStart(xTimerCreate("servo1startup", msToTicks(6000), pdFALSE, 0, servo1_startup_callback), portMAX_DELAY);

	runmanagement::run();
}

//Enable program counter trace
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

int main(void)
{
	InitTraceBuffer();
	system_init();

	//Set the motor and servo GPIO pins to outputs and pull them down (otherwise motors run for a little at startup)
	port_config config;
	port_get_config_defaults(&config);
	config.direction = PORT_PIN_DIR_OUTPUT;
	//Motors
	port_pin_set_config(PIN_PA18, &config);
	port_pin_set_config(PIN_PA22, &config);

	//Servos
	config.direction = PORT_PIN_DIR_INPUT;
	config.input_pull = PORT_PIN_PULL_NONE;
	port_pin_set_config(PIN_PA07, &config);
	port_pin_set_config(PIN_PA09, &config);

	port_pin_set_output_level(PIN_PA18, false);
	port_pin_set_output_level(PIN_PA22, false);

	TaskHandle_t task;
	xTaskCreate(main_task, "main", 512, nullptr, 1, &task);

	vTaskStartScheduler();
	debugbreak();
	while(true);
}


//Hardfault debugging (not made by me)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
extern "C" {
void HardFault_HandlerC(unsigned long *hardfault_args);

void HardFault_HandlerC(unsigned long *hardfault_args){
	volatile unsigned long stacked_r0 ;
	volatile unsigned long stacked_r1 ;
	volatile unsigned long stacked_r2 ;
	volatile unsigned long stacked_r3 ;
	volatile unsigned long stacked_r12 ;
	volatile unsigned long stacked_lr ;
	volatile unsigned long stacked_pc ;
	volatile unsigned long stacked_psr ;
	volatile unsigned long _CFSR ;
	volatile unsigned long _HFSR ;
	volatile unsigned long _DFSR ;
	volatile unsigned long _AFSR ;
	volatile unsigned long _BFAR ;
	volatile unsigned long _MMAR ;
	
	stacked_r0 = ((unsigned long)hardfault_args[0]) ;
	stacked_r1 = ((unsigned long)hardfault_args[1]) ;
	stacked_r2 = ((unsigned long)hardfault_args[2]) ;
	stacked_r3 = ((unsigned long)hardfault_args[3]) ;
	stacked_r12 = ((unsigned long)hardfault_args[4]) ;
	stacked_lr = ((unsigned long)hardfault_args[5]) ;
	stacked_pc = ((unsigned long)hardfault_args[6]) ;
	stacked_psr = ((unsigned long)hardfault_args[7]) ;
	
	// Configurable Fault Status Register
	// Consists of MMSR, BFSR and UFSR
	_CFSR = (*((volatile unsigned long *)(0xE000ED28))) ;
	
	// Hard Fault Status Register
	_HFSR = (*((volatile unsigned long *)(0xE000ED2C))) ;
	
	// Debug Fault Status Register
	_DFSR = (*((volatile unsigned long *)(0xE000ED30))) ;
	
	// Auxiliary Fault Status Register
	_AFSR = (*((volatile unsigned long *)(0xE000ED3C))) ;
	
	// Read the Fault Address Registers. These may not contain valid values.
	// Check BFARVALID/MMARVALID to see if they are valid values
	// MemManage Fault Address Register
	_MMAR = (*((volatile unsigned long *)(0xE000ED34))) ;
	// Bus Fault Address Register
	_BFAR = (*((volatile unsigned long *)(0xE000ED38))) ;
	
	__asm("BKPT #0\n") ; // Break into the debugger
}

void NMI_Handler             ( void ) { while(1); }
__attribute__((naked))
void HardFault_Handler       ( void ) {
	__asm volatile (
    " movs r0,#4       \n"
    " movs r1, lr      \n"
    " tst r0, r1       \n"
    " beq _MSP         \n"
    " mrs r0, psp      \n"
    " b _HALT          \n"
  "_MSP:               \n"
    " mrs r0, msp      \n"
  "_HALT:              \n"
    " ldr r1,[r0,#20]  \n"
    " b HardFault_HandlerC \n"
    " bkpt #0          \n"
  );
}
void SVC_Handler             ( void ) { while(1); }
//void PendSV_Handler          ( void ) { while(1); }
//void SysTick_Handler         ( void ) { while(1); }
}

#pragma GCC diagnostic pop