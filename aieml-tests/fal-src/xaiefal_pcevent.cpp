/******************************************************************************
* Copyright (C) 2022 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/


/*****************************************************************************/
/**
* @file xaiefal_pcevent.cpp
* @{
*
* This file contains the test application of pc events for aieml.
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
#define PC_ADDR 0x00031100
#define LS_ADDR 0x00031140
#define LE_ADDR 0x00031150
#define LC_ADDR 0x00031160

/*****************************************************************************/
/**
*
* This is the main entry point for the AIEFAL aietile pcrange event example.
*
* @param	None.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_aiefal_aietile_pcrange(XAie_DevInst *DevInst)
{
	XAie_LocType Loc = XAie_TileLoc(1, XAIE_AIE_TILE_ROW_START);
	uint32_t Addr0, Addr1;
	XAie_Events Event;
	uint8_t Status;
	AieRC RC;

	/* Instantiate FAL object */
	XAieDev Aie(DevInst, false);

	/* Get UserEvent object */
	auto PCRange = Aie.tile(Loc).core().pcRange();
	RC = PCRange->updatePcAddr(0x0, 0x100);
	if (RC != XAIE_OK) {
		cout << "Failed to update PC Addr" << endl;
		return -1;
	}

	PCRange->getPcAddr(Addr0, Addr1);
	if ((Addr0 != 0x0) || (Addr1 != 0x100)) {
		cout << "Get PC addresses do not match" << endl;
		return -1;
	}

	RC = PCRange->reserve();
	if (RC != XAIE_OK) {
		cout << "Failed to reserve PCRange" << endl;
		return -1;
	}

	RC = PCRange->getEvent(Event);
	if (RC != XAIE_OK) {
		cout << "Failed to get Event" << endl;
		return -1;
	}

	if (Event != XAIE_EVENT_PC_RANGE_0_1_CORE) {
		cout << "Get Event returned incorrect event" << endl;
		return -1;
	}


	RC = PCRange->start();
	if (RC != XAIE_OK) {
		cout << "Failed to start PCRange" << endl;
		return -1;
	}

	/* Write value to PC and loop registers */
	RC = XAie_Write32(Aie.dev(),
			_XAie_GetTileAddr(Aie.dev(), Loc.Row, Loc.Col) + PC_ADDR,
			0x0);
	if (RC != XAIE_OK) {
		cout << "Failed to write to PC" << endl;
		return -1;
	}
	RC = XAie_Write32(Aie.dev(),
			_XAie_GetTileAddr(Aie.dev(), Loc.Row, Loc.Col) + LS_ADDR,
			0x0);
	if (RC != XAIE_OK) {
		cout << "Failed to write to LS" << endl;
		return -1;
	}
	RC = XAie_Write32(Aie.dev(),
			_XAie_GetTileAddr(Aie.dev(), Loc.Row, Loc.Col) + LE_ADDR,
			0x100);
	if (RC != XAIE_OK) {
		cout << "Failed to write to LE" << endl;
		return -1;
	}
	RC = XAie_Write32(Aie.dev(),
			_XAie_GetTileAddr(Aie.dev(), Loc.Row, Loc.Col) + LC_ADDR,
			1U);
	if (RC != XAIE_OK) {
		cout << "Failed to write to LC" << endl;
		return -1;
	}

	RC = XAie_CoreEnable(Aie.dev(), Loc);
	if (RC != XAIE_OK) {
		cout << "Failed to enable core" << endl;
		return -1;
	}

	RC = XAie_EventReadStatus(Aie.dev(), Loc, XAIE_CORE_MOD, Event, &Status);
	if (RC != XAIE_OK) {
		cout << "Failed to read event status" << endl;
		return -1;
	}
	if (!Status) {
		cout << "PC range event not triggered" << endl;
		return -1;
	}

	RC = XAie_CoreDisable(Aie.dev(), Loc);
	if (RC != XAIE_OK) {
		cout << "Failed to disable core" << endl;
		return -1;
	}

	RC = PCRange->stop();
	if (RC != XAIE_OK) {
		cout << "Failed to stop PCRange" << endl;
		return -1;
	}

	RC = PCRange->release();
	if (RC != XAIE_OK) {
		cout << "Failed to release PCEvent" << endl;
		return -1;
	}

	cout << "AIEFAL Aietile PC Range success." << endl;
	return 0;
}
/*****************************************************************************/
/**
*
* This is the main entry point for the AIEFAL aietile pc event example.
*
* @param	None.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_aiefal_aietile_pcevent(XAie_DevInst *DevInst)
{
	XAie_LocType Loc = XAie_TileLoc(1, XAIE_AIE_TILE_ROW_START);
	XAie_Events Event;
	uint8_t Status;
	AieRC RC;

	/* Instantiate FAL object */
	XAieDev Aie(DevInst, false);

	/* Get UserEvent object */
	auto PCEvent = Aie.tile(Loc).core().pcEvent();
	RC = PCEvent->updatePcAddr(0x0);
	if (RC != XAIE_OK) {
		cout << "Failed to update PC Addr" << endl;
		return -1;
	}

	if (PCEvent->getPcAddr() != 0x0) {
		cout << "PC addresses do not match" << endl;
		return -1;
	}

	RC = PCEvent->reserve();
	if (RC != XAIE_OK) {
		cout << "Failed to reserve PCEvent" << endl;
		return -1;
	}

	RC = PCEvent->getEvent(Event);
	if (RC != XAIE_OK) {
		cout << "Failed to get Event" << endl;
		return -1;
	}

	XAie_LocType L; XAie_ModuleType M; uint32_t RscId;
	RC = PCEvent->getRscId(L, M, RscId);
	if (RC != XAIE_OK) {
		cout << "Failed to get RscId" << endl;
		return -1;
	}
	if (static_cast<uint32_t>(Event) !=
			(static_cast<uint32_t>(XAIE_EVENT_PC_0_CORE) + RscId)) {
		cout << "RscId and Event do no match" << endl;
		return -1;
	}


	RC = PCEvent->start();
	if (RC != XAIE_OK) {
		cout << "Failed to start PCEvent" << endl;
		return -1;
	}

	/* Write value to PC register */
	RC = XAie_Write32(Aie.dev(),
			_XAie_GetTileAddr(Aie.dev(), Loc.Row, Loc.Col) + PC_ADDR,
			0x0);
	if (RC != XAIE_OK) {
		cout << "Failed to write to PC" << endl;
		return -1;
	}

	RC = XAie_CoreEnable(Aie.dev(), Loc);
	if (RC != XAIE_OK) {
		cout << "Failed to enable core" << endl;
		return -1;
	}

	RC = XAie_EventReadStatus(Aie.dev(), Loc, XAIE_CORE_MOD, Event, &Status);
	if (RC != XAIE_OK) {
		cout << "Failed to read event status" << endl;
		return -1;
	}
	if (!Status) {
		cout << "PC event not triggered" << endl;
		return -1;
	}

	RC = XAie_CoreDisable(Aie.dev(), Loc);
	if (RC != XAIE_OK) {
		cout << "Failed to disable core" << endl;
		return -1;
	}

	RC = PCEvent->stop();
	if (RC != XAIE_OK) {
		cout << "Failed to stop PCEvent" << endl;
		return -1;
	}

	RC = PCEvent->release();
	if (RC != XAIE_OK) {
		cout << "Failed to release PCEvent" << endl;
		return -1;
	}

	auto PCEvent0 = Aie.tile(Loc).core().pcEvent();
	auto PCEvent1 = Aie.tile(Loc).core().pcEvent();
	auto PCEvent2 = Aie.tile(Loc).core().pcEvent();
	auto PCEvent3 = Aie.tile(Loc).core().pcEvent();
	auto PCEvent4 = Aie.tile(Loc).core().pcEvent();
	PCEvent0->updatePcAddr(0x0);
	PCEvent1->updatePcAddr(0x0);
	PCEvent2->updatePcAddr(0x0);
	PCEvent3->updatePcAddr(0x0);
	PCEvent4->updatePcAddr(0x0);

	PCEvent0->reserve();
	PCEvent1->reserve();
	PCEvent2->reserve();
	PCEvent3->reserve();
	RC = PCEvent4->reserve();
	if (RC == XAIE_OK) {
		cout << "Reserved extra PC" << endl;
		return -1;
	}

	cout << "AIEFAL Aietile PC Event success." << endl;
	return 0;
}
/** @} */
