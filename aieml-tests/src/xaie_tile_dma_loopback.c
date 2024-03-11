/******************************************************************************
* Copyright (C) 2020 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/


/*****************************************************************************/
/**
* @file xaie_dma_loopback.c
* @{
*
* This file contains the test application of dma loopback for aie.
*
* This application writes random integer values to AIE data memory at location
* DATA_MEM_INPUT_ADDR on Tile_1. The DMA(MM2S channel) on Tile_1 is configured
* to read data from DATA_MEM_INPUT_ADDR and push it to stream switch DMA port.
* This data is received by stream switch on Tile_2. The DMA(S2MM channel) on
* Tile_2 configured to push data from stream switch to data memory
* buffer(DATA_MEM_INPUT_ADDR).
* Similarly, MM2S channel on Tile_2 is configured to fetch data from
* DATA_MEM_INPUT_ADDR and push it to stream switch. S2MM channel on Tile_1
* is configured to fetch this data and write to DATA_MEM_OUTPUT_ADDR in the
* memory module. Hardware locks are used by DMA channels for synchronization.
* Data from DATA_MEM_OUTPUT_ADDR on Tile_1 is read over AXIM-MM to check for
* correctness.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who     Date     Changes
* ----- ------  -------- -----------------------------------------------------
* 1.0   Tejus   16/12/2020  Initial creation
* </pre>
*
******************************************************************************/

/***************************** Include Files *********************************/
#include <stdlib.h>
#include <xaiengine.h>
#include "hw_config.h"

/************************** Constant Definitions *****************************/
/* Lock values used for synchronization */
#define LOCK_FOR_WRITE	0
#define LOCK_FOR_READ	1

/* Input and output address in aie data memory */
#define DATA_MEM_INPUT_ADDR	0x4000
#define DATA_MEM_OUTPUT_ADDR	0x3000

#define NUM_ELEMS 32

static uint32_t data[NUM_ELEMS];
static uint32_t buffer[NUM_ELEMS];
/*****************************************************************************/
/**
*
* This is the main entry point for the AIE driver Tile DMA loopback example.
*
* @param	None.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_aie_dma(XAie_DevInst *DevInst)
{
	AieRC RC = XAIE_OK;
	uint8_t PendingBd = 5;
	XAie_LocType Tile_1, Tile_2;
	XAie_DmaDesc Tile_1_MM2S, Tile_1_S2MM, Tile_2_MM2S, Tile_2_S2MM;
  int t2_row = 4;
	//add the check to make sure the tile2 row number not exeed the HW row range
	Tile_1 = XAie_TileLoc(1, XAIE_AIE_TILE_ROW_START);
	Tile_2 = XAie_TileLoc(1, XAIE_AIE_TILE_ROW_START + 1);

	/* Initialize array with random integers */
	for(uint8_t i = 0U; i < NUM_ELEMS; i++) {
		data[i] = rand() % 127;
	}

	/* Write data to aie tile data memory */
	RC = XAie_DataMemBlockWrite(DevInst, Tile_1, DATA_MEM_INPUT_ADDR,
			(void *)data, sizeof(uint32_t) * NUM_ELEMS);
	if(RC != XAIE_OK) {
		printf("Writing data to aie data memory failed.\n");
		return -1;
	}

	/* Configure stream switch ports to move data from Tile_1 to Tile_2 */
	RC |= XAie_StrmConnCctEnable(DevInst, Tile_1, DMA, 0, NORTH, 0);
	RC |= XAie_StrmConnCctEnable(DevInst, Tile_2, SOUTH, 0, DMA, 0);

	/* Configure stream switch ports to move data from Tile_2 to Tile_1 */
	RC |= XAie_StrmConnCctEnable(DevInst, Tile_2, DMA, 0, SOUTH, 0);
	RC |= XAie_StrmConnCctEnable(DevInst, Tile_1, NORTH, 0, DMA, 0);

	if(RC != XAIE_OK) {
		printf("Failed to configure stream switches.\n");
		return -1;
	}

	/* Initialize software descriptors for aie dma */
	RC |= XAie_DmaDescInit(DevInst, &Tile_1_MM2S, Tile_1);
	RC |= XAie_DmaDescInit(DevInst, &Tile_1_S2MM, Tile_1);
	RC |= XAie_DmaDescInit(DevInst, &Tile_2_MM2S, Tile_2);
	RC |= XAie_DmaDescInit(DevInst, &Tile_2_S2MM, Tile_2);

	/* Configure address and length in dma software descriptors */
	RC |= XAie_DmaSetAddrLen(&Tile_1_MM2S, DATA_MEM_INPUT_ADDR,
			NUM_ELEMS * sizeof(uint32_t));
	RC |= XAie_DmaSetAddrLen(&Tile_1_S2MM, DATA_MEM_OUTPUT_ADDR,
			NUM_ELEMS * sizeof(uint32_t));
	RC |= XAie_DmaSetAddrLen(&Tile_2_MM2S, DATA_MEM_INPUT_ADDR,
			NUM_ELEMS * sizeof(uint32_t));
	RC |= XAie_DmaSetAddrLen(&Tile_2_S2MM, DATA_MEM_INPUT_ADDR,
			NUM_ELEMS * sizeof(uint32_t));

	/*
	 * Configure locks for dma channel synchronization. S2MM should run
	 * before MM2S on Tile_2. Lock Id 5 is used for synchronization.
	 */
	RC |= XAie_DmaSetLock(&Tile_2_S2MM, XAie_LockInit(5U, LOCK_FOR_WRITE),
			XAie_LockInit(5, LOCK_FOR_READ));
	RC |= XAie_DmaSetLock(&Tile_2_MM2S, XAie_LockInit(5U, LOCK_FOR_READ),
			XAie_LockInit(5, LOCK_FOR_WRITE));
	/*
	 * Configure locks for dma channel synchronization. MM2S should run
	 * before S2MM on Tile_1. Lock Id 6 is used for synchronization.
	 */
	RC |= XAie_DmaSetLock(&Tile_1_MM2S, XAie_LockInit(6U, LOCK_FOR_WRITE),
			XAie_LockInit(6U, LOCK_FOR_READ));
	RC |= XAie_DmaSetLock(&Tile_1_S2MM, XAie_LockInit(6U, LOCK_FOR_READ),
			XAie_LockInit(6U, LOCK_FOR_WRITE));

	/* Enable the buffer descriptors in software dma descriptors */
	RC |= XAie_DmaEnableBd(&Tile_1_MM2S);
	RC |= XAie_DmaEnableBd(&Tile_1_S2MM);
	RC |= XAie_DmaEnableBd(&Tile_2_MM2S);
	RC |= XAie_DmaEnableBd(&Tile_2_S2MM);
	if(RC != XAIE_OK) {
		printf("Failed to setup software dma descriptors.\n");
		return -1;
	}

	/*
	 * Configure aie dma hardware using software descriptors. Use buffer
	 * descriptor 1 for MM2S and 9 for S2MM on both tiles.
	 */
	RC |= XAie_DmaWriteBd(DevInst, &Tile_1_MM2S, Tile_1, 1U);
	RC |= XAie_DmaWriteBd(DevInst, &Tile_1_S2MM, Tile_1, 9U);
	RC |= XAie_DmaWriteBd(DevInst, &Tile_2_MM2S, Tile_2, 1U);
	RC |= XAie_DmaWriteBd(DevInst, &Tile_2_S2MM, Tile_2, 9U);

	/* Push Bd numbers to aie dma channel queues and enable the channels */
	RC |= XAie_DmaChannelPushBdToQueue(DevInst, Tile_1, 0U, DMA_MM2S, 1U);
	RC |= XAie_DmaChannelPushBdToQueue(DevInst, Tile_1, 0U, DMA_S2MM, 9U);
	RC |= XAie_DmaChannelPushBdToQueue(DevInst, Tile_2, 0U, DMA_MM2S, 1U);
	RC |= XAie_DmaChannelPushBdToQueue(DevInst, Tile_2, 0U, DMA_S2MM, 9U);
	RC |= XAie_DmaChannelEnable(DevInst, Tile_1, 0U, DMA_MM2S);
	RC |= XAie_DmaChannelEnable(DevInst, Tile_1, 0U, DMA_S2MM);
	RC |= XAie_DmaChannelEnable(DevInst, Tile_2, 0U, DMA_MM2S);
	RC |= XAie_DmaChannelEnable(DevInst, Tile_2, 0U, DMA_S2MM);
	if(RC != XAIE_OK) {
		printf("Failed to configure aie dma hardware and start dma "
				"tranactions.\n");
		return -1;
	}

	while(PendingBd != 0)
		RC = XAie_DmaGetPendingBdCount(DevInst, Tile_1, 0, DMA_S2MM,
				&PendingBd);
#if DEVICE == 0
	sleep(1);
#endif
	/*
	 * Read data from aie data memory at DATA_MEM_OUTPUT_ADDR to compare
	 * with input data.
	 */
	RC |= XAie_DataMemBlockRead(DevInst, Tile_1, DATA_MEM_OUTPUT_ADDR,
			(void*)buffer, NUM_ELEMS * sizeof(uint32_t));
	if(RC != XAIE_OK) {
		printf("Failed to read from aie data memory.\n");
		return -1;
	}

	/* Check for correctness */
	for(uint8_t i = 0; i < NUM_ELEMS; i++) {
		if(data[i] != buffer[i]) {
			printf("Data mismatch at index %d.\n", i);
			printf("AIE Tile DMA Loopback failed.\n");
			return -1;
		}
	}

	printf("AIE Tile DMA Loopback success.\n");

	return 0;
}
/** @} */
