/*
 * sem_bms.cpp
 *
 * Created: 22/01/2018 3:37:31 PM
 * Author : teddy
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdlib.h>
#include <math.h>

#define BMS_NUMBER 0
	
#if (BMS_NUMBER == 0)
#define ERROR_OFFSET_CURRENT 1
#define ERROR_OFFSET_VCC 8
#define ERROR_OFFSET_TEMPERATURE 0
#elif (BMS_NUMBER == 1)
#define ERROR_OFFSET_CURRENT 1
#define ERROR_OFFSET_VCC 4
#define ERROR_OFFSET_TEMPERATURE 0
#endif

constexpr float CutoffCurrent = 5;
constexpr float CutoffTemperature = 100;
constexpr float CutoffVoltage = 3;
constexpr unsigned int Keepalive_maxCycles = 50; //For 10ms ISR updates this should be 500ms.
//Number of cycles that there has to be an error for before firing
constexpr unsigned int ConsecutiveErrors = 10;
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
	float result = 2230.8 - m;
	result *= (4 * 0.00433);
	result += square(-13.528);
	result = 13.582 - sqrt(result);
	result /= (2 * -0.00433);
	return result + 30;
}

float calculateTemperature(uint16_t const value) {
	return milliVoltsToDegreesC(vdiv_input(adc_voltage<uint16_t, 1024>(value, 1.1f), vdiv_factor(5.1f, 7.5f)) * 1000.0f);
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

//Used to store what triggered the error
enum class Trigger : uint8_t {
	None = 0,
	Current,
	Voltage,
	Temperature,
} volatile trigger = Trigger::None;

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

//Using a double buffer (*2 since values are 16 bit)
volatile uint8_t dataBuffer[2][static_cast<unsigned int>(DataIndexes::_size) * 2];
volatile uint8_t dataBufferPos = 0;
volatile uint8_t dataBufferChoice = 0;

//Number of cycles of the timer ISR
uint32_t totalCycles = 0;
//Used to determine whether main board is still communicating.
unsigned int keepalive_cycles = 0;
//Used so that current doesn't have to constantly be flowing through relay if it is triggered
unsigned int relaytimeout_cycles = 50;

//Set to true if the relay is fired. Used to determine whether or not setBlinkingLEDState should work or not (LED should be permanently on if relay fired)
bool relay_fired = false;

uint16_t retreiveFromBuffer(volatile uint8_t const buffer[], DataIndexes const pos) {
	return (buffer[static_cast<unsigned int>(pos) * 2] |
	(buffer[static_cast<unsigned int>(pos) * 2 + 1] << 8));
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

void setRelayState(bool const state) {
	if(state && !relay_fired) {
		PORTA.OUTSET = 1 << 5;
		setLEDBlinkingState(false);
		setLEDState(true);
	}
	else if (!state) {
		PORTA.OUTCLR = 1 << 5;
		setLEDState(false);
	}
	relay_fired = state;
}

void relayAssert(bool const result, Trigger const cause) {
	//Check that for the buffer the functions are being called from separate cycles (as opposed to multiple from the same cycle)
	static uint32_t lastCycle = 0;
	static unsigned int consecutiveErrors = 0;
	static bool wasSetLastCycle = false;

	if(lastCycle != totalCycles) {
		//A new cycle
		if(wasSetLastCycle && (++consecutiveErrors == ConsecutiveErrors)) {
			trigger = cause;
			relaytimeout_cycles = RelayTimeout_cycles;
			setRelayState(true);
		}
		else if (wasSetLastCycle == false)
			consecutiveErrors = 0;
		wasSetLastCycle = false;
		lastCycle = totalCycles;
	}
	wasSetLastCycle |= !result;
}

void button_update() {
	//pullup, so values are inverted
	static bool previousState = true;
	bool currentState = PORTB.IN & (1 << 3);
	//If the button was released, reset the relay state
	if(previousState == false && currentState == true) {
		setRelayState(false);
	}
	previousState = currentState;
}

//Interrupt where things are sampled (regular, every 10ms)	
ISR(TCB0_INT_vect) {
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
	button_update();
	//Clear the interrupt flag
	TCB0.INTFLAGS = TCB_CAPT_bm;
	//Sample each ADC
	uint8_t const bufferChoice = (dataBufferChoice == 0 ? 1 : 0);
	for(uint8_t i = 0; i < static_cast<unsigned int>(DataIndexes::_size); i++) {
		//Different depending on the reference voltage, and also different depending on the board
		//TODO: Self calibrate
		int errorOffset = 0;
		//Clear and set reference voltages
		VREF.CTRLA &= ~VREF_ADC0REFSEL_gm;
		if(static_cast<DataIndexes>(i) == DataIndexes::TemperatureADC) {
			VREF.CTRLA |= VREF_ADC0REFSEL_1V1_gc;
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
		volatile uint16_t const result = (ADC0.RES / 1) + errorOffset;	//Div because of the accumulator
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
	relayAssert(vcc > CutoffVoltage, Trigger::Voltage);
	//Make sure that all the other cells are still high enough
	for(uint8_t i = 0; i < 5; i++) {
		volatile uint16_t adc_val = retreiveFromBuffer(dataBuffer[bufferChoice], static_cast<DataIndexes>(i + static_cast<uint8_t>(DataIndexes::Cell1ADC)));
		volatile float voltage = vdiv_input(adc_voltage<uint16_t, 1024>(adc_val, 2.5f), 5.6f / 10.0f);
		relayAssert(voltage > CutoffVoltage, Trigger::Voltage);
	}
	//Make sure that there is no over-current
	relayAssert(current < CutoffCurrent, Trigger::Current);
	//Check the temperature
	//relayAssert(temperature < CutoffTemperature);
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
			if(dataBufferPos >= static_cast<unsigned int>(DataIndexes::_size) * 2)
				dataBufferPos = 0;
			SPI0.DATA = dataBuffer[dataBufferChoice][dataBufferPos++];
			break;
		case Instruction::FireRelay:
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

	//Set sleep mode to idle and enable sleep instruction
	SLPCTRL.CTRLA = SLPCTRL_SMODE_IDLE_gc | SLPCTRL_SEN_bm;

	//Enable alternative SPI0 pins
	PORTMUX.CTRLB |= PORTMUX_SPI0_ALTERNATE_gc;

	//Isolator enable
	PORTB.OUTSET = 1 << 2;
	//Button
	PORTB.PIN3CTRL = PORT_PULLUPEN_bm;

	//Take accumulated samples
	ADC0.CTRLB = ADC_SAMPNUM_ACC1_gc;
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
	constexpr uint32_t TCB0Top = static_cast<uint32_t>((20000000 / 2) / 200);
	static_assert(TCB0Top <= 0xffff, "TCB0 top too large");
	TCB0.CCMP = TCB0Top;
	//Run a check/data update every 10ms.
	TCB0.CTRLA = TCB_RUNSTDBY_bm | TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
	TCB0.INTCTRL = TCB_CAPT_bm;

	//Setup TCA to blink LED for 100ms every two seconds
	constexpr uint32_t TCA0Top = static_cast<uint32_t>((20000000 / 1024) * 2);
	static_assert(TCA0Top <= 0xffff, "TCA0 top too large");
	constexpr uint32_t TCA0Compare = static_cast<uint32_t>((20000000 / 1024) / 10);
	static_assert(TCA0Compare <= 0xffff, "TCA0Compare too large");
	//setLEDBlinkingState will switch TCA_SINGLE_CMP0_bm and TCA_SINGLE_ENABLE_bm
	TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP0_bm | TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
	TCA0.SINGLE.PER = TCA0Top;
	TCA0.SINGLE.CMP0 = TCA0Compare;
	TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1024_gc | TCA_SINGLE_ENABLE_bm;

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
