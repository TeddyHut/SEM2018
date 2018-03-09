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

namespace Run {
	class DisplayLine {
		public:
		virtual void get_text(char str[], Input const &input) = 0;
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
		DL_Idle_Top(int const&countdown);
		void get_text(char str[], Input const &input) override;
	private:
		int const &countdown;
	};

	//Displays battery statistics
	class DL_Battery : public DisplayLine, public TimerUpdate {
	public:
		void get_text(char str[], Input const &input) override;
	protected:
		void cycle() override;
		enum class Cycle {
			Voltage = 0,
			Current,
			AvgCellVoltage,
			_size,
		} curcycle = Cycle::Voltage;
	};

	//Displays current vehicle speed and total energy usage. Switches to total time. During normal circumstances always on the top.
	class DL_SpeedEnergy : public DisplayLine, public TimerUpdate {
	public:
		void get_text(char str[], Input const &input) override;
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
	protected:
		void cycle() override;
		enum class Cycle {
			MotorCurrent = 0,
			DutyCycle,
			Rampspeed,
			_size,
		} curcycle = Cycle::MotorCurrent;
	};

	//Alternates between coasting statistics (distance traveled, time coasting, battery voltages)
	class DL_Coasting : public DisplayLine, public TimerUpdate {
	public:
		void get_text(char str[], Input const &input) override;
	protected:
		void cycle() override;
		enum class Cycle {
			Distance = 0,
			CoastTime,
			BatteryVoltage,
			_size,
		} curcycle = Cycle::Distance;
		//Used to workout coasting time
		bool firstGetText = true;
		float starttime;
	};

	class DL_Finished : public DisplayLine, public TimerUpdate {
	public:
		DL_Finished(float const time, float const energy, float const distance);
		void get_text(char str[], Input const &input) override;
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
