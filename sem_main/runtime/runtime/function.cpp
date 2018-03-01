/*
 * task.cpp
 *
 * Created: 1/03/2018 12:55:26 PM
 *  Author: teddy
 */ 

#include <utility>
#include <algorithm>

#include "function.h"

runtime::Function::Function(Priority const priority) : priority(priority) {}
	
runtime::Function::Output runtime::Function::pullOutput()
{
	std::for_each(outputs.begin(), outputs.end(), [&] (auto &p){ p->priority = priority; });
	return std::move(outputs);
}

void runtime::Function::function_update(void *const input)
{
	outputs.clear();
	for(auto &child : children) {
		child->function_update(input);
		auto &&childOut = child->pullOutput();
		std::for_each(childOut.begin(), childOut.end(), [&](auto &p) { this->addAction(p); });
	}
	update();
}

runtime::Function::Priority runtime::Function::getPriority() const
{
	return priority;
}

void runtime::Function::addAction(munique_ptr<nsfunction::Action> &action)
{
	auto res = getInsertPosition(*action);
	//If the action should be added
	if(std::get<0>(res)) {
		//If it should occupy a new spot, allocated a new spot and move it there
		if(std::get<1>(res) == outputs.end())
			outputs.push_back(std::move(action));
		//If it should replace something, move it into the replacement position (smart pointer should automatically deallocate the previous one)
		else
			(*std::get<1>(res)) = std::move(action);
	}
}

std::tuple<bool, mvector<munique_ptr<runtime::nsfunction::Action>>::iterator> runtime::Function::getInsertPosition(nsfunction::Action const &action)
{
	auto itr = std::find_if(outputs.begin(), outputs.end(), [&action](auto const &p) {
		return p->id == action.id;
	});
	//If this action should replace the action currently there of the same id
	if((itr != outputs.end()) && ((*itr)->priority <= action.priority))
		return std::make_tuple(true, itr);
	//If the action is new, the bool will be true, otherwise don't allocate a position
	return std::make_tuple(itr == outputs.end(), outputs.end());
}

void runtime::Function::addChild(munique_ptr<Function> &child)
{
	children.push_back(std::move(child));
}
