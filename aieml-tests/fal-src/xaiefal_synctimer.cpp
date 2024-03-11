/******************************************************************************
* Copyright (C) 2024 Amd, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/


/*****************************************************************************/
/**
* @file xaiefal_synctimer.cpp
* @{
*
* This file contains the test application of timer sync for aieml
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who     Date     Changes
* ----- ------  -------- -----------------------------------------------------
* 1.0   Gregory 03/05/24 Initial creation
* </pre>
*
******************************************************************************/
/***************************** Include Files *********************************/
#include <unistd.h>
#include <iostream>
#include <xaiengine.h>
#include <xaiefal/xaiefal.hpp>

#include "hw_config.h"

using namespace std;
using namespace xaiefal;
/************************** Constant Definitions *****************************/
#define LSB_NIBBLE	0xFFFF0000
#define NUM_TILES	5

int test_aiefal_timersync(XAie_DevInst *DevInst)
{
	XAie_LocType Locs[NUM_TILES] = {
		XAie_TileLoc(4, 0),
		XAie_TileLoc(5, 1),
		XAie_TileLoc(6, 8),
		XAie_TileLoc(2, 5),
		XAie_TileLoc(9, 0),
	};
	uint64_t prev[NUM_TILES];
	AieRC RC = XAIE_OK;
	vector<XAie_LocType> vLocs;

	/* Instantiate FAL object */
	XAieDev Aie(DevInst, false);

	/* Get Broadcast Channel */
	auto BC = Aie.broadcast(vLocs, XAIE_PL_MOD, XAIE_PL_MOD);
	RC = BC->reserve();
	if (RC != XAIE_OK) {
		cout << "Failed to reserve broadcast channel" << endl;
		return -1;
	}

	RC = XAie_ReadTimer(DevInst, Locs[0], XAIE_PL_MOD, &prev[0]);
	if (RC != XAIE_OK) {
		cout << "Failed to read timer for Loc 0" << endl;
		return -1;
	}

	RC = XAie_ReadTimer(DevInst, Locs[1], XAIE_MEM_MOD, &prev[1]);
	if (RC != XAIE_OK) {
		cout << "Failed to read timer for Loc 1" << endl;
		return -1;
	}

	RC = XAie_ReadTimer(DevInst, Locs[2], XAIE_CORE_MOD, &prev[2]);
	if (RC != XAIE_OK) {
		cout << "Failed to read timer for Loc 2" << endl;
		return -1;
	}

	RC = XAie_ReadTimer(DevInst, Locs[3], XAIE_MEM_MOD, &prev[3]);
	if (RC != XAIE_OK) {
		cout << "Failed to read timer for Loc 3" << endl;
		return -1;
	}

	RC = XAie_ReadTimer(DevInst, Locs[4], XAIE_PL_MOD, &prev[4]);
	if (RC != XAIE_OK) {
		cout << "Failed to read timer for Loc 4" << endl;
		return -1;
	}

        for(int i = 0; i < NUM_TILES; i++)
		cout << "Timer value: 0x" << hex << prev[i] << endl;


        //reset a timer
        RC = XAie_ResetTimer(DevInst, Locs[1], XAIE_MEM_MOD);
	if (RC != XAIE_OK) {
		cout << "Failed to reset timer" << endl;
		return -1;
	}
        //Get timer value for 5,1
        RC = XAie_ReadTimer(DevInst, Locs[1], XAIE_MEM_MOD, &prev[1]);
	if (RC != XAIE_OK) {
		cout << "Failed to read timer" << endl;
		return -1;
	}

        for(int i = 0; i < NUM_TILES; i++)
		cout << "Timer value: 0x" << hex << prev[i] << endl;

        RC = XAie_SyncTimer(DevInst, BC->getBc());
	if (RC != XAIE_OK) {
		cout << "Failed to sync timer" << endl;
		return -1;
	}

	RC = XAie_ReadTimer(DevInst, Locs[0], XAIE_PL_MOD, &prev[0]);
	if (RC != XAIE_OK) {
		cout << "Failed to read timer for Loc 0" << endl;
		return -1;
	}

	RC = XAie_ReadTimer(DevInst, Locs[1], XAIE_MEM_MOD, &prev[1]);
	if (RC != XAIE_OK) {
		cout << "Failed to read timer for Loc 1" << endl;
		return -1;
	}

	RC = XAie_ReadTimer(DevInst, Locs[2], XAIE_CORE_MOD, &prev[2]);
	if (RC != XAIE_OK) {
		cout << "Failed to read timer for Loc 2" << endl;
		return -1;
	}

	RC = XAie_ReadTimer(DevInst, Locs[3], XAIE_MEM_MOD, &prev[3]);
	if (RC != XAIE_OK) {
		cout << "Failed to read timer for Loc 3" << endl;
		return -1;
	}

	RC = XAie_ReadTimer(DevInst, Locs[4], XAIE_PL_MOD, &prev[4]);
	if (RC != XAIE_OK) {
		cout << "Failed to read timer for Loc 4" << endl;
		return -1;
	}


        //print timer values
        for(int i = 0; i < NUM_TILES; i++) {
		cout << "Timer value: 0x" << hex << prev[i] << endl;
                if ((prev[0] & LSB_NIBBLE) != (prev[i] & LSB_NIBBLE)) {
			cout << "Test failed, timers not synced" << endl;
			return -1;
		}
        }

        return 0;
}
