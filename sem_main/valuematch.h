/*
 * valuematch.h
 *
 * Created: 25/01/2018 2:19:42 PM
 *  Author: teddy
 */ 

#pragma once

#include <algorithm>

namespace ValueMatch {
	template <typename T, typename time_t>
	class ValueMatch {
	public:
		T operator()(time_t const time) const;
		virtual T currentValue(time_t const time) const = 0;
		virtual void startup(T const initial, T const destination, time_t const time_initial, time_t const time_destination) = 0;
		T get_time_destination() const;
	protected:
		time_t time_destination;
	};
	
	template <typename T, typename time_t>
	class Linear : public ValueMatch<T, time_t> {
	public:
		T currentValue(time_t const time) const override;
		void startup(T const initial, T const destination, time_t const time_initial, time_t const time_destination) override;
	private:
		using ValueMatch<T, time_t>::time_destination;
		T m;
		T c;
	};
}

template <typename T, typename time_t>
T ValueMatch::ValueMatch<T, time_t>::operator()(time_t const time) const
{
	return currentValue(time);
}

template <typename T, typename time_t>
T ValueMatch::ValueMatch<T, time_t>::get_time_destination() const
{
	return time_destination;
}

template <typename T, typename time_t>
void ValueMatch::Linear<T, time_t>::startup(T const initial, T const destination, time_t const time_initial, time_t const time_destination)
{
	this->time_destination = time_destination;
	m = (destination - initial) / (time_destination - time_initial);
	c = initial - (m * time_initial);
}

template <typename T, typename time_t>
T ValueMatch::Linear<T, time_t>::currentValue(time_t const time) const 
{
	//Don't return more than the value at destination time
	return m * std::min(time, time_destination) + c;
}
