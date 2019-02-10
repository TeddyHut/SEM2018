/*
 * rundisplay.cpp
 *
 * Created: 2/02/2018 12:09:50 AM
 *  Author: teddy
 */ 

#include "rundisplay.h"
#include "instance.h"

template <typename T>
constexpr T enumCycle(T const v) {
	static_assert(std::is_enum<T>::value, "T must be of enum type");
	T next = static_cast<T>(static_cast<std::underlying_type_t<T>>(v) + 1);
	if(next == T::_size)
	next = static_cast<T>(0);
	return next;
}

void program::Display::printDisplay(Input const &input) const
{
	char str[17];
	char str2[sizeof str];
	memset(str, 0, 17);
	runtime::viewerboard->setPosition(0);
	topline->get_text(str, input);
	rpl_snprintf(str2, sizeof str2, "%-16s", str);
	runtime::viewerboard->writeText(str2, 16);
	memset(str, 0, 17);
	runtime::viewerboard->setPosition(40);
	bottomline->get_text(str, input);
	rpl_snprintf(str2, sizeof str2, "%-16s", str);
	runtime::viewerboard->writeText(str2, 16);
	runtime::viewerboard->send();
}

program::TimerUpdate::TimerUpdate(TickType_t const period /*= config::run::displaycycleperiod*/)
{
	if((timer = xTimerCreate("line", period, pdTRUE, this, callback)) == NULL)
		debugbreak();
	xTimerStart(timer, portMAX_DELAY);
}

program::TimerUpdate::~TimerUpdate()
{
	if(timer != NULL)
		xTimerDelete(timer, portMAX_DELAY);
}

program::DisplayLine::DisplayLine(ID const id /*= ID::None*/) : id(id) {}


program::DL_Idle_Top::DL_Idle_Top(int const &countdown) : DisplayLine(ID::Idle), countdown(countdown) {}
void program::DL_Idle_Top::get_text(char str[], Input const &input)
{
	if(countdown == -1)
		strcpy(str, "     Ready      ");
	else
		rpl_snprintf(str, 17, "Starting in:   %i.1", countdown);
}

void program::TimerUpdate::callback(TimerHandle_t timer)
{
	static_cast<TimerUpdate *>(pvTimerGetTimerID(timer))->cycle();
}


program::DL_Battery::DL_Battery() : DisplayLine(ID::Battery) {}
void program::DL_Battery::get_text(char str[], Input const &input)
{
	switch(curcycle) {
	case Cycle::Current:
		rpl_snprintf(str, 17, "BC1 %4.2f C2 %4.2f", std::max(input.bms0data.current, 0.0f), std::max(input.bms1data.current, 0.0f));
		break;
	case Cycle::Voltage:
		rpl_snprintf(str, 17, "BV1 %#4.1f V2 %#4.1f", input.bms0data.voltage, input.bms1data.voltage);
		break;
	case Cycle::AvgCellVoltage:
		rpl_snprintf(str, 17, "BA1 %#4.2f A2 %#4.2f", input.bms0data.voltage / input.bms0data.cellVoltage.size(), input.bms1data.voltage / input.bms1data.cellVoltage.size());
		break;
	case Cycle::Temperature:
		rpl_snprintf(str, 17, "BT1 %#4.1f T2 %#4.1f", input.bms0data.temperature, input.bms1data.temperature);
	case Cycle::_size:
		break;
	}
}
void program::DL_Battery::cycle()
{
	curcycle = enumCycle(curcycle);
}


program::DL_SpeedEnergy::DL_SpeedEnergy() : DisplayLine(ID::SpeedEnergy) {}
void program::DL_SpeedEnergy::get_text(char str[], Input const &input)
{
	switch(curcycle) {
	case Cycle::SpeedEnergy:
		rpl_snprintf(str, 17, "%#5.2fkmh %#5.2fWh", msToKmh(input.vehicleSpeedMs), input.totalEnergyUsage);
		break;
	case Cycle::Time:
		rpl_snprintf(str, 17, "Runtime:   %02u:%02u", static_cast<unsigned int>(input.startTime) / 60, static_cast<unsigned int>(input.startTime) % 60);
		break;
	case Cycle::_size:
		break;
	}
}
void program::DL_SpeedEnergy::cycle()
{
	curcycle = enumCycle(curcycle);
}


program::DL_Finished::DL_Finished(float const time, float const energy, float const distance) : DisplayLine(ID::Finished), time(time), energy(energy), distance(distance) {}
void program::DL_Finished::get_text(char str[], Input const &input)
{
	switch(curCycle) {
	case Cycle::Finished:
		rpl_snprintf(str, 17, "    Finished    ");
		break;
	case Cycle::Time:
		rpl_snprintf(str, 17, "Time:      %02u:%02u", static_cast<unsigned int>(time) / 60, static_cast<unsigned int>(time) % 60);
		break;
	case Cycle::Energy:
		rpl_snprintf(str, 17, "Energy:  %#5.2fWh", energy);
		break;
	case Cycle::Distance:
		rpl_snprintf(str, 17, "Distance: %#6.3f", distance);
		break;
	case Cycle::_size:
		break;
	}
}
void program::DL_Finished::cycle()
{
	curCycle = enumCycle(curCycle);
}


program::DL_USB::DL_USB() : DisplayLine(ID::USSB) {}
void program::DL_USB::get_text(char str[], Input const &input)
{
	//If there is a USB error, show that
	if(runtime::usbmsc->isError()) {
		rpl_snprintf(str, 17, "%-16s", runtime::usbmsc->lastError());
	}
	//If USB hasn't been connected yet, show "Connect USB"
	else {
		rpl_snprintf(str, 17, "Connect USB");
	}
}


program::DL_Settings::DL_Settings() : DisplayLine(ID::Settings), TimerUpdate(msToTicks(1500)) {}
void program::DL_Settings::get_text(char str[], Input const &input)
{
	switch(curcycle) {
	case Cycle::File:
		rpl_snprintf(str, 17, "File: %10s", runtime::usbmsc->name());
		break;
	case Cycle::Speeds:
		rpl_snprintf(str, 17, "Low %3.1f Hi %3.1f", runtime::usbmsc->settings.cruiseMin, runtime::usbmsc->settings.cruiseMax);
		break;
	case Cycle::Times:
		rpl_snprintf(str, 17, "St %3.0fs Cst %3.0fs", runtime::usbmsc->settings.startupramptime, runtime::usbmsc->settings.coastramptime);
		break;
	case Cycle::PWMFrequency:
		rpl_snprintf(str, 17, "PWMFreq: %5.0fHz", runtime::usbmsc->settings.motorFrequency);
		break;
	case Cycle::SampleFrequency:
		rpl_snprintf(str, 17, "SplFreq: %5.2fHz", runtime::usbmsc->settings.sampleFrequency);
		break;
	case Cycle::_size:
		break;
	}
}
void program::DL_Settings::cycle()
{
	curcycle = enumCycle(curcycle);
}


program::DL_Ramping::DL_Ramping() : DisplayLine(ID::Ramping) {}
void program::DL_Ramping::get_text(char str[], Input const &input)
{
	switch(curcycle) {
		case Cycle::MotorCurrent:
			rpl_snprintf(str, 17, "Current: %6.2fA", std::max(0.0f, input.bms0data.current));
			break;
		case Cycle::DutyCycle:
			rpl_snprintf(str, 17, "Duty: %9.1f%%", input.motorDutyCycle * 100.0f);
			break;
		case Cycle::Rampspeed:
			rpl_snprintf(str, 17, "MtrRPM: %8.1f", input.motorRPS * 60.0f);
			break;
		case Cycle::_size:
			break;
	}
}
void program::DL_Ramping::cycle()
{
	curcycle = enumCycle(curcycle);
}


program::DL_Coasting::DL_Coasting() : DisplayLine(ID::Coasting) {}
void program::DL_Coasting::get_text(char str[], Input const &input)
{
	if(firstGetText) {
		firstGetText = false;
		starttime = input.time;
	}
	switch(curcycle) {
		case Cycle::BatteryVoltage:
			rpl_snprintf(str, 17, "BV1 %#4.1f V2 %#4.1f", input.bms0data.voltage, input.bms1data.voltage);
			break;
		case Cycle::CoastTime:
			rpl_snprintf(str, 17, "Coasting:  %2.2u:%2.2u", static_cast<unsigned int>(input.time - starttime) / 60, static_cast<unsigned int>(input.time - starttime) % 60);
			break;
		case Cycle::Distance:
			rpl_snprintf(str, 17, "Distance: %#6.3f", input.startDistance);
			break;
		case Cycle::Samples:
			rpl_snprintf(str, 17, "Samples: %7u", input.samples);
			break;
		case Cycle::_size:
			break;
	}
}
void program::DL_Coasting::cycle()
{
	curcycle = enumCycle(curcycle);
}
