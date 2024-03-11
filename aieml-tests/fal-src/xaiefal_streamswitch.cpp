/******************************************************************************
* Copyright (C) 2022 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/


/*****************************************************************************/
/**
* @file xaiefal_userevent.cpp
* @{
*
* This file contains the test application of stream switch events for aieml.
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
#define NUM_ELEMS 64

/*****************************************************************************/
/**
*
* This is the main entry point for the AIEFAL memtile streamswitch event
* example.
*
* @param	None.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_aiefal_memtile_streamswitch(XAie_DevInst *DevInst)
{
	XAie_LocType NorthTile = XAie_TileLoc(1, XAIE_MEM_TILE_ROW_START+1);
	XAie_LocType Loc = XAie_TileLoc(1, XAIE_MEM_TILE_ROW_START);
	XAie_DmaDesc NorthTile_S2MM, Tile_MM2S;
	XAie_Events IdleEvent, RunningEvent;
	XAie_ModuleType Mod = XAIE_MEM_MOD;
	uint8_t Status, PendingBd = 4;
	uint32_t data[NUM_ELEMS];
	AieRC RC;

	/* Instantiate FAL object */
	XAieDev Aie(DevInst, false);

	/* Grab stream switch event object */
	auto Stream = Aie.tile(Loc).sswitchPort();
	RC = Stream->setPortToSelect(XAIE_STRMSW_SLAVE, DMA, 0);
	if(RC != XAIE_OK) {
		cout << "Failed to set stream switch event" << endl;
		return -1;
	}

	RC = Stream->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve stream switch event" << endl;
		return -1;
	}

	RC = Stream->start();
	if(RC != XAIE_OK) {
		cout << "Failed to start stream switch event" << endl;
		return -1;
	}

	RC = Stream->getSSIdleEvent(IdleEvent);
	if(RC != XAIE_OK) {
		cout << "Failed to get Idle event" << endl;
		return -1;
	}

	RC = Stream->getSSRunningEvent(RunningEvent);
	if(RC != XAIE_OK) {
		cout << "Failed to get Running event" << endl;
		return -1;
	}

	/* Init data */
	for(uint32_t i = 0; i < NUM_ELEMS; i++)
		data[i] = rand();

	/* Configure StreamSwitch to output dma to north stream*/
	RC = XAie_StrmConnCctEnable(DevInst, Loc, DMA, 0, NORTH, 0);
	if(RC != XAIE_OK) {
		cout << "Failed to enable stream switch" << endl;
		return -1;
	}
	RC = XAie_StrmConnCctEnable(DevInst, NorthTile, SOUTH, 0, DMA, 0);
	if(RC != XAIE_OK) {
		cout << "Failed to enable stream switch" << endl;
		return -1;
	}

	/* Write data in data memory */
	RC = XAie_DataMemBlockWrite(Aie.dev(), Loc, 0x0, data,
			NUM_ELEMS * sizeof(uint32_t));
	if(RC != XAIE_OK) {
		cout << "Failed to write to data memory" << endl;
		return -1;
	}

	/* Setup DMA transaction to north stream */
	RC = XAie_DmaDescInit(Aie.dev(), &Tile_MM2S, Loc);
	if(RC != XAIE_OK) {
		cout << "Failed to init descriptor" << endl;
		return -1;
	}
	RC = XAie_DmaSetAddrLen(&Tile_MM2S, 0x80000, NUM_ELEMS*sizeof(uint32_t));
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
	RC = XAie_DmaDescInit(Aie.dev(), &NorthTile_S2MM, NorthTile);
	if(RC != XAIE_OK) {
		cout << "Failed to init descriptor" << endl;
		return -1;
	}
	RC = XAie_DmaSetAddrLen(&NorthTile_S2MM, 0x80000, NUM_ELEMS*sizeof(uint32_t));
	if(RC != XAIE_OK) {
		cout << "Failed to set addr len" << endl;
		return -1;
	}
	RC = XAie_DmaEnableBd(&NorthTile_S2MM);
	if(RC != XAIE_OK) {
		cout << "Failed to enable bd" << endl;
		return -1;
	}
	RC = XAie_DmaWriteBd(Aie.dev(), &NorthTile_S2MM, NorthTile, 0);
	if(RC != XAIE_OK) {
		cout << "Failed to write bd" << endl;
		return -1;
	}

	/* Check for Idle event */
	Status = 0;
	RC = XAie_EventReadStatus(Aie.dev(), Loc, Mod, IdleEvent, &Status);
	if(!Status) {
		cout << "Idle Event not triggered" << endl;
		return -1;
	}

	RC = XAie_DmaChannelPushBdToQueue(Aie.dev(), Loc, 0, DMA_MM2S, 0);
	if(RC != XAIE_OK) {
		cout << "Failed to push bd to queue" << endl;
		return -1;
	}

	RC = XAie_DmaChannelPushBdToQueue(Aie.dev(), NorthTile, 0, DMA_S2MM, 0);
	if(RC != XAIE_OK) {
		cout << "Failed to push bd to queue" << endl;
		return -1;
	}

	while(PendingBd != 0) {
		XAie_DmaGetPendingBdCount(Aie.dev(), Loc, 0, DMA_MM2S, &PendingBd);
	}

	/* Check for Running event */
	Status = 0;
	RC = XAie_EventReadStatus(Aie.dev(), Loc, Mod, RunningEvent, &Status);
	if(!Status) {
		cout << "Running Event not triggered" << endl;
		return -1;
	}

	cout << "AIEFAL Memtile Stream Switch Event success." << endl;
	return 0;
}

/*****************************************************************************/
/**
*
* This is the main entry point for the AIEFAL shimtile streamswitch event
* example.
*
* @param	None.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_aiefal_shimtile_streamswitch(XAie_DevInst *DevInst)
{
	XAie_LocType NorthTile = XAie_TileLoc(1, XAIE_SHIM_ROW+1);
	XAie_LocType Loc = XAie_TileLoc(1, XAIE_SHIM_ROW);
	XAie_Events IdleEvent, RunningEvent;
	XAie_ModuleType Mod = XAIE_PL_MOD;
	uint8_t Status, PendingBd = 4;
	XAie_DmaDesc NorthTile_MM2S;
	uint32_t data[NUM_ELEMS];
	AieRC RC;

	/* Instantiate FAL object */
	XAieDev Aie(DevInst, false);

	/* Grab stream switch event object */
	auto Stream = Aie.tile(Loc).sswitchPort();
	RC = Stream->setPortToSelect(XAIE_STRMSW_SLAVE, NORTH, 0);
	if(RC != XAIE_OK) {
		cout << "Failed to set stream switch event" << endl;
		return -1;
	}

	RC = Stream->reserve();
	if(RC != XAIE_OK) {
		cout << "Failed to reserve stream switch event" << endl;
		return -1;
	}

	RC = Stream->start();
	if(RC != XAIE_OK) {
		cout << "Failed to start stream switch event" << endl;
		return -1;
	}

	RC = Stream->getSSIdleEvent(IdleEvent);
	if(RC != XAIE_OK) {
		cout << "Failed to get Idle event" << endl;
		return -1;
	}

	RC = Stream->getSSRunningEvent(RunningEvent);
	if(RC != XAIE_OK) {
		cout << "Failed to get Running event" << endl;
		return -1;
	}

	/* Init data */
	for(uint32_t i = 0; i < NUM_ELEMS; i++)
		data[i] = rand();

	/* Configure StreamSwitch to loopback data from dma */
	RC = XAie_StrmConnCctEnable(DevInst, Loc, NORTH, 0, NORTH, 0);
	if(RC != XAIE_OK) {
		cout << "Failed to enable stream switch" << endl;
		return -1;
	}
	RC = XAie_StrmConnCctEnable(DevInst, NorthTile, DMA, 0, SOUTH, 0);
	if(RC != XAIE_OK) {
		cout << "Failed to enable stream switch" << endl;
		return -1;
	}
	RC = XAie_StrmConnCctEnable(DevInst, NorthTile, SOUTH, 0, DMA, 0);
	if(RC != XAIE_OK) {
		cout << "Failed to enable stream switch" << endl;
		return -1;
	}

	/* Write data in data memory */
	RC = XAie_DataMemBlockWrite(Aie.dev(), NorthTile, 0x0, data,
			NUM_ELEMS * sizeof(uint32_t));
	if(RC != XAIE_OK) {
		cout << "Failed to write to data memory" << endl;
		return -1;
	}

	/* Setup DMA transaction to north stream */
	RC = XAie_DmaDescInit(Aie.dev(), &NorthTile_MM2S, NorthTile);
	if(RC != XAIE_OK) {
		cout << "Failed to init descriptor" << endl;
		return -1;
	}
	RC = XAie_DmaSetAddrLen(&NorthTile_MM2S, 0x80000, NUM_ELEMS*sizeof(uint32_t));
	if(RC != XAIE_OK) {
		cout << "Failed to set addr len" << endl;
		return -1;
	}
	RC = XAie_DmaEnableBd(&NorthTile_MM2S);
	if(RC != XAIE_OK) {
		cout << "Failed to enable bd" << endl;
		return -1;
	}
	RC = XAie_DmaWriteBd(Aie.dev(), &NorthTile_MM2S, NorthTile, 0);
	if(RC != XAIE_OK) {
		cout << "Failed to write bd" << endl;
		return -1;
	}

	/* Check for Idle event */
	Status = 0;
	RC = XAie_EventReadStatus(Aie.dev(), Loc, Mod, IdleEvent, &Status);
	if(!Status) {
		cout << "Idle Event not triggered" << endl;
		return -1;
	}

	RC = XAie_DmaChannelPushBdToQueue(Aie.dev(), NorthTile, 0, DMA_MM2S, 0);
	if(RC != XAIE_OK) {
		cout << "Failed to push bd to queue" << endl;
		return -1;
	}
	RC = XAie_DmaChannelPushBdToQueue(Aie.dev(), NorthTile, 0, DMA_S2MM, 0);
	if(RC != XAIE_OK) {
		cout << "Failed to push bd to queue" << endl;
		return -1;
	}

	while(PendingBd != 0) {
		XAie_DmaGetPendingBdCount(Aie.dev(), NorthTile, 0, DMA_S2MM, &PendingBd);
	}

	/* Check for Running event */
	Status = 0;
	RC = XAie_EventReadStatus(Aie.dev(), Loc, Mod, RunningEvent, &Status);
	if(!Status) {
		cout << "Running Event not triggered" << endl;
		return -1;
	}

	cout << "AIEFAL Shimtile Stream Switch Event success." << endl;
	return 0;
}
/** @} */
