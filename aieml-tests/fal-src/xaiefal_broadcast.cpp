/******************************************************************************
* Copyright (C) 2022 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/


/*****************************************************************************/
/**
* @file xaiefal_broadcast.cpp
* @{
*
* This file contains the test application of broadcast channels for aieml.
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
/*****************************************************************************/
/**
*
* This is the main entry point for the AIEFAL memtile broadcast select
* tiles example.
*
* @param	None.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_aiefal_memtile_broadcast(XAie_DevInst *DevInst)
{
	XAie_ModuleType Mod = XAIE_MEM_MOD;
	XAie_Events Event, BCastEvent;
	vector<XAie_LocType> vLocs;
	XAie_LocType OutLoc;
	uint8_t Status;
	AieRC RC;

	/* Instantiate FAL object */
	XAieDev Aie(DevInst, false);

	/* Test selected tiles broadcast */
	vLocs.push_back(XAie_TileLoc(0, XAIE_MEM_TILE_ROW_START));
	vLocs.push_back(XAie_TileLoc(1, XAIE_MEM_TILE_ROW_START));
	vLocs.push_back(XAie_TileLoc(2, XAIE_MEM_TILE_ROW_START));
	OutLoc = XAie_TileLoc(3, XAIE_MEM_TILE_ROW_START);

	/* Get UserEvent to broadcast */
	XAieUserEvent UserEvent(Aie, vLocs[0], Mod);
	UserEvent.reserve();
	UserEvent.start();
	UserEvent.getEvent(Event);

	/* Create Broadcast Object */
	auto BC = Aie.broadcast(vLocs, Mod, Mod);
	RC = BC->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve broadcast channel" << endl;
		return -1;
	}

	RC = BC->getEvent(vLocs[0], Mod, BCastEvent);
	if(RC != XAIE_OK) {
		cout << "Failed to get broadcast channel event" << endl;
		return -1;
	}

	RC = BC->start();
	if(RC != XAIE_OK) {
		cout << "Failed to start broadcast channel" << endl;
		return -1;
	}

	/* Setup broadcast network and generate event */
	RC = XAie_EventBroadcast(Aie.dev(), vLocs[0], Mod, BC->getBc(), Event);
	if(RC != XAIE_OK) {
		cout << "Failed to setup event broadcast" << endl;
		return -1;
	}

	RC = XAie_EventGenerate(Aie.dev(), vLocs[0], Mod, Event);
	if(RC != XAIE_OK) {
		cout << "Failed to generate event" << endl;
		return -1;
	}

	for(uint8_t i = 1; i < vLocs.size(); i++) {
		Status = 0;
		RC = XAie_EventReadStatus(Aie.dev(), vLocs[i], Mod, BCastEvent, &Status);
		if(RC != XAIE_OK) {
			cout << "Failed to read event status" << endl;
			return -1;
		}
		if(!Status) {
			cout << "Event not triggered on Loc: ("
				<< (int)vLocs[i].Col << ", " << (int)vLocs[i].Col
				<< ")" << endl;
			return -1;
		}
	}

	Status = 0;
	RC = XAie_EventReadStatus(Aie.dev(), OutLoc, Mod, BCastEvent, &Status);
	if(RC != XAIE_OK) {
		cout << "Failed to read event status" << endl;
		return -1;
	}
	if(Status) {
		cout << "Event triggered on Loc outside" << endl;
		return -1;
	}

	RC = BC->stop();
	if(RC != XAIE_OK) {
		cout << "Failed to stop broadcast channel" << endl;
		return -1;
	}

	RC = BC->release();
	if(RC != XAIE_OK) {
		cout << "Failed to release broadcast channel" << endl;
		return -1;
	}

	cout << "AIEFAL Memtile Broadcast success." << endl;
	return 0;
}

/*****************************************************************************/
/**
*
* This is the main entry point for the AIEFAL shimtile broadcast select
* tiles example.
*
* @param	None.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_aiefal_shimtile_broadcast(XAie_DevInst *DevInst)
{
	XAie_ModuleType Mod = XAIE_PL_MOD;
	XAie_Events Event, BCastEvent;
	vector<XAie_LocType> vLocs;
	XAie_LocType OutLoc;
	uint8_t Status;
	AieRC RC;

	/* Instantiate FAL object */
	XAieDev Aie(DevInst, false);

	/* Test selected tiles broadcast */
	vLocs.push_back(XAie_TileLoc(0, XAIE_SHIM_ROW));
	vLocs.push_back(XAie_TileLoc(1, XAIE_SHIM_ROW));
	vLocs.push_back(XAie_TileLoc(2, XAIE_SHIM_ROW));
	OutLoc = XAie_TileLoc(3, XAIE_SHIM_ROW);

	/* Get UserEvent to broadcast */
	XAieUserEvent UserEvent(Aie, vLocs[0], Mod);
	UserEvent.reserve();
	UserEvent.start();
	UserEvent.getEvent(Event);

	/* Create Broadcast Object */
	auto BC = Aie.broadcast(vLocs, Mod, Mod);
	RC = BC->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve broadcast channel" << endl;
		return -1;
	}

	RC = BC->getEvent(vLocs[0], Mod, BCastEvent);
	if(RC != XAIE_OK) {
		cout << "Failed to get broadcast channel event" << endl;
		return -1;
	}

	RC = BC->start();
	if(RC != XAIE_OK) {
		cout << "Failed to start broadcast channel" << endl;
		return -1;
	}

	/* Setup broadcast network and generate event */
	RC = XAie_EventBroadcast(Aie.dev(), vLocs[0], Mod, BC->getBc(), Event);
	if(RC != XAIE_OK) {
		cout << "Failed to setup event broadcast" << endl;
		return -1;
	}

	RC = XAie_EventGenerate(Aie.dev(), vLocs[0], Mod, Event);
	if(RC != XAIE_OK) {
		cout << "Failed to generate event" << endl;
		return -1;
	}

	for(uint8_t i = 1; i < vLocs.size(); i++) {
		Status = 0;
		RC = XAie_EventReadStatus(Aie.dev(), vLocs[i], Mod, BCastEvent, &Status);
		if(RC != XAIE_OK) {
			cout << "Failed to read event status" << endl;
			return -1;
		}
		if(!Status) {
			cout << "Event not triggered on Loc: ("
				<< (int)vLocs[i].Col << ", " << (int)vLocs[i].Col
				<< ")" << endl;
			return -1;
		}
	}

	Status = 0;
	RC = XAie_EventReadStatus(Aie.dev(), OutLoc, Mod, BCastEvent, &Status);
	if(RC != XAIE_OK) {
		cout << "Failed to read event status" << endl;
		return -1;
	}
	if(Status) {
		cout << "Event triggered on Loc outside" << endl;
		return -1;
	}

	RC = BC->stop();
	if(RC != XAIE_OK) {
		cout << "Failed to stop broadcast channel" << endl;
		return -1;
	}

	RC = BC->release();
	if(RC != XAIE_OK) {
		cout << "Failed to release broadcast channel" << endl;
		return -1;
	}

	cout << "AIEFAL Shimtile Broadcast success." << endl;
	return 0;
}
