/*
 * task.h
 *
 * Created: 1/03/2018 12:55:06 PM
 *  Author: teddy
 */ 

#pragma once

#include <algorithm>
#include <type_traits>
#include <tuple>

#include "../../utility/memory.h"

namespace runtime {

namespace actionid {
	enum ActionID : uint8_t {
		BinarySpeed_Output,
		SpeedMatchChecker_Ready,
	};
}

namespace nsfunction {
	using Priority = uint8_t;
	//An action is part of the output from a function
	struct Action {
		Action(uint8_t const id = 0, uint8_t const priority = 0);
		virtual ~Action() = default;
		uint8_t id;
		uint8_t priority = 0;
	};
}


class Function {
public:
	using Priority = uint8_t;
	using Output = mvector<munique_ptr<nsfunction::Action>>;
	Function(Priority const priority);
	virtual ~Function() = default;
	//Sets all output priorities to the priority of this function and returns them
	Output pullOutput();
	//Calls function_update on all children, pull their outputs into this function, and runs update on this function
	void function_update(void *const input);
	//Gets the priority of this function
	Priority getPriority() const;
	//Sets the priority
	void setPriority(Priority const priority);

protected:
	Priority priority;
	mvector<munique_ptr<Function>> children;
	Output outputs;

	//Adds an action, and if an action of equal id with lower priority exists, will replace that action.
	void addAction(munique_ptr<nsfunction::Action> &action);
	template <typename action_t, typename = typename std::enable_if<std::is_base_of<nsfunction::Action, action_t>::value>::type>
	void addAction(action_t const &action);
	template <typename action_t, typename = typename std::enable_if<std::is_base_of<nsfunction::Action, action_t>::value>::type>
	munique_ptr<nsfunction::Action> copyAction(action_t const &action);
	//Perfect place for std::optional, bool will be true if an element should be inserted
	std::tuple<bool, mvector<munique_ptr<nsfunction::Action>>::iterator> getInsertPosition(nsfunction::Action const &action);
	//Finds an action based on actionID
	nsfunction::Action *findAction(actionid::ActionID const &actionID);

	//Adds child functions to this one
	void addChild(munique_ptr<Function> &child);

	//Called once per cycle
	virtual void update(void *const input) = 0;
};

}


template <typename action_t, typename>
void runtime::Function::addAction(action_t const &action)
{
	static_assert(std::is_base_of<nsfunction::Action, action_t>::value, "action_t must be subclass of nsfunction::Action");
	auto res = getInsertPosition(action);
	//If the action should be added
	if(std::get<0>(res)) {
		//If it should occupy a new spot, allocated a new spot and move it there
		if(std::get<1>(res) == outputs.end()) {
			outputs.push_back(copyAction(action));
		}
		//If it should replace something, move it into the replacement position (smart pointer should automatically deallocate the previous one)
		else
			(*std::get<1>(res)) = copyAction(action);
	}
}

template <typename action_t, typename>
munique_ptr<runtime::nsfunction::Action> runtime::Function::copyAction(action_t const &action)
{
	static_assert(std::is_base_of<nsfunction::Action, action_t>::value, "action_t must be subclass of nsfunction::Action");
	return munique_ptr<nsfunction::Action>(new (pvPortMalloc(sizeof(action_t))) action_t(action));
}
