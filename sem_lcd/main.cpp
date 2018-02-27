/*
 * sem_lcd.cpp
 *
 * Created: 16/01/2018 12:35:01 AM
 * Author : teddy
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "tedavr/include/tedavr/ic_hd44780.h"

#define INVERTED 1

namespace serial {
	//Transfer starts with 0xfe, ends with 0xff
	constexpr float baudRate = 9600;
	constexpr float ticksPerBit = (F_CPU / 8) / baudRate;
	constexpr float ticksPerHalfBit = ticksPerBit / 2;
	//Buffer to store received characters in
	volatile uint8_t buffer[64];
	//Position in buffer
	volatile uint8_t bufferpos = 0;
	//Transfer has completed
	volatile bool complete = false;
	uint8_t reverseByte (uint8_t x) {
		x = ((x >> 1) & 0x55) | ((x << 1) & 0xaa);
		x = ((x >> 2) & 0x33) | ((x << 2) & 0xcc);
		x = ((x >> 4) & 0x0f) | ((x << 4) & 0xf0);
		return x;
	}
}

enum class InputCommand : uint8_t {
	ClearDisplay = 1,
	SetPosition,
	StartBuzzer,
	StopBuzzer,
	LEDOn,
	LEDOff,
	BacklightOn,
	BacklightOff,
};

inline void setLEDState(bool const state) {
	if(state) PORTA |= _BV(4);
	else PORTA &= ~_BV(4);
}

inline void setBuzzerState(bool const state) {
	if(state) TCCR1B |= _BV(CS10);
	else TCCR1B &= ~_BV(CS10);
}

inline void setBacklightState(bool const state) {
	if(state) PORTA |= _BV(5);
	else PORTA &= ~_BV(5);
}

ISR(TIM1_OVF_vect) {
	PORTA |= _BV(7);
}

ISR(TIM1_COMPA_vect) {
	PORTA &= ~_BV(7);
}

ISR(PCINT0_vect) {
	//Pin has gone from high to low
#if (INVERTED == 0)
	if(!(PINA & _BV(6))) {
#else
	if(PINA & _BV(6)) {
#endif
		//Disable pin change interrupt on PCINT6
		PCMSK0 &= ~_BV(PCINT6);
		//Set the USI counter to 16 - 9 (will be overflowed after start bit + 8 data bits)
		USISR = 16 - 9;
		//Add the CPU cycles it took to get to here to the timer, also make it so that the start bit will be triggered at a half bit offset
		TCNT0 = serial::ticksPerHalfBit + 27;
		//Start the serial receive timer, 8 prescale
		TCCR0B |= _BV(CS01);
	}
}

ISR(USI_OVF_vect) {
	//Clear the interrupt flag
	USISR |= _BV(USIOIF);
	//Stop the counter
	TCCR0B &= ~0b111;
	//Store the contents of the data register in (reverse the byte since it's clocked in left-shifting)
	uint8_t recv = serial::reverseByte(USIDR);
#if (INVERTED == 1)
	recv = ~recv;
#endif
	//Start transfer/reset buffer
	if(recv == 0xfe) {
		serial::bufferpos = 0;
		serial::complete = false;
	}
	else if(recv == 0xff)
		serial::complete = true;
	else {
		serial::buffer[serial::bufferpos++] = recv;
		if(serial::bufferpos >= 64) {
			serial::bufferpos = 0;
		}
	}
	//Enable the pin change interrupt again, (won't trigger a new transfer on stop bit because start bit finishes high)
	PCMSK0 |= _BV(PCINT6);
}

int main(void)
{
	//---Setup ports---
	DDRA = 0b10111111;
	DDRB = 0b0111;
	PORTB = 0;
	PORTA = 0;

	//---Setup buzzer PWM---
	//Fast PWM, top ICR1
	TCCR1A |= _BV(WGM11);
	TCCR1B |= _BV(WGM13) | _BV(WGM12);
	//Enable compare match and overflow interrupts
	TIMSK1 |= _BV(OCIE1A) | _BV(TOIE1);
	//Set ICR1 to frequency, and OCR1A to 50% duty cycle
	ICR1 = F_CPU / 2400;
	OCR1A = (F_CPU / 2400) / 2;

	//---Setup serial receiver
	//Setup pin change interrupt on pin PCINT6 for detecting start condition
	PCMSK0 |= _BV(PCINT6);
	GIMSK |= _BV(PCIE0);
	//Setup timer0 to clock in the bits
	//CTC mode, OCRA as top
	TCCR0A |= _BV(WGM01);
	//TIMSK0 |= _BV(OCIE0A);
	OCR0A = static_cast<uint8_t>(serial::ticksPerBit);
	//Set the USI to clock data in on compare match of timer 0, and enable USI counter overflow interrupt
	USICR |= _BV(USICS0) | _BV(USIOIE);

	//---Setup the display---
	IC_HD44780 display;
	display.pin.ddr_data0 = &DDRA;
	display.pin.ddr_data1 = &DDRA;
	display.pin.ddr_data2 = &DDRA;
	display.pin.ddr_data3 = &DDRA;
	display.pin.port_in_data0 = &PINA;
	display.pin.port_in_data1 = &PINA;
	display.pin.port_in_data2 = &PINA;
	display.pin.port_in_data3 = &PINA;
	display.pin.port_out_data0 = &PORTA;
	display.pin.port_out_data1 = &PORTA;
	display.pin.port_out_data2 = &PORTA;
	display.pin.port_out_data3 = &PORTA;
	display.pin.port_out_en = &PORTB;
	display.pin.port_out_rs = &PORTB;
	display.pin.port_out_rw = &PORTB;
	display.pin.shift_data0 = 0;
	display.pin.shift_data1 = 1;
	display.pin.shift_data2 = 2;
	display.pin.shift_data3 = 3;
	display.pin.shift_en = 2;
	display.pin.shift_rs = 0;
	display.pin.shift_rw = 1;
	
	using namespace hd;
	display << instr::init_4bit;
	display << instr::function_set << function_set::datalength_4 << function_set::font_5x8 << function_set::lines_2;
	display << instr::display_power << display_power::cursor_off << display_power::cursorblink_off << display_power::display_on;
	display << instr::clear_display;
	display << "    SEM LCD\nConnect to PCB";
	
	//Default to backlight on
	setBacklightState(true);

	//---Enable interrupts
	sei();

	//Main program loop
	while(true) {
		//Go to sleep until interrupts (will resume after interrupt)
		sleep_mode();
		//Check for a transfer from the main board
		if(serial::complete) {
			serial::complete = false;
			//Hope that it doesn't send instructions too fast, and we can process the buffer before it changes (as opposed to copying the buffer)
			for(uint8_t i = 0; i < serial::bufferpos; i++) {
				switch(static_cast<InputCommand>(serial::buffer[i])) {
				case InputCommand::ClearDisplay:
					display << instr::clear_display;
					break;
				case InputCommand::SetPosition:
					display << instr::set_ddram_addr << serial::buffer[++i];
					break;
				case InputCommand::StartBuzzer:
					setBuzzerState(true);
					break;
				case InputCommand::StopBuzzer:
					setBuzzerState(false);
					break;
				case InputCommand::LEDOn:
					setLEDState(true);
					break;
				case InputCommand::LEDOff:
					setLEDState(false);
					break;
				case InputCommand::BacklightOn:
					setBacklightState(true);
				break;
				case InputCommand::BacklightOff:
					setBacklightState(false);
				break;
				case static_cast<InputCommand>('\n'):
					display << instr::set_ddram_addr << 40;
					break;
				default:
					display << instr::write << serial::buffer[i];
					break;
				}
			}
		}
	}
}
