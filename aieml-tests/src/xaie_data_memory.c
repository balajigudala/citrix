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

/************************** Constant Definitions *****************************/
#define AIE_DATA_MEM_CHECK_SIZE 1024

/************************** Function Definitions *****************************/
static int memory_check(XAie_DevInst *DevInst, XAie_LocType Loc, size_t MemSize)
{
	AieRC RC;
	uint32_t Data, RdData;

	for(uint32_t Addr = 0; Addr < MemSize; Addr += 4) {
		Data = rand() % 127;
		RC = XAie_DataMemWrWord(DevInst, Loc, Addr, Data);
		if(RC != XAIE_OK)
			return -1;

		RC = XAie_DataMemRdWord(DevInst, Loc, Addr, &RdData);
		if(RC != XAIE_OK)
			return -1;

		if(Data != RdData){
			printf("Data mismatch at Tile (%d, %d) for address: "\
					"0x%X\n", Loc.Col, Loc.Row, Addr);
			return -1;
		}
	}

	return 0;
}

int test_aie_data_mem(XAie_DevInst *DevInst)
{
	AieRC RC = XAIE_OK;
	int Ret = 0;
	int Flag = 0;

	for(int c = 0; c < XAIE_NUM_COLS; c++) {
		for(int r = 1; r < XAIE_NUM_ROWS; r++) {

			Ret = memory_check(DevInst, XAie_TileLoc(c, r),
					AIE_DATA_MEM_CHECK_SIZE);
			if(Ret)
				Flag = 1;
		}
	}

	if(Flag) {
		printf("AIE Memory check failed.\n");
		return -1;
	}

	printf("AIE Memory check success.\n");
	return 0;
}
