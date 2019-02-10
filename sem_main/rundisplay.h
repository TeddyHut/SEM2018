/*
 * rundisplay.h
 *
 * Created: 2/02/2018 12:10:05 AM
 *  Author: teddy
 */ 

#pragma once

#include <FreeRTOS.h>
#include <semphr.h>
#include <timers.h>
#include "runcommon.h"

namespace program {
	class DisplayLine {
	public:
		enum class ID : uint8_t {
			None,
			Idle,
			Battery,
			Settings,
			USSB,
			SpeedEnergy,
			Ramping,
			Coasting,
			Finished,
		} id;
		virtual void get_text(char str[], Input const &input) = 0;
		DisplayLine(ID const id = ID::None);
		virtual ~DisplayLine() = default;
	};

	//Used in display line to switch between statistics
	class TimerUpdate {
	public:
		TimerUpdate(TickType_t const period = config::run::displaycycleperiod);
		virtual ~TimerUpdate();
	protected:
		TimerHandle_t timer;
		virtual void cycle() = 0;
	private:
		static void callback(TimerHandle_t timer);
	};

	//Displays the startup prompt for top line
	class DL_Idle_Top : public DisplayLine {
	public:
		void get_text(char str[], Input const &input) override;
		DL_Idle_Top(int const &countdown);
	private:
		int const &countdown;
	};

	class DL_USB : public DisplayLine {
	public:
		void get_text(char str[], Input const &input) override;
		DL_USB();
	};

	class DL_Settings : public DisplayLine, public TimerUpdate {
	public:
		void get_text(char str[], Input const &input) override;
		DL_Settings();
	private:
		void cycle() override;
		enum class Cycle {
			File,
			Speeds,
			Times,
			PWMFrequency,
			SampleFrequency,
			Wheel,
			_size,
		} curcycle = Cycle::File;
	};

	//Displays battery statistics
	class DL_Battery : public DisplayLine, public TimerUpdate {
	public:
		void get_text(char str[], Input const &input) override;
		DL_Battery();
	protected:
		void cycle() override;
		enum class Cycle {
			Voltage = 0,
			Current,
			AvgCellVoltage,
			Temperature,
			_size,
		} curcycle = Cycle::Voltage;
	};

	//Displays current vehicle speed and total energy usage. Switches to total time. During normal circumstances always on the top.
	class DL_SpeedEnergy : public DisplayLine, public TimerUpdate {
	public:
		void get_text(char str[], Input const &input) override;
		DL_SpeedEnergy();
	protected:
		void cycle() override;
		enum class Cycle {
			SpeedEnergy = 0,
			Time,
			_size,
		} curcycle = Cycle::SpeedEnergy;
	};

	//Alternates between ramping statistics (motor current usages, duty cycle, ramping speed)
	class DL_Ramping : public DisplayLine, public TimerUpdate {
	public:
		void get_text(char str[], Input const &input) override;
		DL_Ramping();
	protected:
		void cycle() override;
		enum class Cycle {
			DutyCycle = 0,
			MotorCurrent,
			Rampspeed,
			_size,
		} curcycle = Cycle::DutyCycle;
	};

	//Alternates between coasting statistics (distance traveled, time coasting, battery voltages, USB samples)
	class DL_Coasting : public DisplayLine, public TimerUpdate {
	public:
		void get_text(char str[], Input const &input) override;
		DL_Coasting();
	protected:
		void cycle() override;
		enum class Cycle {
			Distance = 0,
			CoastTime,
			BatteryVoltage,
			Samples,
			_size,
		} curcycle = Cycle::Distance;
		//Used to workout coasting time
		bool firstGetText = true;
		float starttime;
	};

	class DL_Finished : public DisplayLine, public TimerUpdate {
	public:
		void get_text(char str[], Input const &input) override;
		DL_Finished(float const time, float const energy, float const distance);
	protected:
		void cycle() override;
		enum class Cycle {
			Finished = 0,
			Energy,
			Time,
			Distance,
			_size,
		} curCycle = Cycle::Finished;
		float time = 0;
		float energy = 0;
		float distance = 0;
	};

	struct Display {
		//The current displayline used for the top line
		std::unique_ptr<DisplayLine, deleter_free<DisplayLine>> topline;
		//Display line used for the bottom line
		std::unique_ptr<DisplayLine, deleter_free<DisplayLine>> bottomline;
		
		void printDisplay(Input const &input) const;
	};
}
