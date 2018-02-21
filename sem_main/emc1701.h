/*
 * emc1701.h
 *
 * Created: 20/02/2018 7:27:25 PM
 *  Author: teddy
 */ 

namespace emc1701 {
	enum class Register {
		Configuration                 = 0x03,
		OneShot                       = 0x0f,
		ChannelMask                   = 0x1f,
		TemperatureCriticalLimit      = 0x20,
		TemperatureCriticalHysteresis = 0x21,
		ConsecutiveAlert              = 0x22,
		Status                        = 0x34,
		HighLimitStatus               = 0x35,
		LowLimitStatus                = 0x36,
		CriticalLimitStatus           = 0x37,
		TemperatureHighByte           = 0x38,
		TemperatureLowByte            = 0x39,
		VoltageSamplingConfig         = 0x50,
		CurrentSamplingConfig         = 0x51,
		PeakDetectionConfig           = 0x52,
		VsenseHighByte                = 0x54,
		VsenseLowByte                 = 0x55,
		VsourceHighByte               = 0x58,
		VsourceLowByte                = 0x59,
		PowerRatioHighByte            = 0x5b,
		PowerRatioLowByte             = 0x5c,
		VsenseHighLimit               = 0x60,
		VsenseLowLimit                = 0x61,
		VsourceHighLimit              = 0x64,
		VsourceLowLimit               = 0x65,
		VsenseCritialLimit            = 0x66,
		VsourceCriticalLimit          = 0x68,
		VsenseCriticalHysteresis      = 0x69,
		VsourceCriticalHysteresis     = 0x6a,
		ProductFeatures               = 0xfc,
		ProductID                     = 0xfd,
		SMSCID                        = 0xfe,
		Revision                      = 0xff,
	};

	//---Status Register---
	enum class pos_Status {
		Busy     = 7,
		Peak     = 6,
		High     = 4,
		Low      = 3,
		Critical = 1,
	};

	//---Config Register---
	enum class pos_Config {
		MaskAll = 7,
		TemperatureDisabled = 6,
		AlertComparatorModeEnabled = 5,
		VoltageMeasurementDisabled = 2,
	};

	//---ConversionRate Register---
	enum class pos_ConversionRate {
		v_ConversionRate = 0,
	};

	enum class val_ConversionRate {
		per16000ms = 0,
		per8000ms,
		per4000ms,
		per2000ms,
		per1000ms,
		per500ms,
		per250ms,
		per125ms,
	};

	//---ChannelMask Register---
	enum class pos_ChannelMask {
		Vsense      = 7,
		Vsrc        = 6,
		Peak        = 5,
		Temperature = 0,
	};

	//---ConsecutiveAlert Register---
	enum class pos_ConsecutiveAlert {
		Timeout   = 7,
		v_Therm = 4,
		v_Alert = 1,
	};

	enum class val_ConsecutiveAlert {
		_1 = 0b000,
		_2 = 0b001,
		_3 = 0b011,
		_4 = 0b111,
	};

	//---HighLimitStatus Register---
	enum class pos_HighLimitStatus {
		Vsense      = 7,
		Vsrc        = 6,
		Temperature = 0,
	};

	//---LowLimitStatus Register---
	enum class pos_LowLimitStatus {
		Vsense      = 7,
		Vsrc        = 6,
		Temperature = 0,
	};

	//---CriticalLimitStatus Register---
	enum class pos_CriticalLimitStatus {
		Vsense      = 7,
		Vsrc        = 6,
		Temperature = 0,
	};

	//---VoltageSamplingConfig Register---
	enum class pos_VoltageSamplingConfig {
		Peak_Alert_Therm     = 7,
		v_VoltageQueue     = 2,
		v_VoltageAveraging = 0,
	};

	enum class val_VoltageQueue {
		_1 = 0b00,
		_2 = 0b01,
		_3 = 0b10,
		_4 = 0b11,
	};

	enum class val_VoltageAveraging {
		_1 = 0b00,
		_2 = 0b01,
		_4 = 0b10,
		_8 = 0b11,
	};

	//---CurrentSamplingConfig Register---
	enum class pos_CurrentSamplingConfig {
		v_CurrentQueue        = 6,
		v_CurrentAveraging    = 4,
		v_CurrentSampleTime   = 2,
		v_CurrentVoltageRange = 0,
	};

	enum class val_CurrentQueue {
		_1 = 0b00,
		_2 = 0b01,
		_3 = 0b10,
		_4 = 0b11,
	};

	enum class val_CurrentAveraging {
		_1 = 0b00,
		_2 = 0b01,
		_4 = 0b10,
		_8 = 0b11,
	};

	enum class val_CurrentSampleTime {
		_82ms  = 0b00,
		_164ms = 0b10,
		_328ms = 0b11,
	};

	enum class val_CurrentVoltageRange {
		_10mV = 0b00,
		_20mV = 0b01,
		_40mV = 0b10,
		_80mV = 0b11,
	};

	//---PeakDetectionConfig Register---
	enum class pos_PeakDetectionConfig {
		v_PeakDetectionVoltageThreshold = 4,
		v_PeakDetectionDuration         = 0,
	};

	enum class val_PeakDetectionVoltageThreshold {
		_10mV = 0,
		_15mV,
		_20mV,
		_25mV,
		_30mV,
		_35mV,
		_40mV,
		_45mV,
		_50mV,
		_55mV,
		_60mV,
		_65mV,
		_70mV,
		_75mV,
		_80mV,
		_85mV,
	};

	enum class val_PeakDetectionDuration {
		   _1000us = 0,
		   _5120us,
		  _25600us,
		  _51200us,
		  _76800us,
		 _102400us,
		 _256000us,
		 _384000us,
		 _512000us,
		 _768000us,
		_1024000us,
		_1536000us,
		_2048000us,
		_3072000us,
		_4096000us,
	};

	struct Config {
		//Can't seem to have defaults with bit fields
		bool config_maskAll						    : 1;
		bool config_temperatureDisabled			    : 1;
		bool config_alertComparatorModeEnabled	    : 1;
		bool config_voltageMeasurementDisabled	    : 1;
		bool channelMask_Vsense					    : 1;
		bool channelMask_Vsrc					    : 1;
		bool channelMask_peak					    : 1;
		bool channelMask_temperature			    : 1;
		bool consecutiveAlert_timeout			    : 1;
		bool voltageSamplingConfig_peak_alert_therm : 1;
		val_ConversionRate conversionRate = val_ConversionRate::per125ms;
		val_ConsecutiveAlert consecutiveAlert_therm = val_ConsecutiveAlert::_4;
		val_ConsecutiveAlert consecutiveAlert_alert = val_ConsecutiveAlert::_1;
		val_VoltageQueue voltageQueue = val_VoltageQueue::_1;
		val_VoltageAveraging voltageAveraging = val_VoltageAveraging::_1;
		val_CurrentQueue currentQueue = val_CurrentQueue::_1;
		val_CurrentAveraging currentAveraging = val_CurrentAveraging::_1;
		val_CurrentSampleTime currentSampleTime = val_CurrentSampleTime::_82ms;
		val_CurrentVoltageRange currentVoltageRange = val_CurrentVoltageRange::_80mV;
		val_PeakDetectionVoltageThreshold peakDetectionVoltageThreshold = val_PeakDetectionVoltageThreshold::_85mV;
		val_PeakDetectionDuration peakDetectionDuration = val_PeakDetectionDuration::_1000us;


		//---Temperature---
		int8_t highLimit_temperature;
		int8_t lowLimit_temperature;
		int8_t criticalLimit_temperature;
		int8_t criticalHysteresis_temperature;

		//---Vsense---
		int8_t highLimit_Vsense;
		int8_t lowLimit_Vsense;
		int8_t criticalLimit_Vsense;
		int8_t criticalHysteresis_Vsense;
		
		//---Vsrc---
		uint8_t highLimit_Vsrc;
		uint8_t lowLimit_Vsrc;
		uint8_t criticalLimit_Vsrc;
		uint8_t criticalHysteresis_Vsrc;
	};
}

class EMC1701 {
public:

};
