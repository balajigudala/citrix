/******************************************************************************
* Copyright (C) 2022 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/


/*****************************************************************************/
/**
* @file xaiefal_comboevent.cpp
* @{
*
* This file contains the test application of combo events for aieml.
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
* This is the main entry point for the AIEFAL memtile combo event example.
*
* @param	None.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_aiefal_memtile_comboevent(XAie_DevInst *DevInst)
{
	XAie_LocType Loc = XAie_TileLoc(1, XAIE_MEM_TILE_ROW_START);
	XAie_ModuleType Mod = XAIE_MEM_MOD;
	vector<XAie_EventComboOps> vOpts;
	vector<XAie_Events> vComboEvents;
	vector<XAie_Events> vEvents;
	XAie_Events Event0, Event1;
	uint8_t Status;
	AieRC RC;

	/* Instantiate FAL object */
	XAieDev Aie(DevInst, false);

	/* Reserve User Events to add to Combo */
	auto UserEvent0 = Aie.tile(Loc).mem().userEvent();
	auto UserEvent1 = Aie.tile(Loc).mem().userEvent();
	RC = UserEvent0->reserve();
	RC = UserEvent1->reserve();
	RC = UserEvent0->start();
	RC = UserEvent1->start();
	RC = UserEvent0->getEvent(Event0);
	RC = UserEvent1->getEvent(Event1);

	vEvents.push_back(Event0);
	vEvents.push_back(Event1);
	vOpts.push_back(XAIE_EVENT_COMBO_E1_OR_E2);

	/* Get ComboEvent object */
	auto ComboEvent = Aie.tile(Loc).mem().comboEvent();

	/* Set events to userevents and trigger on OR of events */
	RC = ComboEvent->setEvents(vEvents, vOpts);
	if(RC != XAIE_OK) {
		cout << "Failed to reserve Combo event" << endl;
		return -1;
	}

	RC = ComboEvent->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve Combo event" << endl;
		return -1;
	}

	RC = ComboEvent->start();
	if(RC != XAIE_OK) {
		cout << "Failed to start Combo event" << endl;
		return -1;
	}

	/* Get Combo Event */
	RC = ComboEvent->getEvents(vComboEvents);
	if(RC != XAIE_OK) {
		cout << "Failed to reserve Combo event" << endl;
		return -1;
	}

	/* Generate user events to trigger combo event */
	RC = XAie_EventGenerate(Aie.dev(), Loc, Mod, Event0);
	if(RC != XAIE_OK) {
		cout << "Failed to generate event0" << endl;
		return -1;
	}

	RC = XAie_EventGenerate(Aie.dev(), Loc, Mod, Event1);
	if(RC != XAIE_OK) {
		cout << "Failed to generate event1" << endl;
		return -1;
	}

	/* Check to see if combo event was generated */
	RC = XAie_EventReadStatus(Aie.dev(), Loc, Mod, vComboEvents[0], &Status);
	if(!Status) {
		cout << "Combo Event not generated" << endl;
		return -1;
	}

	RC = ComboEvent->stop();
	if(RC != XAIE_OK) {
		cout << "Failed to stop Combo event" << endl;
		return -1;
	}

	RC = ComboEvent->release();
	if(RC != XAIE_OK) {
		cout << "Failed to release Combo event" << endl;
		return -1;
	}

	cout << "AIEFAL Memtile Combo Event success." << endl;
	return 0;
}

/*****************************************************************************/
/**
*
* This is the main entry point for the AIEFAL shimtile combo event example.
*
* @param	None.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_aiefal_shimtile_comboevent(XAie_DevInst *DevInst)
{
	XAie_LocType Loc = XAie_TileLoc(1, XAIE_SHIM_ROW);
	XAie_ModuleType Mod = XAIE_PL_MOD;
	vector<XAie_EventComboOps> vOpts;
	vector<XAie_Events> vComboEvents;
	vector<XAie_Events> vEvents;
	XAie_Events Event0, Event1;
	uint8_t Status;
	AieRC RC;

	/* Instantiate FAL object */
	XAieDev Aie(DevInst, false);

	/* Reserve User Events to add to Combo */
	auto UserEvent0 = Aie.tile(Loc).pl().userEvent();
	auto UserEvent1 = Aie.tile(Loc).pl().userEvent();
	RC = UserEvent0->reserve();
	RC = UserEvent1->reserve();
	RC = UserEvent0->start();
	RC = UserEvent1->start();
	RC = UserEvent0->getEvent(Event0);
	RC = UserEvent1->getEvent(Event1);

	vEvents.push_back(Event0);
	vEvents.push_back(Event1);
	vOpts.push_back(XAIE_EVENT_COMBO_E1_OR_E2);

	/* Get ComboEvent object */
	auto ComboEvent = Aie.tile(Loc).pl().comboEvent();

	/* Set events to userevents and trigger on OR of events */
	RC = ComboEvent->setEvents(vEvents, vOpts);
	if(RC != XAIE_OK) {
		cout << "Failed to reserve Combo event" << endl;
		return -1;
	}

	RC = ComboEvent->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve Combo event" << endl;
		return -1;
	}

	RC = ComboEvent->start();
	if(RC != XAIE_OK) {
		cout << "Failed to start Combo event" << endl;
		return -1;
	}

	/* Get Combo Event */
	RC = ComboEvent->getEvents(vComboEvents);
	if(RC != XAIE_OK) {
		cout << "Failed to reserve Combo event" << endl;
		return -1;
	}

	/* Generate user events to trigger combo event */
	RC = XAie_EventGenerate(Aie.dev(), Loc, Mod, Event0);
	if(RC != XAIE_OK) {
		cout << "Failed to generate event0" << endl;
		return -1;
	}

	RC = XAie_EventGenerate(Aie.dev(), Loc, Mod, Event1);
	if(RC != XAIE_OK) {
		cout << "Failed to generate event1" << endl;
		return -1;
	}

	/* Check to see if combo event was generated */
	RC = XAie_EventReadStatus(Aie.dev(), Loc, Mod, vComboEvents[0], &Status);
	if(!Status) {
		cout << "Combo Event not generated" << endl;
		return -1;
	}

	RC = ComboEvent->stop();
	if(RC != XAIE_OK) {
		cout << "Failed to stop Combo event" << endl;
		return -1;
	}

	RC = ComboEvent->release();
	if(RC != XAIE_OK) {
		cout << "Failed to release Combo event" << endl;
		return -1;
	}

	cout << "AIEFAL Shimtile Combo Event success." << endl;
	return 0;
}
/** @} */
