/******************************************************************************
* Copyright (C) 2022 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/


/*****************************************************************************/
/**
* @file xaiefal_perfcounter.cpp
* @{
*
* This file contains the test application of performance counters for aieml.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who     Date     Changes
* ----- ------  -------- -----------------------------------------------------
* 1.0   Gregory 04/24/2023  Initial creation
* </pre>
*
******************************************************************************/
/***************************** Include Files *********************************/
#include <iostream>
#include <stdlib.h>
#include <xaiengine.h>
#include <xaiefal/xaiefal.hpp>

#include "hw_config.h"

using namespace std;
using namespace xaiefal;
/************************** Constant Definitions *****************************/
#define THRESHOLD	1000000
/*****************************************************************************/
/**
*
* This is the main entry point for the AIEFAL memtile perfcounter event
* example.
*
* @param	None.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_aiefal_memtile_perfcounter_event(XAie_DevInst *DevInst)
{
	XAie_LocType Loc = XAie_TileLoc(1, XAIE_MEM_TILE_ROW_START);
	XAie_ModuleType Mod = XAIE_MEM_MOD;
	XAie_Events PerfEvent;
	uint8_t status = 0;
	AieRC RC;

	/* Instantiate FAL object */
	XAieDev Aie(DevInst, false);

	/* Test threshold API */
	auto PCounterThresh = Aie.tile(Loc).mem().perfCounter();
	RC = PCounterThresh->initialize(Mod, XAIE_EVENT_TRUE_MEM_TILE,
					Mod, XAIE_EVENT_TRUE_MEM_TILE,
				       	Mod, XAIE_EVENT_NONE_MEM_TILE);
	if (RC != XAIE_OK) {
		cout << "Failed to initialize perf counterthresh" << endl;
		return -1;
	}

	RC = PCounterThresh->changeThreshold(THRESHOLD);
	if (RC != XAIE_OK) {
		cout << "Failed to change threshold" << endl;
		return -1;
	}

	RC = PCounterThresh->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve perf counterthresh" << endl;
		return -1;
	}

	RC = PCounterThresh->getCounterEvent(Mod, PerfEvent);
	if(RC != XAIE_OK) {
		cout << "Failed to get counter event" << endl;
		return -1;
	}

	RC = XAie_ResetTimer(Aie.dev(), Loc, Mod);
	if(RC != XAIE_OK) {
		cout << "Failed to reset timer" << endl;
		return -1;
	}

	RC = PCounterThresh->start();
	if(RC != XAIE_OK) {
		cout << "Failed to start perf counterthresh" << endl;
		return -1;
	}

	/* Wait for Perfcnt threshold to be reached */
	RC = XAie_WaitCycles(Aie.dev(), Loc, Mod, THRESHOLD);
	if(RC != XAIE_OK) {
		cout << "Failed to wait cycles" << endl;
		return -1;
	}

	/* Check if perfcounter generated event */
	RC = XAie_EventReadStatus(Aie.dev(), Loc, Mod,
				  PerfEvent, &status);
	if(RC != XAIE_OK) {
		cout << "Failed to read event status" << endl;
		return -1;
	}

	if(!status) {
		cout << "Performance counter event not triggered" << endl;
		return -1;
	}

	cout << "AIEFAL Memtile Performance Counter Event success." << endl;
	return 0;
}

/*****************************************************************************/
/**
*
* This is the main entry point for the AIEFAL memtile perfcounter example.
*
* @param	None.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_aiefal_memtile_perfcounter(XAie_DevInst *DevInst)
{
	XAie_LocType Loc = XAie_TileLoc(1, XAIE_MEM_TILE_ROW_START);
	uint32_t Result = 0, EventCount = 0;
	XAie_Events Event0, Event1;
	XAie_ModuleType Mod = XAIE_MEM_MOD;
	AieRC RC;

	/* Instantiate FAL object */
	XAieDev Aie(DevInst, false);

	/* Get Perfcounter object */
	auto PCounter = Aie.tile(Loc).mem().perfCounter();
	RC = PCounter->initialize(Mod, XAIE_EVENT_NONE_MEM_TILE,
				  Mod, XAIE_EVENT_NONE_MEM_TILE,
				  Mod, XAIE_EVENT_NONE_MEM_TILE);
	if (RC != XAIE_OK) {
		cout << "Failed to initialize perfcounter" << endl;
		return -1;
	}

	/* Get User Events to start/stop/rst perf counter */
	auto UserEvent0 = Aie.tile(Loc).mem().userEvent();
	auto UserEvent1 = Aie.tile(Loc).mem().userEvent();

	RC = UserEvent0->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve user event0" << endl;
		return -1;
	}
	RC = UserEvent1->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve user event1" << endl;
		return -1;
	}

	RC = UserEvent0->getEvent(Event0);
	if(RC != XAIE_OK) {
		cout << "Failed to get user event0" << endl;
		return -1;
	}
	RC = UserEvent1->getEvent(Event1);
	if(RC != XAIE_OK) {
		cout << "Failed to get user event1" << endl;
		return -1;
	}

	RC = PCounter->changeStartEvent(Mod, Event0);
	if(RC != XAIE_OK) {
		cout << "Failed to change start event" << endl;
		return -1;
	}

	RC = PCounter->changeStopEvent(Mod, Event0);
	if(RC != XAIE_OK) {
		cout << "Failed to change stop event" << endl;
		return -1;
	}

	RC = PCounter->changeRstEvent(Mod, Event1);
	if(RC != XAIE_OK) {
		cout << "Failed to change reset event" << endl;
		return -1;
	}

	RC = PCounter->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve perf counter" << endl;
		return -1;
	}

	RC = PCounter->start();
	if(RC != XAIE_OK) {
		cout << "Failed to start perf counter" << endl;
		return -1;
	}

	/* Since start/stop event are the same, perfcounter should
	 * count the amount of times event is triggered
	 */
	for(int i = 0; i < 10; i++) {
		RC = XAie_EventGenerate(Aie.dev(), Loc, Mod, Event0);
		if(RC != XAIE_OK) {
			cout << "Failed to generate Event " << i << endl;
		} else {
			EventCount++;
		}
	}

	RC = PCounter->readResult(Result);
	if(RC != XAIE_OK) {
		cout << "Failed to read result" << endl;
		return -1;
	}

	if((EventCount == 0) || (EventCount != Result)) {
		cout << "Mismatch in result: " << Result << "|" <<
			EventCount << endl;
		return -1;
	}

	RC = XAie_EventGenerate(Aie.dev(), Loc, Mod, Event1);
	if(RC != XAIE_OK) {
		cout << "Failed to generate Event for reset" << endl;
		return -1;
	}

	RC = PCounter->readResult(Result);
	if(RC != XAIE_OK) {
		cout << "Failed to read result" << endl;
		return -1;
	}

	if(Result != 0) {
		cout << "Counter not reset by User Event 1" << endl;
		return -1;
	}

	RC = PCounter->stop();
	if(RC != XAIE_OK) {
		cout << "Failed to stop perf counter" << endl;
		return -1;
	}

	RC = PCounter->release();
	if(RC != XAIE_OK) {
		cout << "Failed to release perf counter" << endl;
		return -1;
	}

	/* Test too many requests */
	auto PCounter0 = Aie.tile(Loc).mem().perfCounter();
	auto PCounter1 = Aie.tile(Loc).mem().perfCounter();
	auto PCounter2 = Aie.tile(Loc).mem().perfCounter();
	auto PCounter3 = Aie.tile(Loc).mem().perfCounter();
	auto PCounter4 = Aie.tile(Loc).mem().perfCounter();
	PCounter0->initialize(Mod, XAIE_EVENT_NONE_MEM_TILE, Mod, XAIE_EVENT_NONE_MEM_TILE,
			Mod, XAIE_EVENT_NONE_MEM_TILE);
	PCounter1->initialize(Mod, XAIE_EVENT_NONE_MEM_TILE, Mod, XAIE_EVENT_NONE_MEM_TILE,
			Mod, XAIE_EVENT_NONE_MEM_TILE);
	PCounter2->initialize(Mod, XAIE_EVENT_NONE_MEM_TILE, Mod, XAIE_EVENT_NONE_MEM_TILE,
			Mod, XAIE_EVENT_NONE_MEM_TILE);
	PCounter3->initialize(Mod, XAIE_EVENT_NONE_MEM_TILE, Mod, XAIE_EVENT_NONE_MEM_TILE,
			Mod, XAIE_EVENT_NONE_MEM_TILE);
	PCounter4->initialize(Mod, XAIE_EVENT_NONE_MEM_TILE, Mod, XAIE_EVENT_NONE_MEM_TILE,
			Mod, XAIE_EVENT_NONE_MEM_TILE);

	RC = PCounter0->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve perf counter 0" << endl;
		return -1;
	}
	RC = PCounter1->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve perf counter 1" << endl;
		return -1;
	}
	RC = PCounter2->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve perf counter 2" << endl;
		return -1;
	}
	RC = PCounter3->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve perf counter 3" << endl;
		return -1;
	}
	RC = PCounter4->reserve();
	if(RC == XAIE_OK) {
		cout << "Reserved invalid perf counter 4" << endl;
		return -1;
	}

	cout << "AIEFAL Memtile Performance Counter success." << endl;
	return 0;
}

/*****************************************************************************/
/**
*
* This is the main entry point for the AIEFAL shimtile perfcounter event
* example.
*
* @param	None.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_aiefal_shimtile_perfcounter_event(XAie_DevInst *DevInst)
{
	XAie_LocType Loc = XAie_TileLoc(1, XAIE_SHIM_ROW);
	XAie_ModuleType Mod = XAIE_PL_MOD;
	XAie_Events PerfEvent;
	uint8_t status = 0;
	AieRC RC;

	/* Instantiate FAL object */
	XAieDev Aie(DevInst, false);

	/* Test threshold API */
	auto PCounterThresh = Aie.tile(Loc).pl().perfCounter();
	RC = PCounterThresh->initialize(Mod, XAIE_EVENT_TRUE_PL,
					Mod, XAIE_EVENT_TRUE_PL,
				       	Mod, XAIE_EVENT_NONE_PL);
	if (RC != XAIE_OK) {
		cout << "Failed to initialize perf counterthresh" << endl;
		return -1;
	}

	RC = PCounterThresh->changeThreshold(THRESHOLD);
	if (RC != XAIE_OK) {
		cout << "Failed to change threshold" << endl;
		return -1;
	}

	RC = PCounterThresh->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve perf counterthresh" << endl;
		return -1;
	}

	RC = PCounterThresh->getCounterEvent(Mod, PerfEvent);
	if(RC != XAIE_OK) {
		cout << "Failed to get counter event" << endl;
		return -1;
	}

	RC = XAie_ResetTimer(Aie.dev(), Loc, Mod);
	if(RC != XAIE_OK) {
		cout << "Failed to reset timer" << endl;
		return -1;
	}

	RC = PCounterThresh->start();
	if(RC != XAIE_OK) {
		cout << "Failed to start perf counterthresh" << endl;
		return -1;
	}

	/* Wait for Perfcnt threshold to be reached */
	RC = XAie_WaitCycles(Aie.dev(), Loc, Mod, THRESHOLD);
	if(RC != XAIE_OK) {
		cout << "Failed to wait cycles" << endl;
		return -1;
	}

	/* Check if perfcounter generated event */
	RC = XAie_EventReadStatus(Aie.dev(), Loc, Mod,
				  PerfEvent, &status);
	if(RC != XAIE_OK) {
		cout << "Failed to read event status" << endl;
		return -1;
	}

	if(!status) {
		cout << "Performance counter event not triggered" << endl;
		return -1;
	}

	cout << "AIEFAL Shimtile Performance Counter Event success." << endl;
	return 0;
}

/*****************************************************************************/
/**
*
* This is the main entry point for the AIEFAL shimtile perfcounter example.
*
* @param	None.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_aiefal_shimtile_perfcounter(XAie_DevInst *DevInst)
{
	XAie_LocType Loc = XAie_TileLoc(1, XAIE_SHIM_ROW);
	uint32_t Result = 0, EventCount = 0;
	XAie_Events Event0, Event1;
	XAie_ModuleType Mod = XAIE_PL_MOD;
	AieRC RC;

	/* Instantiate FAL object */
	XAieDev Aie(DevInst, false);

	/* Get Perfcounter object */
	auto PCounter = Aie.tile(Loc).pl().perfCounter();
	RC = PCounter->initialize(Mod, XAIE_EVENT_NONE_PL,
				  Mod, XAIE_EVENT_NONE_PL,
				  Mod, XAIE_EVENT_NONE_PL);
	if (RC != XAIE_OK) {
		cout << "Failed to initialize perfcounter" << endl;
		return -1;
	}

	/* Get User Events to start/stop/rst perf counter */
	auto UserEvent0 = Aie.tile(Loc).pl().userEvent();
	auto UserEvent1 = Aie.tile(Loc).pl().userEvent();

	RC = UserEvent0->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve user event0" << endl;
		return -1;
	}
	RC = UserEvent1->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve user event1" << endl;
		return -1;
	}

	RC = UserEvent0->getEvent(Event0);
	if(RC != XAIE_OK) {
		cout << "Failed to get user event0" << endl;
		return -1;
	}
	RC = UserEvent1->getEvent(Event1);
	if(RC != XAIE_OK) {
		cout << "Failed to get user event1" << endl;
		return -1;
	}

	RC = PCounter->changeStartEvent(Mod, Event0);
	if(RC != XAIE_OK) {
		cout << "Failed to change start event" << endl;
		return -1;
	}

	RC = PCounter->changeStopEvent(Mod, Event0);
	if(RC != XAIE_OK) {
		cout << "Failed to change stop event" << endl;
		return -1;
	}

	RC = PCounter->changeRstEvent(Mod, Event1);
	if(RC != XAIE_OK) {
		cout << "Failed to change reset event" << endl;
		return -1;
	}

	RC = PCounter->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve perf counter" << endl;
		return -1;
	}

	RC = PCounter->start();
	if(RC != XAIE_OK) {
		cout << "Failed to start perf counter" << endl;
		return -1;
	}

	/* Since start/stop event are the same, perfcounter should
	 * count the amount of times event is triggered
	 */
	for(int i = 0; i < 10; i++) {
		RC = XAie_EventGenerate(Aie.dev(), Loc, Mod, Event0);
		if(RC != XAIE_OK) {
			cout << "Failed to generate Event " << i << endl;
		} else {
			EventCount++;
		}
	}

	RC = PCounter->readResult(Result);
	if(RC != XAIE_OK) {
		cout << "Failed to read result" << endl;
		return -1;
	}

	if((EventCount == 0) || (EventCount != Result)) {
		cout << "Mismatch in result: " << Result << "|" <<
			EventCount << endl;
		return -1;
	}

	RC = XAie_EventGenerate(Aie.dev(), Loc, Mod, Event1);
	if(RC != XAIE_OK) {
		cout << "Failed to generate Event for reset" << endl;
		return -1;
	}

	RC = PCounter->readResult(Result);
	if(RC != XAIE_OK) {
		cout << "Failed to read result" << endl;
		return -1;
	}

	if(Result != 0) {
		cout << "Counter not reset by User Event 1" << endl;
		return -1;
	}

	RC = PCounter->stop();
	if(RC != XAIE_OK) {
		cout << "Failed to stop perf counter" << endl;
		return -1;
	}

	RC = PCounter->release();
	if(RC != XAIE_OK) {
		cout << "Failed to release perf counter" << endl;
		return -1;
	}

	/* Test too many requests */
	auto PCounter0 = Aie.tile(Loc).pl().perfCounter();
	auto PCounter1 = Aie.tile(Loc).pl().perfCounter();
	auto PCounter2 = Aie.tile(Loc).pl().perfCounter();
	PCounter0->initialize(Mod, XAIE_EVENT_NONE_PL, Mod, XAIE_EVENT_NONE_PL,
			Mod, XAIE_EVENT_NONE_PL);
	PCounter1->initialize(Mod, XAIE_EVENT_NONE_PL, Mod, XAIE_EVENT_NONE_PL,
			Mod, XAIE_EVENT_NONE_PL);
	PCounter2->initialize(Mod, XAIE_EVENT_NONE_PL, Mod, XAIE_EVENT_NONE_PL,
			Mod, XAIE_EVENT_NONE_PL);

	RC = PCounter0->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve perf counter 0" << endl;
		return -1;
	}
	RC = PCounter1->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve perf counter 1" << endl;
		return -1;
	}
	RC = PCounter2->reserve();
	if(RC == XAIE_OK) {
		cout << "Reserved invalid perf counter 2" << endl;
		return -1;
	}

	cout << "AIEFAL Shimtile Performance Counter success." << endl;
	return 0;
}
/** @} */
