/*
 * util.h
 *
 * Created: 19/01/2018 6:53:44 PM
 *  Author: teddy
 */ 

#pragma once

#include <functional>
#include <memory>
#include <type_traits>
#include <vector>
#include <list>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "main_config.h"

//Sets motor duty cycle to zero and turns off servo power
void stopmovement();

//Called whenever there's an error. Need to make better error management.
void debugbreak();

//printf implementations taken from GitHub, since built-in ones that supported float always messed with stack.
extern "C" {
//int rpl_vsnprintf(char *, size_t, const char *, va_list);
//int rpl_vasprintf(char **, const char *, va_list);
//int rpl_asprintf(char **, const char *, ...);
int rpl_snprintf(char *, size_t, const char *, ...) __attribute__((format(printf, 3, 4)));
}

//Deleter to be used with smart pointers, using FreeRTOS memory management
template <typename T>
class deleter_free {
public:
	void operator() (T *const p) const {
		p->~T();
		vPortFree(p);
	}
};

//Mallocator. Used for containers so that FreeRTOS memory management and new don't mess with each other.
//Taken from http://en.cppreference.com/w/cpp/concept/Allocator
#include <cstdlib>
#include <new>

template <class T>
struct Mallocator {
	typedef T value_type;
	
	Mallocator() = default;
	template <class U> constexpr Mallocator(const Mallocator<U>&) noexcept {}
	
	T *allocate(std::size_t n) noexcept {
		if (n > std::size_t(-1) / sizeof(T))
			debugbreak();
		if (auto p = static_cast<T *>(pvPortMalloc(n * sizeof(T))))
			return p;
		debugbreak();
		return nullptr;
	}

	void deallocate(T *p, std::size_t) noexcept {
		vPortFree(p);
	}
};
template <class T, class U>
bool operator==(const Mallocator<T> &, const Mallocator<U> &) { return true; }
template <class T, class U>
bool operator!=(const Mallocator<T> &, const Mallocator<U> &) { return false; }

//Standard container typedefs that use Mallocator
template <typename T>
using mvector = std::vector<T, Mallocator<T>>;
template <typename T>
using mlist = std::list<T, Mallocator<T>>;


//This was a thing and then it wasn't
constexpr void throwIfTrue(bool const p, char const *const str) {
	//p ? throw std::logic_error(str) : 0;
}

//Useful ADC conversion functions
namespace adcutil {
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
}
