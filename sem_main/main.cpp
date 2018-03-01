/*
 * sem_main.cpp
 *
 * Created: 15/01/2018 11:27:40 AM
 * Author : teddy
 */ 


#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>

#include <asf.h>

#include "main_config.h"
#include "runtime/hardwareruntime/hardwareruntime.h"

void main_task(void *const param) {
	//Init runtime

#if (ENABLE_MANUALSERIAL == 1)
	ManualSerial manserial;
	manserial.init();
#endif

	hardwareruntime::init();
	while(true);
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
	port_config outconfig;
	port_get_config_defaults(&outconfig);
	outconfig.direction = PORT_PIN_DIR_OUTPUT;
	//Motors
	port_pin_set_config(config::pins::motor0, &outconfig);
	port_pin_set_config(config::pins::motor1, &outconfig);
	port_pin_set_output_level(config::pins::motor0, false);
	port_pin_set_output_level(config::pins::motor1, false);

	//Servos
	port_pin_set_config(config::pins::servopower0, &outconfig);
	port_pin_set_config(config::pins::servopower1, &outconfig);
	port_pin_set_output_level(config::pins::servopower0, false);
	port_pin_set_output_level(config::pins::servopower1, false);

	TaskHandle_t task;
	xTaskCreate(main_task, "main", 512, nullptr, 1, &task);

	vTaskStartScheduler();
	//debugbreak();
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
	
	//Stop movement
	//stopmovement();

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