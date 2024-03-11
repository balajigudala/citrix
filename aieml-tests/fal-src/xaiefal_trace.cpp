/******************************************************************************
* Copyright (C) 2022 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/


/*****************************************************************************/
/**
* @file xaiefal_trace.cpp
* @{
*
* This file contains the test application of event trace for aieml.
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
#define DATA_MEM_ADDR		0x80000
#define DATA_MEM_MAX		0x7FF00
#define NUM_TRACE_CYCLES 	1000
#define DMA_CHAN		5 	//Only channel connected to trace
#define DMA_BD			24	//BD for chanel DMA_CHAN

/*****************************************************************************/
/**
*
* This is the main entry point for the AIEFAL memtile trace example.
*
* @param	None.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_aiefal_memtile_trace(XAie_DevInst *DevInst)
{
	XAie_LocType Loc = XAie_TileLoc(1, XAIE_MEM_TILE_ROW_START);
	XAie_Packet Packet = XAie_PacketInit(1, 1);
	XAie_ModuleType Mod = XAIE_MEM_MOD;
	XAie_Events StartEvent, StopEvent;
	uint8_t PendingBd = 4;
	AieRC RC;

	/* Instantiate FAL object */
	XAieDev Aie(DevInst, false);

	/* Reserve userevents to trigger start events */
	auto UserEvent0 = Aie.tile(Loc).mem().userEvent();
	RC = UserEvent0->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve user event 0" << endl;
		return -1;
	}
	RC = UserEvent0->getEvent(StartEvent);
	if(RC != XAIE_OK) {
		cout << "GetEvent failed for user event 0" << endl;
		return -1;
	}
	RC = UserEvent0->start();
	if(RC != XAIE_OK) {
		cout << "Failed to start user event 0" << endl;
		return -1;
	}

	/* Reserve perfcount to time trace */
	auto PerfCount = Aie.tile(Loc).mem().perfCounter();
	RC = PerfCount->initialize(Mod, StartEvent,
				   Mod, XAIE_EVENT_NONE_MEM_TILE,
				   Mod, XAIE_EVENT_NONE_MEM_TILE);
	if (RC != XAIE_OK) {
		cout << "Failed to initialize perf counter 1" << endl;
		return -1;
	}
	RC = PerfCount->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve perf counter 1" << endl;
		return -1;
	}
	RC = PerfCount->changeThreshold(NUM_TRACE_CYCLES);
	if(RC != XAIE_OK) {
		cout << "Failed to change threshold for perf counter 1" << endl;
		return -1;
	}
	RC = PerfCount->getCounterEvent(Mod, StopEvent);
	if(RC != XAIE_OK) {
		cout << "Failed to get event for perf counter 1" << endl;
		return -1;
	}
	RC = PerfCount->changeStopEvent(Mod, StopEvent);
	if(RC != XAIE_OK) {
		cout << "Failed to change stop event for perf counter 1" << endl;
		return -1;
	}
	RC = PerfCount->start();
	if(RC != XAIE_OK) {
		cout << "Failed to start perf counter 1" << endl;
		return -1;
	}

	/* Configure StreamSwitch to output trace to DMA*/
	RC = XAie_StrmConnCctEnable(DevInst, Loc, TRACE, 0, DMA, DMA_CHAN);
	if(RC != XAIE_OK) {
		cout << "Failed to enable stream switch" << endl;
		return -1;
	}

	/* Configure S2MM DMA to write to from trace stream to data memory */
	XAie_DmaChannelDesc Trace2DataDmaCh;
	XAie_DmaDesc Trace2DataDma;
	RC = XAie_DmaDescInit(Aie.dev(), &Trace2DataDma, Loc);
	if(RC != XAIE_OK) {
		cout << "Failed to initialize dma descriptor" << endl;
		return -1;
	}
	RC = XAie_DmaChannelDescInit(Aie.dev(), &Trace2DataDmaCh, Loc);
	if(RC != XAIE_OK) {
		cout << "Failed to initialize dma channel descriptor" << endl;
		return -1;
	}
	RC = XAie_DmaSetAddrLen(&Trace2DataDma, DATA_MEM_ADDR, DATA_MEM_MAX);
	if(RC != XAIE_OK) {
		cout << "Failed to set addr len" << endl;
		return -1;
	}
	RC = XAie_DmaEnableBd(&Trace2DataDma);
	if(RC != XAIE_OK) {
		cout << "Failed to enable bd" << endl;
		return -1;
	}
	RC = XAie_DmaWriteBd(Aie.dev(), &Trace2DataDma, Loc, DMA_BD);
	if(RC != XAIE_OK) {
		cout << "Failed to write bd" << endl;
		return -1;
	}
	/* Set Finish on Tlast as trace stream length is unknown */
	RC = XAie_DmaChannelSetFoTMode(&Trace2DataDmaCh,
				       DMA_FoT_COUNTS_FROM_MM_REG);
	if(RC != XAIE_OK) {
		cout << "Failed to set FoT Mode" << endl;
		return -1;
	}
	RC = XAie_DmaWriteChannel(Aie.dev(), &Trace2DataDmaCh, Loc, DMA_CHAN,
				  DMA_S2MM);
	if(RC != XAIE_OK) {
		cout << "Failed to write channel" << endl;
		return -1;
	}
	RC = XAie_DmaChannelPushBdToQueue(Aie.dev(), Loc, DMA_CHAN,
					  DMA_S2MM, DMA_BD);
	if(RC != XAIE_OK) {
		cout << "Failed to push bd to queue" << endl;
		return -1;
	}

	XAieTracing Trace(Aie, Loc, XAIE_MEM_MOD);
	RC = Trace.setMode(XAIE_TRACE_EVENT_TIME);
	if(RC != XAIE_OK) {
		cout << "Failed to set mode" << endl;
		return -1;
	}
	RC = Trace.setPkt(Packet);
	if(RC != XAIE_OK) {
		cout << "Failed to set packet" << endl;
		return -1;
	}
	RC = Trace.addEvent(Mod, XAIE_EVENT_TRUE_MEM_TILE);
	if(RC != XAIE_OK) {
		cout << "Failed to add true event to trace" << endl;
		return -1;
	}
	RC = Trace.setCntrEvent(StartEvent, StopEvent);
	if(RC != XAIE_OK) {
		cout << "Failed to set packet" << endl;
		return -1;
	}
	RC = Trace.reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve trace" << endl;
		return -1;
	}
	RC = Trace.start();
	if(RC != XAIE_OK) {
		cout << "Failed to start trace" << endl;
		return -1;
	}

	/* Generate start event for trace and perfcounter */
	RC = XAie_EventGenerate(Aie.dev(), Loc, Mod, StartEvent);
	if(RC != XAIE_OK) {
		cout << "Failed to generate start event" << endl;
		return -1;
	}

	while(PendingBd != 0) {
		XAie_DmaGetPendingBdCount(Aie.dev(), Loc, DMA_CHAN, DMA_S2MM,
					  &PendingBd);
	}

	/* Check how many words written by DMA to see if trace packet
	 * was sent
	 */
	uint32_t DmaWriteCount = 0;
	XAie_Read32(Aie.dev(),
		    _XAie_GetTileAddr(Aie.dev(), Loc.Row, Loc.Col) +
		    0x000A06C4, &DmaWriteCount);

	if(DmaWriteCount != 8) {
		cout << "Trace packet not sent" << endl;
		return -1;
	}

	/* Release trace */
	RC = Trace.stop();
	if(RC != XAIE_OK) {
		cout << "Failed to stop trace" << endl;
		return -1;
	}
	RC = Trace.release();
	if(RC != XAIE_OK) {
		cout << "Failed to release trace" << endl;
		return -1;
	}

	cout << "AIEFAL Memtile Trace success." << endl;
	return 0;
}

/*****************************************************************************/
/**
*
* This is the main entry point for the AIEFAL shimtile trace example.
*
* @param	None.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_aiefal_shimtile_trace(XAie_DevInst *DevInst)
{
	XAie_LocType MemTile = XAie_TileLoc(0, XAIE_MEM_TILE_ROW_START);
	XAie_LocType WestLoc = XAie_TileLoc(0, XAIE_SHIM_ROW);
	XAie_LocType Loc = XAie_TileLoc(1, XAIE_SHIM_ROW);
	XAie_Packet Packet = XAie_PacketInit(1, 1);
	XAie_ModuleType Mod = XAIE_PL_MOD;
	XAie_Events StartEvent, StopEvent;
	uint8_t PendingBd = 4;
	AieRC RC;

	/* Instantiate FAL object */
	XAieDev Aie(DevInst, false);

	/* Reserve userevents to trigger start events */
	auto UserEvent0 = Aie.tile(Loc).pl().userEvent();
	RC = UserEvent0->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve user event 0" << endl;
		return -1;
	}
	RC = UserEvent0->getEvent(StartEvent);
	if(RC != XAIE_OK) {
		cout << "GetEvent failed for user event 0" << endl;
		return -1;
	}
	RC = UserEvent0->start();
	if(RC != XAIE_OK) {
		cout << "Failed to start user event 0" << endl;
		return -1;
	}

	/* Reserve perfcount to time trace */
	auto PerfCount = Aie.tile(Loc).pl().perfCounter();
	RC = PerfCount->initialize(Mod, StartEvent,
				   Mod, XAIE_EVENT_NONE_PL,
				   Mod, XAIE_EVENT_NONE_PL);
	if (RC != XAIE_OK) {
		cout << "Failed to initialize perf counter 1" << endl;
		return -1;
	}
	RC = PerfCount->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve perf counter 1" << endl;
		return -1;
	}
	RC = PerfCount->changeThreshold(NUM_TRACE_CYCLES);
	if(RC != XAIE_OK) {
		cout << "Failed to change threshold for perf counter 1" << endl;
		return -1;
	}
	RC = PerfCount->getCounterEvent(Mod, StopEvent);
	if(RC != XAIE_OK) {
		cout << "Failed to get event for perf counter 1" << endl;
		return -1;
	}
	RC = PerfCount->changeStopEvent(Mod, StopEvent);
	if(RC != XAIE_OK) {
		cout << "Failed to change stop event for perf counter 1" << endl;
		return -1;
	}
	RC = PerfCount->start();
	if(RC != XAIE_OK) {
		cout << "Failed to start perf counter 1" << endl;
		return -1;
	}

	/* Configure StreamSwitch to output trace to MemTile */
	RC = XAie_StrmConnCctEnable(DevInst, Loc, TRACE, 0U, WEST, 0U);
	if(RC != XAIE_OK) {
		cout << "Failed to enable stream switch" << endl;
		return -1;
	}
	RC = XAie_StrmConnCctEnable(DevInst, WestLoc, EAST, 0U, NORTH, 0U);
	if(RC != XAIE_OK) {
		cout << "Failed to enable stream switch" << endl;
		return -1;
	}
	RC = XAie_StrmConnCctEnable(DevInst, MemTile, SOUTH, 0U, DMA, 0U);
	if(RC != XAIE_OK) {
		cout << "Failed to enable stream switch" << endl;
		return -1;
	}

	/* Configure S2MM DMA to write to from trace stream to data memory */
	XAie_DmaChannelDesc Trace2DataDmaCh;
	XAie_DmaDesc Trace2DataDma;
	RC = XAie_DmaDescInit(Aie.dev(), &Trace2DataDma, MemTile);
	if(RC != XAIE_OK) {
		cout << "Failed to initialize dma descriptor" << endl;
		return -1;
	}
	RC = XAie_DmaChannelDescInit(Aie.dev(), &Trace2DataDmaCh, MemTile);
	if(RC != XAIE_OK) {
		cout << "Failed to initialize dma channel descriptor" << endl;
		return -1;
	}
	RC = XAie_DmaSetAddrLen(&Trace2DataDma, DATA_MEM_ADDR, DATA_MEM_MAX);
	if(RC != XAIE_OK) {
		cout << "Failed to set addr len" << endl;
		return -1;
	}
	RC = XAie_DmaEnableBd(&Trace2DataDma);
	if(RC != XAIE_OK) {
		cout << "Failed to enable bd" << endl;
		return -1;
	}
	RC = XAie_DmaWriteBd(Aie.dev(), &Trace2DataDma, MemTile, 0U);
	if(RC != XAIE_OK) {
		cout << "Failed to write bd" << endl;
		return -1;
	}
	/* Set Finish on Tlast as trace stream length is unknown */
	RC = XAie_DmaChannelSetFoTMode(&Trace2DataDmaCh,
				       DMA_FoT_COUNTS_FROM_MM_REG);
	if(RC != XAIE_OK) {
		cout << "Failed to set FoT Mode" << endl;
		return -1;
	}
	RC = XAie_DmaWriteChannel(Aie.dev(), &Trace2DataDmaCh, MemTile, 0U, DMA_S2MM);
	if(RC != XAIE_OK) {
		cout << "Failed to write channel" << endl;
		return -1;
	}
	RC = XAie_DmaChannelPushBdToQueue(Aie.dev(), MemTile, 0U, DMA_S2MM, 0U);
	if(RC != XAIE_OK) {
		cout << "Failed to push bd to queue" << endl;
		return -1;
	}

	XAieTracing Trace(Aie, Loc, XAIE_PL_MOD);
	RC = Trace.setMode(XAIE_TRACE_EVENT_TIME);
	if(RC != XAIE_OK) {
		cout << "Failed to set mode" << endl;
		return -1;
	}
	RC = Trace.setPkt(Packet);
	if(RC != XAIE_OK) {
		cout << "Failed to set packet" << endl;
		return -1;
	}
	RC = Trace.addEvent(Mod, XAIE_EVENT_TRUE_PL);
	if(RC != XAIE_OK) {
		cout << "Failed to add true event to trace" << endl;
		return -1;
	}
	RC = Trace.setCntrEvent(StartEvent, StopEvent);
	if(RC != XAIE_OK) {
		cout << "Failed to set packet" << endl;
		return -1;
	}
	RC = Trace.reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve trace" << endl;
		return -1;
	}
	RC = Trace.start();
	if(RC != XAIE_OK) {
		cout << "Failed to start trace" << endl;
		return -1;
	}

	/* Generate start event for trace and perfcounter */
	RC = XAie_EventGenerate(Aie.dev(), Loc, Mod, StartEvent);
	if(RC != XAIE_OK) {
		cout << "Failed to generate start event" << endl;
		return -1;
	}

	while(PendingBd != 0) {
		XAie_DmaGetPendingBdCount(Aie.dev(), MemTile, 0U, DMA_S2MM,
					  &PendingBd);
	}

	/* Check how many words written by DMA to see if trace packet
	 * was sent
	 */
	uint32_t DmaWriteCount = 0;
	XAie_Read32(Aie.dev(),
		    _XAie_GetTileAddr(Aie.dev(), MemTile.Row, MemTile.Col) +
		    0x000A06B0, &DmaWriteCount);

	if(DmaWriteCount != 8) {
		cout << "Trace packet not sent" << endl;
		return -1;
	}

	/* Release trace */
	RC = Trace.stop();
	if(RC != XAIE_OK) {
		cout << "Failed to stop trace" << endl;
		return -1;
	}
	RC = Trace.release();
	if(RC != XAIE_OK) {
		cout << "Failed to release trace" << endl;
		return -1;
	}

	cout << "AIEFAL Shimtile Trace success." << endl;
	return 0;
}
/** @} */
