/******************************************************************************
* Copyright (C) 2022 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/


/*****************************************************************************/
/**
* @file xaiefal_userevent.cpp
* @{
*
* This file contains the test application of user events for aieml.
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
* This is the main entry point for the AIEFAL memtile user event example.
*
* @param	None.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_aiefal_memtile_userevent(XAie_DevInst *DevInst)
{
	XAie_LocType Loc = XAie_TileLoc(1, XAIE_MEM_TILE_ROW_START);
	XAie_Events Event0, Event1;
	XAie_Events Event;
	AieRC RC;

	/* Instantiate FAL object */
	XAieDev Aie(DevInst, false);

	/* Get UserEvent object */
	auto UserEvent = Aie.tile(Loc).mem().userEvent();
	RC = UserEvent->getEvent(Event);
	if(RC != XAIE_INVALID_ARGS) {
		cout << "GetEvent did not return Invalid args w/o reserve" <<
			endl;
		return -1;
	}

	/* Reserve */
	RC = UserEvent->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve resource" << endl;
		return -1;
	}

	/* Check Event */
	RC = UserEvent->getEvent(Event);
	if(RC != XAIE_OK) {
		cout << "GetEvent failed after reserve" << endl;
		return -1;
	}
	if(Event != XAIE_EVENT_USER_EVENT_0_MEM_TILE) {
		cout << "GetEvent returned incorrect event" << endl;
		return -1;
	}

	/* Start */
	RC = UserEvent->start();
	if(RC != XAIE_OK) {
		cout << "Failed to start resource" << endl;
		return -1;
	}

	/* Stop */
	RC = UserEvent->stop();
	if(RC != XAIE_OK) {
		cout << "Failed to stop resource" << endl;
		return -1;
	}

	/* Release */
	RC = UserEvent->release();
	if(RC != XAIE_OK) {
		cout << "Failed to release resource" << endl;
		return -1;
	}

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

	if((Event0 != XAIE_EVENT_USER_EVENT_0_MEM_TILE) ||
			(Event1 != XAIE_EVENT_USER_EVENT_1_MEM_TILE)) {
		cout << "GetEvents returned incorrectly" << endl;
		return -1;
	}

	auto UserEvent2 = Aie.tile(Loc).mem().userEvent();
	RC = UserEvent2->reserve();
	if(RC == XAIE_OK) {
		cout << "Grabed 3rd UserEvent" << endl;
		return -1;
	}

	cout << "AIEFAL Memtile User Event success." << endl;
	return 0;
}

/*****************************************************************************/
/**
*
* This is the main entry point for the AIEFAL shimtile user event example.
*
* @param	None.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_aiefal_shimtile_userevent(XAie_DevInst *DevInst)
{
	XAie_LocType Loc = XAie_TileLoc(1, XAIE_SHIM_ROW);
	XAie_Events Event0, Event1;
	XAie_Events Event;
	AieRC RC;

	/* Instantiate FAL object */
	XAieDev Aie(DevInst, false);

	/* Get UserEvent object */
	auto UserEvent = Aie.tile(Loc).pl().userEvent();
	RC = UserEvent->getEvent(Event);
	if(RC != XAIE_INVALID_ARGS) {
		cout << "GetEvent did not return Invalid args w/o reserve" <<
			endl;
		return -1;
	}

	/* Reserve */
	RC = UserEvent->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve resource" << endl;
		return -1;
	}

	/* Check Event */
	RC = UserEvent->getEvent(Event);
	if(RC != XAIE_OK) {
		cout << "GetEvent failed after reserve" << endl;
		return -1;
	}
	if(Event != XAIE_EVENT_USER_EVENT_0_PL) {
		cout << "GetEvent returned incorrect event" << endl;
		return -1;
	}

	/* Start */
	RC = UserEvent->start();
	if(RC != XAIE_OK) {
		cout << "Failed to start resource" << endl;
		return -1;
	}

	/* Stop */
	RC = UserEvent->stop();
	if(RC != XAIE_OK) {
		cout << "Failed to stop resource" << endl;
		return -1;
	}

	/* Release */
	RC = UserEvent->release();
	if(RC != XAIE_OK) {
		cout << "Failed to release resource" << endl;
		return -1;
	}

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

	if((Event0 != XAIE_EVENT_USER_EVENT_0_PL) ||
			(Event1 != XAIE_EVENT_USER_EVENT_1_PL)) {
		cout << "GetEvents returned incorrectly" << endl;
		return -1;
	}

	auto UserEvent2 = Aie.tile(Loc).pl().userEvent();
	RC = UserEvent2->reserve();
	if(RC == XAIE_OK) {
		cout << "Grabed 3rd UserEvent" << endl;
		return -1;
	}

	cout << "AIEFAL Shimtile User Event success." << endl;
	return 0;
}
/** @} */
