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

 Run::DL_Idle_Top::DL_Idle_Top(int const &countdown) : DisplayLine(ID::Idle), countdown(countdown)
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

 Run::DL_Battery::DL_Battery() : DisplayLine(ID::Battery) {}

void Run::DL_Battery::cycle()
{
	curcycle = enumCycle(curcycle);
}

void Run::DL_SpeedEnergy::get_text(char str[], Input const &input)
{
	switch(curcycle) {
		case Cycle::SpeedEnergy:
		//rpl_snprintf(str, 17, "%#5.2fkmh %#5.2fWh", msToKmh(input.vehicleSpeed), input.totalEnergyUsage);
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

void Run::Display::printDisplay(Input const &input) const
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

void Run::DL_USB::get_text(char str[], Input const &input)
{
	//IF there is a USB error, show that
	if(runtime::usbmsc->isError()) {
		rpl_snprintf(str, 17, "%-16s", runtime::usbmsc->lastError());
	}
	else if(runtime::usbmsc->isReady()) {
		rpl_snprintf(str, 17, "File: %10s", runtime::usbmsc->name());
	}
	else {
		rpl_snprintf(str, 17, "Connect USB");
	}
}

Run::DL_USB::DL_USB() : DisplayLine(ID::USSB) {}

void Run::DL_Settings::get_text(char str[], Input const &input)
{
	switch(runtime::usbmsc->settings.testtype) {
	default:
		rpl_snprintf(str, 17, "Unknown test");
		break;
	case USBMSC::Settings::TestType::Torque:
		rpl_snprintf(str, 17, "%02u%% %#7.4f N/m", static_cast<unsigned int>(runtime::usbmsc->settings.dutyCycle[0] * 100),
			runtime::usbmsc->settings.springConstant);
		break;
	case USBMSC::Settings::TestType::DutyCycle:
		rpl_snprintf(str, 17, "%3u%% %3u%% %u",
			static_cast<unsigned int>(runtime::usbmsc->settings.dutyCycle[0] * 100),
			static_cast<unsigned int>(runtime::usbmsc->settings.dutyCycle[1] * 100),
			runtime::usbmsc->settings.dutyCycleDivisions);
		break;
	case USBMSC::Settings::TestType::Frequency:
		rpl_snprintf(str, 17, "%04f %04f %u",
			runtime::usbmsc->settings.frequency[0],
			runtime::usbmsc->settings.frequency[1],
			runtime::usbmsc->settings.frequencyDivisions);
		break;
	}
}

Run::DL_Settings::DL_Settings() : DisplayLine(ID::Settings) {}

Run::DisplayLine::DisplayLine(ID const id /*= ID::None*/) : id(id) {}

void Run::DL_Samples::get_text(char str[], Input const &input)
{
	rpl_snprintf(str, 17, "Samples: %7u", input.samples);
}

Run::DL_Samples::DL_Samples() : DisplayLine(ID::Samples) {}
 
 void Run::DL_Torque::get_text(char str[], Input const &input)
 {
	//Binary character should be tau on the HD44780
	rpl_snprintf(str, 17, "%c: %#10.8f Nm", 0b10010111, torque);
 }
  
 Run::DL_Torque::DL_Torque(float const &torque) : DisplayLine(ID::Torque), torque(torque) {}

void Run::DL_DutyCycle::get_text(char str[], Input const &input)
{
	rpl_snprintf(str, 17, "DutyCycle: %#4.1f%%", input.motorDutyCycle * 100.0f);
}

 Run::DL_DutyCycle::DL_DutyCycle() : DisplayLine(ID::DutyCycle) {}

void Run::DL_Frequency::get_text(char str[], Input const &input)
{
	rpl_snprintf(str, 17, "Freq: %8fHz", input.motorPWMFrequency);
}

Run::DL_Frequency::DL_Frequency() : DisplayLine(ID::Frequency) {}

void Run::DL_Delay::get_text(char str[], Input const &input)
{
	rpl_snprintf(str, 17, "Wait: %4fs", finishTime - input.time);
}

 Run::DL_Delay::DL_Delay(float const finishTime) : DisplayLine(ID::Delay), finishTime(finishTime) {}
