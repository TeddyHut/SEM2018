/*
 * conversions.h
 *
 * Created: 28/02/2018 12:18:19 PM
 *  Author: teddy
 */

#pragma once

#include <cmath>
#include <numeric>

#include <FreeRTOS.h>

namespace utility {
	namespace conversions {
		//Gets the linear conversion factor for a voltage divider
		template <typename T>
		constexpr T vdiv_factor(T const r1, T const r2) {
			return r2 / (r1 + r2);
		}

		//Gets the input voltage of a voltage divider based on the output
		template <typename T>
		constexpr T vdiv_input(T const output, T const factor) {
			return output / factor;
		}

		//Gets the voltage at an ADC input based on the value
		template <typename adc_t, adc_t max, typename T>
		constexpr T adc_voltage(adc_t const value, T const vref) {
			return vref * (static_cast<T>(value) / static_cast<T>(max));
		}

		//Gets the value of an ADC input based on the voltage
		template <typename adc_t, adc_t max, typename T>
		constexpr adc_t adc_value(T const voltage, T const vref) {
			return (voltage / vref) * static_cast<T>(max);
		}

		//Gets the output voltage of an ACS711 based on the current flowing through it
		template <typename T>
		constexpr T acs711_outV(T const current, T const vcc) {
			return (current * ((vcc - 0.6f) / (12.5f * 2.0f))) + (vcc / 2);
		}

		//Gets the current flowing through an ACS711 based on the output voltage
		template <typename T>
		constexpr T acs711_current(T const outV, T const vcc) {
			return ((12.5f * 2.0f) / (vcc - 0.6f)) * (outV - (vcc / 2.0f));
		}

		//Gets the output voltage based on the input voltage of the opamp configuration for the ACS711 on the BMS
		template <typename T>
		constexpr T bms_current_opamp_transformation_outV(T const inV, T const vcc) {
			return (inV * (5.0f / 6.0f)) - (vcc / 3.0f);
		}

		//Gets the input voltage based on the output voltage of the opamp configuration for the ACS711 on the BMS
		template <typename T>
		constexpr T bms_current_opamp_transformation_inV(T const outV, T const vcc) {
			return (6.0f / 5.0f) * (outV + (vcc / 3.0f));
		}

		//Gets the output voltage based on the input voltage of the opamp configuration for the ACS711 on the motor
		template <typename T>
		constexpr T motor_current_opamp_transformation_outV(T const inV, T const vcc) {
			return (inV * ((5.1 * (2 * (1 + 5.1) + 10)) / ((1 + 5.1) * (2 * 1 + 10)))) - (vcc * (5.1 / (2 * 1 + 10)));
		}

		//Gets the input voltage based on the output voltage of the opamp configuration for the ACS711 on the motor
		template <typename T>
		constexpr T motor_current_opamp_transformation_inV(T const outV, T const vcc) {
			return (outV + (vcc * (5.1 / (2 * 1 + 10)))) / ((5.1 * (2 * (1 + 5.1) + 10)) / ((1 + 5.1) * (2 * 1 + 10)));
		}

		//Gets the temperature of the BMS temperature chip based on the output (in milli-volts) from the device
		template <typename T>
		T temperature_degrees(T const outmV) {
			T result = 2230.8 - outmV;
			result *= (4 * 0.00433);
			result += std::pow(-13.528, 2);
			result = 13.582 - std::sqrt(result);
			result /= (2 * -0.00433);
			return result + 30;
		}

		//Converts milliseconds to FreeRTOS ticks
		template <typename T>
		constexpr TickType_t msToTicks(T const ms) {
			return (ms / 1000.0f) * configTICK_RATE_HZ;
		}

		//Converts kilometers per hour to meters per second
		template <typename T>
		constexpr T kmhToMs(T const kmh) {
			return kmh / 3.6;
		}

		//Converts meters per second to kilometers per hour
		template <typename T>
		constexpr T msToKmh(T const ms) {
			return ms * 3.6;
		}
	}
}