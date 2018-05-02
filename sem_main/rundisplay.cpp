/*
 * rundisplay.cpp
 *
 * Created: 2/02/2018 12:09:50 AM
 *  Author: teddy
 */ 

#include "rundisplay.h"

template <typename T>
constexpr T enumCycle(T const v) {
	static_assert(std::is_enum<T>::value, "T must be of enum type");
	T next = static_cast<T>(static_cast<std::underlying_type_t<T>>(v) + 1);
	if(next == T::_size)
	next = static_cast<T>(0);
	return next;
}

Run::TimerUpdate::TimerUpdate(TickType_t const period /*= config::run::displaycycleperiod*/)
{
	if((timer = xTimerCreate("line", period, pdTRUE, this, callback)) == NULL)
	debugbreak();
	xTimerStart(timer, portMAX_DELAY);
}

Run::TimerUpdate::~TimerUpdate()
{
	if(timer != NULL)
	xTimerDelete(timer, portMAX_DELAY);
}

 Run::DL_Idle_Top::DL_Idle_Top(int const &countdown) : countdown(countdown)
{
}

void Run::DL_Idle_Top::get_text(char str[], Input const &input)
{
	if(countdown == -1)
		strcpy(str, "     Ready      ");
	else
		rpl_snprintf(str, 17, "Starting in:   %i.1", countdown);
}

void Run::TimerUpdate::callback(TimerHandle_t timer)
{
	static_cast<TimerUpdate *>(pvTimerGetTimerID(timer))->cycle();
}

void Run::DL_Battery::get_text(char str[], Input const &input)
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

void Run::DL_Battery::cycle()
{
	curcycle = enumCycle(curcycle);
}

void Run::DL_SpeedEnergy::get_text(char str[], Input const &input)
{
	switch(curcycle) {
		case Cycle::SpeedEnergy:
		rpl_snprintf(str, 17, "%#5.2fkmh %#5.2fWh", msToKmh(input.vehicleSpeed), input.totalEnergyUsage);
		break;
		case Cycle::Time:
		rpl_snprintf(str, 17, "Time:      %02u:%02u", static_cast<unsigned int>(input.startTime) / 60, static_cast<unsigned int>(input.startTime) % 60);
		break;
		case Cycle::_size:
		break;
	}
}

void Run::DL_SpeedEnergy::cycle()
{
	curcycle = enumCycle(curcycle);
}

void Run::DL_Ramping::get_text(char str[], Input const &input)
{
	switch(curcycle) {
		case Cycle::MotorCurrent:
		rpl_snprintf(str, 17, "MC1 %4.2f C2 %4.2f", input.motor0Current, input.motor1Current);
		break;
		case Cycle::DutyCycle:
		rpl_snprintf(str, 17, "MD1 %4.2f D2 %4.2f", input.motor0DutyCycle, input.motor1DutyCycle);
		break;
		case Cycle::Rampspeed:
		rpl_snprintf(str, 17, "MR1 %4.4u R2 %4.4u", static_cast<unsigned int>(input.motor0Speed), static_cast<unsigned int>(input.motor1Speed));
		break;
		case Cycle::_size:
		break;
	}
}

void Run::DL_Ramping::cycle()
{
	curcycle = enumCycle(curcycle);
}

void Run::DL_Coasting::get_text(char str[], Input const &input)
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
		case Cycle::_size:
		break;
	}
}

void Run::DL_Coasting::cycle()
{
	curcycle = enumCycle(curcycle);
}

 Run::DL_Finished::DL_Finished(float const time, float const energy, float const distance) : TimerUpdate(), time(time), energy(energy), distance(distance) {}

void Run::DL_Finished::get_text(char str[], Input const &input)
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

void Run::DL_Finished::cycle()
{
	curCycle = enumCycle(curCycle);
}
