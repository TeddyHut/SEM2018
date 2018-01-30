/*
 * adpcontrol.h
 *
 * Created: 30/01/2018 5:15:04 PM
 *  Author: teddy
 */ 

#pragma once
#include "run.h"
#include <adp.h>
#include <usart.h>
#include <usart_interrupt.h>

using namespace Run;

class ADPControl : public Task {
public:
	void init();
	Output update(Input const &input) override;
	Task *complete(Input const &input) override;
	ADPControl();
private:
	
};
