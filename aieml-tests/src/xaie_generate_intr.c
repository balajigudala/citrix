/******************************************************************************
* Copyright (C) 2019 - 2020 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

/*****************************************************************************/
/**
* @file xaie_data_memory.c
* @{
*
* This file contains the test application of data memory check.
*
* This application write random integers to first 1024 bytes of data memory of
* all valid tiles and reads the data back to check the basic functioning of
* data memory.
*
******************************************************************************/

/***************************** Include Files *********************************/
#include <stdlib.h>
#include <xaiengine.h>

#include "hw_config.h"

const XAie_Events NoCTileErrors[] = {
	XAIE_EVENT_AXI_MM_SLAVE_TILE_ERROR_PL,
	XAIE_EVENT_CONTROL_PKT_ERROR_PL,
	XAIE_EVENT_AXI_MM_DECODE_NSU_ERROR_PL,
	XAIE_EVENT_AXI_MM_SLAVE_NSU_ERROR_PL,
	XAIE_EVENT_AXI_MM_UNSUPPORTED_TRAFFIC_PL,
	XAIE_EVENT_AXI_MM_UNSECURE_ACCESS_IN_SECURE_MODE_PL,
	XAIE_EVENT_AXI_MM_BYTE_STROBE_ERROR_PL,
	XAIE_EVENT_DMA_S2MM_0_ERROR_PL,
	XAIE_EVENT_DMA_S2MM_1_ERROR_PL,
	XAIE_EVENT_DMA_MM2S_0_ERROR_PL,
	XAIE_EVENT_DMA_MM2S_1_ERROR_PL,
};

const XAie_Events PLTileErrors[] = {
	XAIE_EVENT_AXI_MM_SLAVE_TILE_ERROR_PL,
	XAIE_EVENT_CONTROL_PKT_ERROR_PL,
};

const XAie_Events CoreModErrors[] = {
	#if  HW_GEN == 1
	XAIE_EVENT_TLAST_IN_WSS_WORDS_0_2_CORE,
	#endif
	XAIE_EVENT_PM_REG_ACCESS_FAILURE_CORE,
	XAIE_EVENT_STREAM_PKT_PARITY_ERROR_CORE,
	XAIE_EVENT_CONTROL_PKT_ERROR_CORE,
	XAIE_EVENT_AXI_MM_SLAVE_ERROR_CORE,
	XAIE_EVENT_INSTR_DECOMPRSN_ERROR_CORE,
	XAIE_EVENT_DM_ADDRESS_OUT_OF_RANGE_CORE,
	XAIE_EVENT_PM_ECC_ERROR_SCRUB_2BIT_CORE,
	XAIE_EVENT_PM_ECC_ERROR_2BIT_CORE,
	XAIE_EVENT_PM_ADDRESS_OUT_OF_RANGE_CORE,
	XAIE_EVENT_DM_ACCESS_TO_UNAVAILABLE_CORE,
	XAIE_EVENT_LOCK_ACCESS_TO_UNAVAILABLE_CORE,
};

const XAie_Events MemModErrors[] = {
	XAIE_EVENT_DM_ECC_ERROR_SCRUB_2BIT_MEM,
	XAIE_EVENT_DM_ECC_ERROR_2BIT_MEM,
	XAIE_EVENT_DM_PARITY_ERROR_BANK_2_MEM,
	XAIE_EVENT_DM_PARITY_ERROR_BANK_3_MEM,
	XAIE_EVENT_DM_PARITY_ERROR_BANK_4_MEM,
	XAIE_EVENT_DM_PARITY_ERROR_BANK_5_MEM,
	XAIE_EVENT_DM_PARITY_ERROR_BANK_6_MEM,
	XAIE_EVENT_DM_PARITY_ERROR_BANK_7_MEM,
	XAIE_EVENT_DMA_S2MM_0_ERROR_MEM,
	XAIE_EVENT_DMA_S2MM_1_ERROR_MEM,
	XAIE_EVENT_DMA_MM2S_0_ERROR_MEM,
	XAIE_EVENT_DMA_MM2S_1_ERROR_MEM,
};

void generate_shim_tile_event(XAie_DevInst *DevInst, XAie_LocType Loc)
{
	u8 TileType;
	int i;

	TileType = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
	if (TileType != XAIEGBL_TILE_TYPE_SHIMNOC &&
	    TileType != XAIEGBL_TILE_TYPE_SHIMPL) {
		printf("Tile [%d, %d] not shim tile.\n", Loc.Col, Loc.Row);
		return;
	}

	for (i = XAIE_EVENT_GROUP_ERRORS_PL + 1; i < XAIE_EVENT_GROUP_STREAM_SWITCH_PL; i++) {
		printf("[%d, %d]: Generating shim tile event: %d\n", Loc.Col, Loc.Row, i);
		XAie_EventGenerate(DevInst, Loc, XAIE_PL_MOD, i);
	}
}

void generate_aie_tile_core_event(XAie_DevInst *DevInst, XAie_LocType Loc)
{
	u8 TileType;
	int i;

	TileType = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
	if (TileType != XAIEGBL_TILE_TYPE_AIETILE) {
		printf("Tile [%d, %d] not aie tile.\n", Loc.Col, Loc.Row);
		return;
	}

	for (i = XAIE_EVENT_GROUP_ERRORS_1_CORE + 1; i < XAIE_EVENT_GROUP_STREAM_SWITCH_CORE; i++) {
		printf("[%d, %d]: Generating aie tile mem event: %d\n", Loc.Col, Loc.Row, i);
		XAie_EventGenerate(DevInst, Loc, XAIE_CORE_MOD, i);
	}
}

void generate_aie_tile_mem_event(XAie_DevInst *DevInst, XAie_LocType Loc)
{
	u8 TileType;
	int i;

	TileType = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
	if (TileType != XAIEGBL_TILE_TYPE_AIETILE) {
		printf("Tile [%d, %d] not aie tile.\n", Loc.Col, Loc.Row);
		return;
	}

	for (i = XAIE_EVENT_GROUP_ERRORS_MEM + 1; i < XAIE_EVENT_GROUP_BROADCAST_MEM; i++) {
		printf("[%d, %d]: Generating aie tile mem event: %d\n", Loc.Col, Loc.Row, i);
		XAie_EventGenerate(DevInst, Loc, XAIE_MEM_MOD, i);
	}
}

void generate_mem_tile_event(XAie_DevInst *DevInst, XAie_LocType Loc)
{
	u8 TileType;
	int i;

	TileType = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
	if (TileType != XAIEGBL_TILE_TYPE_MEMTILE) {
		printf("Tile [%d, %d] not memtile.\n", Loc.Col, Loc.Row);
		return;
	}

	for (i = XAIE_EVENT_GROUP_ERRORS_MEM_TILE + 1; i < XAIE_EVENT_GROUP_BROADCAST_MEM_TILE; i++) {
		printf("[%d, %d]: Generating mem tile event: %d\n", Loc.Col, Loc.Row, i);
		XAie_EventGenerate(DevInst, Loc, XAIE_MEM_MOD, i);
	}
}

void generate_events_on_col(XAie_DevInst *DevInst, int col)
{
	u8 TileType;
	XAie_LocType Loc;


	for (Loc.Row = 0, Loc.Col = col; Loc.Row < DevInst->NumRows; Loc.Row++) {
		TileType = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
		switch (TileType) {
		case XAIEGBL_TILE_TYPE_SHIMNOC:
		case XAIEGBL_TILE_TYPE_SHIMPL:
			generate_shim_tile_event(DevInst, Loc);
			break;
		case XAIEGBL_TILE_TYPE_MEMTILE:
			generate_mem_tile_event(DevInst, Loc);
			break;
		case XAIEGBL_TILE_TYPE_AIETILE:
			generate_aie_tile_core_event(DevInst, Loc);
			generate_aie_tile_mem_event(DevInst, Loc);
			break;
		default:
			printf("[%d, %d]: Tiletype invalid\n", Loc.Col, Loc.Row);
		}
	}
}

int test_aie_generate_intr(XAie_DevInst *DevInst)
{
	AieRC RC = XAIE_OK;
	int i;

	printf("Starting aie generate intr test\n");

	RC = XAie_PmRequestTiles(DevInst, NULL, 0);
	if (RC != XAIE_OK) {
		printf("XAie_PmRequestTiles failed: %d", RC);
		//continue
		RC = XAIE_OK;
	}
	printf("Load your CDO now.");
	getchar();
	getchar();

	/* Should already be done by CDO. Uncomment if no CDO*/
//	RC = XAie_ErrorHandlingInit(DevInst);
	if (RC != XAIE_OK) {
		printf("Error handling init failed: %d\n", RC);
		return -1;
	}

	for (i = 0; i < DevInst->NumCols; i++)
		generate_events_on_col(DevInst, i);

	printf("End aie generate intr test\n");
	return 0;
}
