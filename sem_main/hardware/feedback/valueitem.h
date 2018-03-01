/*
 * valueitem.h
 *
 * Created: 28/02/2018 2:48:11 PM
 *  Author: teddy
 */ 

#pragma once

#include "feedbackitem.h"

//Used as a base-class for subclasses to easily be able to set a value of any type using FeedbackItem
template <typename T>
class ValueItem : public FeedbackItem {
public:
	void setValue(T const value);
protected:
	//Response to the action
	virtual void valueItem_setValue(T const value) = 0;
};

namespace valueitem {
	namespace action {
		template <typename T>
		struct SetValue : public feedbackitem::action::Action {
			SetValue(ValueItem<T> *const item, T const value = false);
			void startup() override;
			bool update() override;

			ValueItem<T> *const item;
			T value;
		};
	}
}

template <typename T>
void ValueItem<T>::setValue(T const value)
{
	addAction(valueitem::action::SetValue<T>(this, value));
}

template <typename T>
valueitem::action::SetValue<T>::SetValue(ValueItem<T> *const item, T const value /*= false*/) : Action(feedbackitem::action::Type::ValueItem_SetValue), item(item), value(value) {}


template <typename T>
void valueitem::action::SetValue<T>::startup()
{
	item->valueItem_setValue(value);
}

template <typename T>
bool valueitem::action::SetValue<T>::update()
{
	return true;
}
