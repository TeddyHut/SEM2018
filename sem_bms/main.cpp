/*
 * Girton Grammar School Shell Eco Marathon Asia
 * 6-cell lithium-ion battery management system
 * This code is designed for an Atmel ATtiny816, to be compiled with AVR-GCC
 */

#define BMS_NUMBER 1
#define F_CPU 20000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/cpufunc.h>
#include <avr/wdt.h>
#include <util/delay_basic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

//Due to slight variation between components on boards, these values can be used as a calibration
#if (BMS_NUMBER == 1)
#define ERROR_OFFSET_CURRENT 2
#define ERROR_OFFSET_VCC 4
#define ERROR_OFFSET_TEMPERATURE 0
#elif (BMS_NUMBER == 2)
#define ERROR_OFFSET_CURRENT 2
#define ERROR_OFFSET_VCC 8
#define ERROR_OFFSET_TEMPERATURE 0
#endif


constexpr float CutoffCurrent = 5;
constexpr float CutoffTemperature = 60;
constexpr float CutoffVoltage = 3.15;
constexpr float MaximumVoltage = 4.3;
constexpr float KillVoltage = 3;

//Cycles before BMS determines that main board isn't connected
constexpr unsigned int Keepalive_maxCycles = 50;
//Number of cycles that there has to be an error for before firing
constexpr unsigned int ConsecutiveErrors = 2;
//The number of cycles after firing the relay before stopping current flow through the relay
constexpr unsigned int RelayTimeout_cycles = 50;

constexpr float vdiv_factor(float const r1, float const r2) {
	return r2 / (r1 + r2);
}

constexpr float vdiv_input(float const output, float const factor) {
	return output / factor;
}

template <typename adc_t, adc_t max>
constexpr float adc_voltage(adc_t const value, float const vref) {
	return vref * (static_cast<float>(value) / static_cast<float>(max));
}

template <typename adc_t, adc_t max>
constexpr adc_t adc_value(float const voltage, float const vref) {
	return (voltage / vref) * static_cast<float>(max);
}

float milliVoltsToDegreesC(float const m) {
	//Taken from temperature sensor datasheet
	float result = 2230.8 - m;
	result *= (4 * 0.00433);
	result += square(-13.582);
	result = 13.582 - sqrt(result);
	result /= (2 * -0.00433);
	return result + 30;
}

float calculateTemperature(uint16_t const value) {
	return milliVoltsToDegreesC(vdiv_input(adc_voltage<uint16_t, 1024>(value, 2.5f), vdiv_factor(5.1f, 7.5f)) * 1000.0f);
}

//Output value of the ACS711 current sensor for a particular current
constexpr float acs711_outV(float const current, float const vcc) {
	return (current * ((vcc - 0.6f) / (12.5f * 2.0f))) + (vcc / 2);
}

constexpr float acs711_current(float const outV, float const vcc) {
	return ((12.5f * 2.0f) / (vcc - 0.6f)) * (outV - (vcc / 2.0f));
}

constexpr float current_opamp_transformation_outV(float const inV, float const vcc) {
	return (inV * (5.0f / 6.0f)) - (vcc / 3.0f);
}

constexpr float current_opamp_transformation_inV(float const outV, float const vcc) {
	return (6.0f / 5.0f) * (outV + (vcc / 3.0f));
}

enum class Instruction : uint8_t {
	Normal = 0,
	ReceiveData,
	FireRelay,
	LEDOn,
	LEDOff,
};

enum class DataIndexes {
	TemperatureADC = 0,
	CurrentADC,
	Cell0ADC,
	Cell1ADC,
	Cell2ADC,
	Cell3ADC,
	Cell4ADC,
	Cell5ADC,
	_size,
};

enum LEDBlinkAmount {
	Blink_Button = 0,
	Blink_Temperature = 1,
	Blink_Voltage = 2,
	Blink_OverVoltage = 3,
	Blink_Current = 4,
};

constexpr ADC_MUXPOS_enum ADCMap[static_cast<unsigned int>(DataIndexes::_size)] = {
	ADC_MUXPOS_AIN9_gc,	//Temperature
	ADC_MUXPOS_AIN7_gc,	//Current
	ADC_MUXPOS_AIN1_gc,	//Cell0
	ADC_MUXPOS_AIN2_gc,	//Cell1
	ADC_MUXPOS_AIN3_gc,	//Cell2
	ADC_MUXPOS_AIN4_gc, //Cell3
	ADC_MUXPOS_AIN6_gc, //Cell4
	ADC_MUXPOS_AIN8_gc	//Cell5
};

//Used to store the (potential) cause of a relay firing. Will be written to eeprom if the relay fires
char str_triggerCause[16];

//Using a double buffer (*2 since values are 16 bit)
volatile uint8_t dataBuffer[2][(static_cast<unsigned int>(DataIndexes::_size) * 2) + (sizeof str_triggerCause / sizeof *str_triggerCause)];
volatile uint8_t dataBufferPos = 0;
volatile uint8_t dataBufferChoice = 0;

//Stores the amount of times the LED should blink if it is triggered
volatile LEDBlinkAmount cause_blinkAmount = Blink_Button;
//Number of cycles of the timer ISR
uint32_t totalCycles = 0;
//Used to determine whether main board is still communicating.
unsigned int keepalive_cycles = 0;
//Used so that current doesn't have to constantly be flowing through relay if it is triggered
unsigned int relaytimeout_cycles = 50;

//Set to true if the relay is fired. Used to determine whether or not setBlinkingLEDState should work or not
bool relay_fired = false;

uint16_t retreiveFromBuffer(volatile uint8_t const buffer[], DataIndexes const pos) {
	return (buffer[static_cast<unsigned int>(pos) * 2] |
	(buffer[static_cast<unsigned int>(pos) * 2 + 1] << 8));
}

void nvm_command(NVMCTRL_CMD_t const cmd) {
	//Unlock the NVM protection (works for 4 instructions)
	CCP = CCP_SPM_gc;
	NVMCTRL.CTRLA = cmd;
}

void writeCause(char const *const str_error) {
	//Wait for eeprom to not be busy
	while(NVMCTRL.STATUS & NVMCTRL_EEBUSY_bm);
	//Clear the NVM page buffer
	nvm_command(NVMCTRL_CMD_PAGEBUFCLR_gc);
	//Write values to eeprom mapped area
	strncpy(reinterpret_cast<char *>(EEPROM_START), str_error, sizeof str_triggerCause / sizeof *str_triggerCause);
	//Erase page, and write values to eeprom
	nvm_command(NVMCTRL_CMD_PAGEERASEWRITE_gc);
}

void setLEDState(bool const state) {
	if(state) PORTB.OUTSET = 1 << 0;
	else PORTB.OUTCLR = 1 << 0;
}

void setLEDBlinkingState(bool const state) {
	if(relay_fired == false) {
		if(state) {
			TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;
			TCA0.SINGLE.CTRLB |= TCA_SINGLE_CMP0_bm;
		}
		else {
			TCA0.SINGLE.CTRLA &= ~TCA_SINGLE_ENABLE_bm;
			TCA0.SINGLE.CTRLB &= ~TCA_SINGLE_CMP0_bm;
		}
	}
}

void setRelayState(bool const state, LEDBlinkAmount const blinkAmount = Blink_Button, char const *const str_error = nullptr) {
	if(state && !relay_fired) {
		//Set the relay state
		PORTA.OUTSET = 1 << 5;
		//Stop idle blinking
		setLEDBlinkingState(false);
		setLEDState(false);
		//Set the blink amount that caused to relay to fire (for ISR)
		cause_blinkAmount = blinkAmount;
		//Enable overflow interrupt
		TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
		//Everything should be fine at default settings (normal mode)
		TCA0.SINGLE.CTRLB = 0;
		//Set period to 10ms (interrupt will occur 10ms from now, doesn't really matter when it is). Prescale 1024 was already set in initialization
		TCA0.SINGLE.PER = (F_CPU / 1024) / 100;
		//Enable the timer
		TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;
		//Write cause to eeprom
		if(str_error != nullptr)
			writeCause(str_error);
	}
	else if (!state) {
		PORTA.OUTCLR = 1 << 5;
		setLEDState(false);
	}
	relay_fired = state;
}

//Calling this function with a false -result- for ConsecutiveErrors sampling cycles will cause the relay to trigger
void relayAssert(bool const result, LEDBlinkAmount const blinkAmount) {
	//Check that for the buffer the functions are being called from separate cycles (as opposed to multiple from the same cycle)
	static uint32_t lastCycle = 0;
	static unsigned int consecutiveErrors = 0;
	static bool wasSetLastCycle = false;
	static LEDBlinkAmount errorBlinkAmount = Blink_Button;
	static char str_error[16];
	if(!result) {
		strncpy(str_error, str_triggerCause, sizeof str_triggerCause / sizeof *str_triggerCause);
		errorBlinkAmount = blinkAmount;
	}

	if(lastCycle != totalCycles) {
		//A new cycle
		if(wasSetLastCycle && (consecutiveErrors++ == ConsecutiveErrors)) {
			relaytimeout_cycles = RelayTimeout_cycles;
			setRelayState(true, errorBlinkAmount, str_error);
		}
		else if (wasSetLastCycle == false)
			consecutiveErrors = 0;
		wasSetLastCycle = false;
		lastCycle = totalCycles;
	}
	wasSetLastCycle |= !result;
}

void button_update() {
	//Assert that the button is not held
	snprintf(str_triggerCause, sizeof str_triggerCause / sizeof *str_triggerCause, "Button Pressed");
	relayAssert(PORTB.IN & (1 << 3), Blink_Button);
}

void shutdown() {
	//Stop interrupts
	cli();
	//Turn off watchdog
	CCP = CCP_IOREG_gc;
	WDT.CTRLA = WDT_PERIOD_OFF_gc;
	//Turn off LED if on
	setLEDBlinkingState(false);
	setLEDState(false);
	//Disable isolator
	PORTB.OUTCLR = 1 << 2;
	//Fire relay
	setRelayState(true, Blink_Voltage, "Killed");
	//Wait for around 300ms
	for(uint8_t i = 0; i < 25; i++) {
		_delay_loop_2(0xffff);
		//Reset watchdog
	}
	setRelayState(false);
	//Set sleep mode to power down
	SLPCTRL.CTRLA = SLPCTRL_SMODE_PDOWN_gc | SLPCTRL_SEN_bm;
	//Go to sleep
	sleep_cpu();
	//Never to be woken again
}

ISR(TCA0_OVF_vect) {
	//Will be on for 2 seconds, and then blink quickly a certain amount of times depending on what the cause was
	static uint8_t blinksRemaining = 0;
	static bool previousState = true;
	//Clear the overflow flag
	TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
	//If all the blinks have occurred
	if(blinksRemaining == 0) {
		//Run for 2 seconds
		TCA0.SINGLE.PER = (F_CPU / 1024) * 2;
		blinksRemaining = cause_blinkAmount;
		previousState = true;
		setLEDState(true);
	}
	else {
		setLEDState(!previousState);
		//If the LED was just turned off
		if(previousState)
			blinksRemaining--;
		previousState = !previousState;
		//Wait for 200ms (1 second divided by 5)
		TCA0.SINGLE.PER = (F_CPU / 1024) / 5;
	}
}

//Interrupt where things are sampled (regular, every 10ms)	
ISR(TCB0_INT_vect) {
	//Clear the interrupt flag
	TCB0.INTFLAGS = TCB_CAPT_bm;
	//Reset the watchdog
	wdt_reset();
	//Check whether the BMS data has been retrieved within Keepalive_maxCycles
	totalCycles++;
	if(++keepalive_cycles >= Keepalive_maxCycles) {
		setLEDBlinkingState(true);
		//Such bad code, to prevent overflow
		keepalive_cycles--;
	}
	else
		setLEDBlinkingState(false);
	if(relaytimeout_cycles != 0) {
		if(--relaytimeout_cycles == 0)
			PORTA.OUTCLR = (1 << 5);
	}
	//Check button
	button_update();
	//Sample each ADC
	uint8_t const bufferChoice = (dataBufferChoice == 0 ? 1 : 0);
	for(uint8_t i = 0; i < static_cast<unsigned int>(DataIndexes::_size); i++) {
		//Different depending on the reference voltage, and also different depending on the board
		//TODO: Self calibrate
		int errorOffset = 0;
		//Clear and set reference voltages
		VREF.CTRLA &= ~VREF_ADC0REFSEL_gm;
		if(static_cast<DataIndexes>(i) == DataIndexes::TemperatureADC) {
			VREF.CTRLA |= VREF_ADC0REFSEL_2V5_gc;
			errorOffset = ERROR_OFFSET_TEMPERATURE;
		}
		else if(static_cast<DataIndexes>(i) == DataIndexes::CurrentADC) {
			VREF.CTRLA |= VREF_ADC0REFSEL_1V1_gc;
			errorOffset = ERROR_OFFSET_CURRENT;
		}
		else {
			VREF.CTRLA |= VREF_ADC0REFSEL_2V5_gc;
			errorOffset = ERROR_OFFSET_VCC;
		}
		ADC0.MUXPOS = ADCMap[i];
		ADC0.COMMAND = ADC_STCONV_bm;
		while(ADC0.COMMAND & ADC_STCONV_bm);
		//Store the result as lower byte -> higher byte
		volatile uint16_t const result = (ADC0.RES / 4) + errorOffset;	//Div because of the accumulator
		dataBuffer[bufferChoice][i * 2] = result & 0x00ff;
		dataBuffer[bufferChoice][i * 2 + 1] = (result & 0xff00) >> 8;
	}
	//volatile is so that values can be seen in debugger
	volatile uint16_t cell0val = retreiveFromBuffer(dataBuffer[bufferChoice], DataIndexes::Cell0ADC);
	volatile float vcc = vdiv_input(adc_voltage<uint16_t, 1024>(cell0val, 2.5f), vdiv_factor(10, 5.6));
	volatile uint16_t opamp_adcVal = retreiveFromBuffer(dataBuffer[bufferChoice], DataIndexes::CurrentADC);
	volatile float opamp_outV = adc_voltage<uint16_t, 1024>(opamp_adcVal, 1.1f);
	volatile float acs711OutV = current_opamp_transformation_inV(opamp_outV, vcc);
	volatile float current = acs711_current(acs711OutV, vcc);
	volatile float temperature = calculateTemperature(retreiveFromBuffer(dataBuffer[bufferChoice], DataIndexes::TemperatureADC));
	//Make sure that VCC is still high enough
	snprintf(str_triggerCause, sizeof str_triggerCause / sizeof *str_triggerCause, "Cell0 %-#3.2f", static_cast<double>(vcc));
	relayAssert(vcc > CutoffVoltage, Blink_Voltage);
	//Make sure that VCC is still low enough
	relayAssert(vcc < MaximumVoltage, Blink_OverVoltage);
	//If VCC gets less than 2.95V fire relay (should already be fired, but just in case), disable isolator, and put the processor into deepest sleep mode
	if(vcc <= KillVoltage) {
		shutdown();
	}
	//Make sure that all the other cells are still within conditions
	for(uint8_t i = 0; i < 5; i++) {
		volatile uint16_t adc_val = retreiveFromBuffer(dataBuffer[bufferChoice], static_cast<DataIndexes>(i + static_cast<uint8_t>(DataIndexes::Cell1ADC)));
		volatile float voltage = vdiv_input(adc_voltage<uint16_t, 1024>(adc_val, 2.5f), 5.6f / 10.0f);
		snprintf(str_triggerCause, sizeof str_triggerCause / sizeof *str_triggerCause, "Cell%u %-#3.2f", i + 1, static_cast<double>(voltage));
		//Protect against undervoltage
		relayAssert(voltage > CutoffVoltage, Blink_Voltage);
		//Protect against overvoltage
		relayAssert(voltage < MaximumVoltage, Blink_OverVoltage);
	}
	//Make sure that there is no over-current
	snprintf(str_triggerCause, sizeof str_triggerCause / sizeof *str_triggerCause, "Current %-#4.2f", static_cast<double>(current));
	relayAssert(current < CutoffCurrent, Blink_Current);
	//Make sure there is no over-temperature
	snprintf(str_triggerCause, sizeof str_triggerCause / sizeof *str_triggerCause, "Temp %-#5.2f", static_cast<double>(temperature));
	relayAssert(temperature < CutoffTemperature, Blink_Temperature);
	dataBufferChoice = bufferChoice;
}

//SPI interrupt
ISR(SPI0_INT_vect) {
	keepalive_cycles = 0;
	if(SPI0.INTFLAGS & SPI_RXCIF_bm) {
		uint8_t const data = SPI0.DATA;
		switch(static_cast<Instruction>(data)) {
		case Instruction::ReceiveData:
			dataBufferPos = 0;
			//Fallthrough intentional
		case Instruction::Normal:
			if(dataBufferPos >= (sizeof dataBuffer[0] / sizeof *(dataBuffer[0])))
				dataBufferPos = 0;
			SPI0.DATA = dataBuffer[dataBufferChoice][dataBufferPos++];
			break;
		case Instruction::FireRelay:
			snprintf(str_triggerCause, sizeof str_triggerCause / sizeof *str_triggerCause, "Instruction");
			setRelayState(true);
			break;
		case Instruction::LEDOn:
			setLEDState(true);
			break;
		case Instruction::LEDOff:
			setLEDState(false);
			break;
		default:
			break;
		}
	}
}

void setup() {
	//Set ports pin config
	PORTA.DIR = 0b00100000;
	PORTB.DIR = 0b00000111;
	PORTC.DIR = 0b00000010;
	//Don't switch relay yet
	PORTA.OUTCLR = 1 << 5;
	//Disable CCP protection of clock registers (works for 4 instructions)
	CCP = CCP_IOREG_gc;
	//Enable main clock as 20Mhz osc
	CLKCTRL.MCLKCTRLA |= CLKCTRL_CLKSEL_OSC20M_gc;
	CCP = CCP_IOREG_gc;
	//Disable prescaler
	CLKCTRL.MCLKCTRLB = 0;

	//Make sure that the processor didn't reset because of a watchdog condition, and if it did, shutdown the processor
	if(RSTCTRL.RSTFR & RSTCTRL_WDRF_bm)
		shutdown();

	//Disable CCP protection for watchdog register
	CCP = CCP_IOREG_gc;
	//Setup watchdog timer for 32ms
	WDT.CTRLA = WDT_PERIOD_32CLK_gc;
	//Reset watchdog 
	wdt_reset();

	//Set sleep mode to idle and enable sleep instruction
	SLPCTRL.CTRLA = SLPCTRL_SMODE_IDLE_gc | SLPCTRL_SEN_bm;

	//Enable alternative SPI0 pins
	PORTMUX.CTRLB |= PORTMUX_SPI0_ALTERNATE_gc;

	//Isolator enable
	PORTB.OUTSET = 1 << 2;
	//Button
	PORTB.PIN3CTRL = PORT_PULLUPEN_bm;

	//Take accumulated samples
	ADC0.CTRLB = ADC_SAMPNUM_ACC4_gc;
	//Increase capacitance and prescale
	ADC0.CTRLC = ADC_SAMPCAP_bm | ADC_PRESC_DIV32_gc;
	//64 cycle channel change delay
	ADC0.CTRLD = (0x3 << 5);
	//Increase sample time (might affect accuracy?)
	ADC0.SAMPCTRL = 0x10;
	//Setup ADC
	ADC0.CTRLA = ADC_RESSEL_10BIT_gc | ADC_ENABLE_bm;

	//Setup SPI
	SPI0.CTRLB = SPI_BUFEN_bm | SPI_BUFWR_bm | SPI_MODE_0_gc;
	SPI0.INTCTRL = SPI_RXCIE_bm; //| SPI_TXCIE_bm;
	SPI0.CTRLA = SPI_ENABLE_bm;
	//Set the SPI interrupt to be the highest priority so that it always responds to the master
	CPUINT.LVL1VEC = SPI0_INT_vect_num;

	//Set the CCMP (top) value for TCB0
	constexpr uint32_t TCB0Top = static_cast<uint32_t>((F_CPU / 2) / 200);
	static_assert(TCB0Top <= 0xffff, "TCB0 top too large");
	TCB0.CCMP = TCB0Top;
	//Run a check/data update every 5ms.
	TCB0.CTRLA = TCB_RUNSTDBY_bm | TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
	TCB0.INTCTRL = TCB_CAPT_bm;

	//Setup TCA to blink LED for 100ms every two seconds
	constexpr uint32_t TCA0Top = static_cast<uint32_t>((F_CPU / 1024) * 2);
	static_assert(TCA0Top <= 0xffff, "TCA0 top too large");
	constexpr uint32_t TCA0Compare = static_cast<uint32_t>((F_CPU / 1024) / 10);
	static_assert(TCA0Compare <= 0xffff, "TCA0Compare too large");
	//setLEDBlinkingState will switch TCA_SINGLE_CMP0_bm and TCA_SINGLE_ENABLE_bm
	TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP0_bm | TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
	TCA0.SINGLE.PER = TCA0Top;
	TCA0.SINGLE.CMP0 = TCA0Compare;
	TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1024_gc | TCA_SINGLE_ENABLE_bm;

	//Copy last trigger cause to end of both dataBuffers (const to cast away volatile, reinterpret to convert to char from unsigned char, static to convert from enum class to int)
	strncpy(const_cast<char *>(reinterpret_cast<char volatile *>(&(dataBuffer[0][static_cast<unsigned int>(DataIndexes::_size) * 2]))), reinterpret_cast<char *>(EEPROM_START), sizeof str_triggerCause / sizeof *str_triggerCause);
	strncpy(const_cast<char *>(reinterpret_cast<char volatile *>(&(dataBuffer[1][static_cast<unsigned int>(DataIndexes::_size) * 2]))), reinterpret_cast<char *>(EEPROM_START), sizeof str_triggerCause / sizeof *str_triggerCause);

	//Enable global interrupts
	sei();
}

int main(void)
{
	setup();
	while(true) {
		sleep_cpu();
	}
}
