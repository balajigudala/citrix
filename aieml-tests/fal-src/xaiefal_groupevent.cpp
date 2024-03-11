/******************************************************************************
* Copyright (C) 2022 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/


/*****************************************************************************/
/**
* @file xaiefal_groupevent.cpp
* @{
*
* This file contains the test application of group events for aieml.
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
#include <xaiengine.h>
#include <xaiefal/xaiefal.hpp>

#include "hw_config.h"

using namespace std;
using namespace xaiefal;
/************************** Constant Definitions *****************************/
#define THRESHOLD 10000
/*****************************************************************************/
/**
*
* This is the main entry point for the AIEFAL memtile group event example.
*
* @param	None.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_aiefal_memtile_groupevent(XAie_DevInst *DevInst)
{
	XAie_LocType Loc = XAie_TileLoc(1, XAIE_MEM_TILE_ROW_START);
	XAie_ModuleType Mod = XAIE_MEM_MOD;
	uint8_t Status, PendingBd = 4;
	XAie_DmaDesc Tile_MM2S;
	XAie_Events Event;
	AieRC RC;

	/* Instantiate FAL object */
	XAieDev Aie(DevInst, false);

	/* Get Group Event object */
	Event = XAIE_EVENT_GROUP_DMA_ACTIVITY_MEM_TILE;
	auto GroupEvent = Aie.tile(Loc).mem().groupEvent(Event);
	RC = GroupEvent->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve resource" << endl;
		return -1;
	}
	Event = GroupEvent->getEvent();
	if(Event != XAIE_EVENT_GROUP_DMA_ACTIVITY_MEM_TILE) {
		cout << "GetEvent returned incorrect event" << endl;
		return -1;
	}

	/* Start */
	RC = GroupEvent->start();
	if(RC != XAIE_OK) {
		cout << "Failed to start resource" << endl;
		return -1;
	}

	/* Test DMA Group Event, select DMA channel to generate events
	 * and initiate DMA transaction
	 */
	RC = XAie_EventSelectDmaChannel(Aie.dev(), Loc, 0, DMA_MM2S, 0);
	if(RC != XAIE_OK) {
		cout << "Failed to set DMA event selection" << endl;
		return -1;
	}
	RC = XAie_StrmConnCctEnable(Aie.dev(), Loc, DMA, 0, NORTH, 0);
	if(RC != XAIE_OK) {
		cout << "Failed to enable stream switch" << endl;
		return -1;
	}
	RC = XAie_DmaDescInit(Aie.dev(), &Tile_MM2S, Loc);
	if(RC != XAIE_OK) {
		cout << "Failed to init descriptor" << endl;
		return -1;
	}
	RC = XAie_DmaSetAddrLen(&Tile_MM2S, 0x80000, 32U);
	if(RC != XAIE_OK) {
		cout << "Failed to set addr len" << endl;
		return -1;
	}
	RC = XAie_DmaEnableBd(&Tile_MM2S);
	if(RC != XAIE_OK) {
		cout << "Failed to enable bd" << endl;
		return -1;
	}
	RC = XAie_DmaWriteBd(Aie.dev(), &Tile_MM2S, Loc, 0);
	if(RC != XAIE_OK) {
		cout << "Failed to write bd" << endl;
		return -1;
	}
	RC = XAie_DmaChannelPushBdToQueue(Aie.dev(), Loc, 0, DMA_MM2S, 0);
	if(RC != XAIE_OK) {
		cout << "Failed to push bd to queue" << endl;
		return -1;
	}

	while(PendingBd != 0) {
		XAie_DmaGetPendingBdCount(Aie.dev(), Loc, 0, DMA_MM2S,
					  &PendingBd);
	}

	/* After DMA transaction see if channel generated group events */
	RC = XAie_EventReadStatus(Aie.dev(), Loc, Mod, Event, &Status);
	if(!Status) {
		cout << "Group Event not generated" << endl;
		return -1;
	}

	/* Stop */
	RC = GroupEvent->stop();
	if(RC != XAIE_OK) {
		cout << "Failed to stop resource" << endl;
		return -1;
	}

	/* Release */
	RC = GroupEvent->release();
	if(RC != XAIE_OK) {
		cout << "Failed to release resource" << endl;
		return -1;
	}

	cout << "AIEFAL Memtile Group Event success." << endl;
	return 0;
}

/*****************************************************************************/
/**
*
* This is the main entry point for the AIEFAL shimtile group event example.
*
* @param	None.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_aiefal_shimtile_groupevent(XAie_DevInst *DevInst)
{
	XAie_LocType Loc = XAie_TileLoc(1, XAIE_SHIM_ROW);
	XAie_ModuleType Mod = XAIE_PL_MOD;
	XAie_Events Event, PerfEvent;
	uint8_t Status;
	AieRC RC;

	/* Instantiate FAL object */
	XAieDev Aie(DevInst, false);

	/* Get Group Event object */
	Event = XAIE_EVENT_GROUP_0_PL;
	auto GroupEvent = Aie.tile(Loc).pl().groupEvent(Event);
	RC = GroupEvent->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve resource" << endl;
		return -1;
	}
	Event = GroupEvent->getEvent();
	if(Event != XAIE_EVENT_GROUP_0_PL) {
		cout << "GetEvent returned incorrect event" << endl;
		return -1;
	}

	/* Start */
	RC = GroupEvent->start();
	if(RC != XAIE_OK) {
		cout << "Failed to start resource" << endl;
		return -1;
	}

	/* Setup performance counter to generate group event */
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

	/* After Perfcounter event generated, see if group event is */
	RC = XAie_EventReadStatus(Aie.dev(), Loc, Mod, Event, &Status);
	if(!Status) {
		cout << "Group Event not generated" << endl;
		return -1;
	}

	/* Stop */
	RC = GroupEvent->stop();
	if(RC != XAIE_OK) {
		cout << "Failed to stop resource" << endl;
		return -1;
	}

	/* Release */
	RC = GroupEvent->release();
	if(RC != XAIE_OK) {
		cout << "Failed to release resource" << endl;
		return -1;
	}

	cout << "AIEFAL Memtile Group Event success." << endl;
	return 0;
}
/** @} */
