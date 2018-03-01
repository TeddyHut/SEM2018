/*
 * feedbackitem.h
 *
 * Created: 28/02/2018 12:36:14 PM
 *  Author: teddy
 */ 

#pragma once

#include <memory>
#include <type_traits>

#include <FreeRTOS.h>

#include "../../utility/memory.h"

namespace feedbackitem {
	namespace action {
		enum class Type : uint8_t {
			None = 0,
			Wait,
			ValueItem_SetValue,
		};

		struct Action {
			Action(Type const type = Type::None);
			virtual ~Action() = default;
			
			//Called when the action starts
			virtual void start();
			//Returns true when the action is finished
			virtual bool update() = 0;
			Type type;
		};

		struct Wait : public Action {
			Wait(TickType_t const duration = 0);
			
			void start() override;
			bool update() override;

			TickType_t duration;
		};
	}
}

class FeedbackItem {
public:
	//Wait -ticks- amount of time
	void wait(TickType_t const ticks);
protected:

	template <typename action_t>
	void addAction(action_t const &act);

	//Called by feedback master, calls update on the most recent action and moves onto the next if required
	void update();
private:
	//vector of actions
	mvector<munique_ptr<feedbackitem::action::Action>> actions;
	feedbackitem::action::Action *currentAction = nullptr;
};

template <typename action_t>
void FeedbackItem::addAction(action_t const &act)
{
	static_assert(std::is_base_of<feedbackitem::action::Action, action_t>::value, "action_t must be subclass of feedbackitem::action::Action");
	actions.push_back(std::move(munique_ptr<feedbackitem::action::Action>(new (pvPortMalloc(sizeof(action_t))) action_t(act))));
}
