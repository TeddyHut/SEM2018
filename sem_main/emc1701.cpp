/*
 * emc1701.cpp
 *
 * Created: 23/02/2018 8:55:53 PM
 *  Author: teddy
 */ 

#include "emc1701.h"
#include "util.h"

#include <map>
#include <list>
#include <algorithm>
#include <functional>
#include <cstring>

namespace emc1701 {
	int regpos(Register const reg) {
		for(uint8_t i = 0; i < sizeof registers; i++) {
			if (registers[i] == reg)
				return i;
		}
		return -1;
	}
}

using namespace emc1701;

void EMC1701::init(TWIManager *const twiManager, uint8_t const address)
{
	this->twiManager = twiManager;
	this->address = address;
	if((sem_twicallback = xSemaphoreCreateBinary()) == NULL)
		debugbreak();
	if((mtx_regcache = xSemaphoreCreateMutex()) == NULL)
		debugbreak();
	if(xTaskCreate(taskFunction, config::emc1701::taskName, config::emc1701::taskStackDepth, this, config::emc1701::taskPriority, &task) == pdFAIL)
		debugbreak();
}

void EMC1701::setConfig(emc1701::Config const &config)
{
	if(task == NULL)
		debugbreak();
	
	uint8_t ureg = 0;

	//Configuration
	ureg = (cu(config.config_maskAll                   ) << cs(pos_Config::MaskAll)) |
	       (cu(config.config_temperatureDisabled       ) << cs(pos_Config::TemperatureDisabled)) |
		   (cu(config.config_alertComparatorModeEnabled) << cs(pos_Config::AlertComparatorModeEnabled)) |
		   (cu(config.config_voltageMeasurementDisabled) << cs(pos_Config::VoltageMeasurementDisabled));
	writeRegister(Register::Configuration, ureg);

	//ConversionRate
	ureg = cs(config.conversionRate) << cs(pos_ConversionRate::v_ConversionRate);
	writeRegister(Register::ConversionRate, ureg);

	//ChannelMask
	ureg = (cu(config.channelMask_Vsense     ) << cs(pos_ChannelMask::Vsense)) |
	       (cu(config.channelMask_Vsrc       ) << cs(pos_ChannelMask::Vsrc)) |
		   (cu(config.channelMask_peak       ) << cs(pos_ChannelMask::Peak)) |
		   (cu(config.channelMask_temperature) << cs(pos_ChannelMask::Temperature));
	writeRegister(Register::ChannelMask, ureg);

	//ConsecutiveAlert
	ureg = (cu(config.consecutiveAlert_timeout) << cs(pos_ConsecutiveAlert::Timeout)) |
	       (cs(config.consecutiveAlert_therm  ) << cs(pos_ConsecutiveAlert::v_Therm)) |
		   (cs(config.consecutiveAlert_alert  ) << cs(pos_ConsecutiveAlert::v_Alert));
	writeRegister(Register::ConsecutiveAlert, ureg);

	//VoltageSamplingConfig
	ureg = (cu(config.voltageSamplingConfig_peak_alert_therm) << cs(pos_VoltageSamplingConfig::Peak_Alert_Therm)) |
	       (cs(config.voltageQueue                          ) << cs(pos_VoltageSamplingConfig::v_VoltageQueue)) |
		   (cs(config.voltageAveraging                      ) << cs(pos_VoltageSamplingConfig::v_VoltageAveraging));
	writeRegister(Register::VoltageSamplingConfig, ureg);

	//CurrentSamplingConfig
	ureg = (cs(config.currentQueue       ) << cs(pos_CurrentSamplingConfig::v_CurrentQueue)) |
	       (cs(config.currentAveraging   ) << cs(pos_CurrentSamplingConfig::v_CurrentAveraging)) |
		   (cs(config.currentSampleTime  ) << cs(pos_CurrentSamplingConfig::v_CurrentSampleTime)) |
		   (cs(config.currentVoltageRange) << cs(pos_CurrentSamplingConfig::v_CurrentVoltageRange));
	writeRegister(Register::CurrentSamplingConfig, ureg);

	//PeakDetectionConfig
	ureg = (cs(config.peakDetectionVoltageThreshold) << cs(pos_PeakDetectionConfig::v_PeakDetectionVoltageThreshold)) |
	       (cs(config.peakDetectionDuration        ) << cs(pos_PeakDetectionConfig::v_PeakDetectionDuration));
	writeRegister(Register::PeakDetectionConfig, ureg);

	//Temperature
	writeRegister(Register::TemperatureHighLimit         , config.         highLimit_temperature);
	writeRegister(Register::TemperatureLowLimit          , config.          lowLimit_temperature);
	writeRegister(Register::TemperatureCriticalLimit     , config.     criticalLimit_temperature);
	writeRegister(Register::TemperatureCriticalHysteresis, config.criticalHysteresis_temperature);

	//Vsense
	writeRegister(Register::VsenseHighLimit         , config.         highLimit_Vsense);
	writeRegister(Register::VsenseLowLimit          , config.          lowLimit_Vsense);
	writeRegister(Register::VsenseCriticalLimit     , config.     criticalLimit_Vsense);
	writeRegister(Register::VsenseCriticalHysteresis, config.criticalHysteresis_Vsense);

	//Vsrc
	writeRegister(Register::VsourceHighLimit         , config.         highLimit_Vsrc);
	writeRegister(Register::VsourceLowLimit          , config.          lowLimit_Vsrc);
	writeRegister(Register::VsourceCriticalLimit     , config.     criticalLimit_Vsrc);
	writeRegister(Register::VsourceCriticalHysteresis, config.criticalHysteresis_Vsrc);
}

void EMC1701::addAction(Action const &action)
{
	//Might be better to use double buffer here
	if(xSemaphoreTake(mtx_regcache, msToTicks(100)) == pdFALSE) {
		//Some sort of warning
		//warning();
	}
	//Attempt to find an action equal to this except for value (value can be the same anyway though)
	auto itr = std::find_if(actions.begin(), actions.end(), [&action](Action const &p) {
		return !(action < p || p < action);
	});
	if(itr != actions.end()) {
		//If so, replace the value and return
		(*itr).value = action.value;
		return;
	}
	//Otherwise, find where action fits in the sorted range and put it there
	actions.insert(std::find_if(actions.begin(), actions.end(), [&action](Action const &p) {
		return action < p;
	}), action);
	xSemaphoreGive(mtx_regcache);
}

void EMC1701::taskFunction(void *instance)
{
	static_cast<EMC1701 *>(instance)->task_main();
}

void EMC1701::task_main()
{
	TickType_t previousWakeTime = xTaskGetTickCount();
	while(true) {
		//Add register updates that always take place
		readRegister(Register::Status);
		readRegister(Register::HighLimitStatus);
		readRegister(Register::LowLimitStatus);
		readRegister(Register::CriticalLimitStatus);
		readRegister(Register::TemperatureHighByte);
		readRegister(Register::TemperatureLowByte);
		readRegister(Register::VsenseHighByte);
		readRegister(Register::VsenseLowByte);
		readRegister(Register::VsourceHighByte);
		readRegister(Register::VsourceLowByte);

		//Wait for previous transfer to complete (from end of last cycle, should be there)
		xSemaphoreTake(sem_twicallback, portMAX_DELAY);

		//Obtain localdata mutex
		xSemaphoreTake(mtx_regcache, portMAX_DELAY);

		//Split actions into read actions and write actions (should already be sorted)
		auto partitionPredicate = [](Action const &p) {
			return p.type == Action::ActionType::Read;
		};

		std::size_t const readActionsSize = std::distance(
			actions.begin(),
			std::partition_point(actions.begin(), actions.end(), partitionPredicate));
		mvector<Action> readActions(readActionsSize);
		mvector<Action> writeActions(actions.size() - readActionsSize);
		std::partition_copy(actions.begin(), actions.end(),
			readActions.begin(), writeActions.begin(),
			partitionPredicate);

		//Clear actions
		actions.clear();
		//Release localdata mutex
		xSemaphoreGive(mtx_regcache);

		//Determine where actions make consecutive blocks of memory, and store the blocks of memory and what registers they refer to
		struct BlockOperation {
			mvector<Register> regs;
			//Length of buffer is regs.size()
			uint8_t *buf = nullptr;
		};
		//These should both already be sorted
		mvector<BlockOperation> readBlocks;
		mvector<BlockOperation> writeBlocks;
		auto generateBlocks = [](mvector<BlockOperation> &blocks, mvector<Action> const &actions) {
			mvector<Action>::const_iterator itr_blockStart = actions.begin();
			mvector<Action>::const_iterator itr_blockEnd = actions.begin();
			while(itr_blockEnd != actions.end()) {
				mvector<Register> regs;
				itr_blockStart = itr_blockEnd++;

				//Iterate through actions until the end of the actions was reached or the reg addresses aren't continuous
				for (uint8_t previousAddress = cs(itr_blockStart->reg);
					(cs(itr_blockEnd->reg) == ++previousAddress) && (itr_blockEnd != actions.end());
					itr_blockEnd++);

				BlockOperation block;
				block.regs.resize(std::distance(itr_blockStart, itr_blockEnd));
				block.buf = static_cast<uint8_t *>(pvPortMalloc(block.regs.size()));
				for(uint8_t i = 0; i < block.regs.size(); i++) {
					block.regs[i] = (itr_blockStart + i)->reg;
					block.buf[i] = (itr_blockStart + i)->value;
				}
				blocks.push_back(block);
			}
		};

		generateBlocks(readBlocks, readActions);
		generateBlocks(writeBlocks, writeActions);

		//Clear actions
		readActions.clear();
		writeActions.clear();

		//Write values first
		for(auto &&block : writeBlocks) {
			TWIManager::Job job;
			job.mode = TWIManager::Job::Mode::Write;
			job.sem_complete = sem_twicallback;
			job.address = address;

			//Add the register address to the start of job.outbuffer to comply to SMB protocol
			job.outbuffer = pvPortMalloc(block.regs.size() + 1);
			static_cast<uint8_t *>(job.outbuffer)[0] = cs(block.regs[0]);
			std::memcpy(static_cast<uint8_t *>(job.outbuffer) + 1, block.buf, block.regs.size());
			job.writelen = block.regs.size() + 1;

			twiManager->addJob(job, portMAX_DELAY);
			//Wait until block has been written
			xSemaphoreTake(sem_twicallback, portMAX_DELAY);
			//Free memory used by block and outbuffer
			vPortFree(block.buf);
			vPortFree(job.outbuffer);
		}

		//Then read values
		for(auto &&block : readBlocks) {
			TWIManager::Job job;
			job.mode = TWIManager::Job::Mode::WriteRead;
			job.sem_complete = sem_twicallback;
			job.address = address;
			
			//Set address for SMB to read
			uint8_t regaddress = cs(block.regs[0]);
			job.outbuffer = &regaddress;
			job.writelen = 1;

			//Set buffer to read to
			job.inbuffer = block.buf;
			job.readlen = block.regs.size();
			
			twiManager->addJob(job, portMAX_DELAY);
			//Wait until block has been read
			xSemaphoreTake(sem_twicallback, portMAX_DELAY);
		}

		xSemaphoreTake(mtx_regcache, portMAX_DELAY);
		//Update cached values
		for(auto &&block : readBlocks) {
			for(uint8_t i = 0; i < block.regs.size(); i++) {
				regcache[regpos(block.regs[i])] = block.buf[i];
			}
			//Free memory used by block
			vPortFree(block.buf);
		}
		xSemaphoreGive(mtx_regcache);
		//Wait until next update
		vTaskDelayUntil(&previousWakeTime, config::emc1701::refreshRate);
	}
}
