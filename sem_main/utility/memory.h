/*
 * memory.h
 *
 * Created: 28/02/2018 11:53:28 AM
 *  Author: teddy
 */ 

#pragma once

#include <cstdlib>
#include <memory>
#include <vector>
#include <list>

#include <FreeRTOS.h>

//I suppose I could have just created an operator new overload instead of these... oh well.

namespace utility {
	namespace memory {
		//Mallocator. Used for containers so that FreeRTOS memory management and new don't mess with each other.
		//Taken from http://en.cppreference.com/w/cpp/concept/Allocator
		template <class T>
		struct Allocator_FreeRTOS {
			typedef T value_type;
			
			Allocator_FreeRTOS() = default;
			template <class U>
			constexpr Allocator_FreeRTOS(const Allocator_FreeRTOS<U>&) noexcept {}
			
			T *allocate(std::size_t n) noexcept {
				if (n > std::size_t(-1) / sizeof(T))
					//Error
				if (auto p = static_cast<T *>(pvPortMalloc(n * sizeof(T))))
					return p;
				//Error
				return nullptr;
			}

			void deallocate(T *p, std::size_t) noexcept {
				vPortFree(p);
			}
		};

		//Deleter to be used with smart pointers, using FreeRTOS memory management
		template <typename T>
		struct Deleter_FreeRTOS {
			void operator() (T *const p) const {
				p->~T();
				vPortFree(p);
			}
		};
	}
}

template <class T, class U>
bool operator==(utility::memory::Allocator_FreeRTOS<T> const &, utility::memory::Allocator_FreeRTOS<T> const &) { return true; }
template <class T, class U>
bool operator!=(utility::memory::Allocator_FreeRTOS<T> const &, utility::memory::Allocator_FreeRTOS<T> const &) { return false; }

//Standard container typedefs that use Allocator_FreeRTOS
template <typename T>
using mvector = std::vector<T, utility::memory::Allocator_FreeRTOS<T>>;
template <typename T>
using mlist = std::list<T, utility::memory::Allocator_FreeRTOS<T>>;

//Standard pointers typedefs that use Deleter_FreeRTOS
template <typename T>
using munique_ptr = std::unique_ptr<T, utility::memory::Deleter_FreeRTOS<T>>;
