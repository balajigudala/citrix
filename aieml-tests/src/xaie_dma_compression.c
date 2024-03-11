/******************************************************************************
* Copyright (C) 2021 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

/*****************************************************************************/
/**
* @file xaie_mem_tile_tests.c
* @{
*
* The files contains the test application for mem tile dma tests.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who     Date     Changes
* ----- ------  -------- -----------------------------------------------------
* 1.0   Tejus   09/22/2021  Initial creation
* </pre>
*
******************************************************************************/
/***************************** Include Files *********************************/
#include <stdlib.h>
#include <xaiengine.h>

#include "hw_config.h"

/************************** Constant Definitions *****************************/
/* Input and output address in aie data memory */
#define DATA_MEM_INPUT_ADDR	0x4000

#define NUM_ELEMS 32

/************************** Function Definitions *****************************/
/*****************************************************************************/
/**
*
* This is the main entry point for the AIE driver Memory tile tests
*
* @param	None.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_mem_tiles_compression(XAie_DevInst *DevInst)
{
	AieRC RC = XAIE_OK;
	uint8_t PendingBd = 5;
	uint32_t data[NUM_ELEMS];
	uint32_t buffer[NUM_ELEMS];

	XAie_LocType Tile_1, Tile_2, Tile_3, Tile_4, AieTile_2, AieTile_3;
	XAie_DmaDesc Tile_3_S2MM, Tile_2_MM2S;

	Tile_1 = XAie_TileLoc(0, XAIE_AIE_TILE_ROW_START - 1);
	Tile_2 = XAie_TileLoc(1, XAIE_AIE_TILE_ROW_START - 1);
	Tile_3 = XAie_TileLoc(2, XAIE_AIE_TILE_ROW_START - 1);
	Tile_4 = XAie_TileLoc(3, XAIE_AIE_TILE_ROW_START - 1);
	AieTile_2 = XAie_TileLoc(1, XAIE_AIE_TILE_ROW_START);
	AieTile_3 = XAie_TileLoc(2, XAIE_AIE_TILE_ROW_START);

	/* Initialize array with random integers */
	for(uint8_t i = 0U; i < NUM_ELEMS; i++) {
		data[i] =  i + 1;
	}

	/* Write data to aie tile data memory */
	RC = XAie_DataMemBlockWrite(DevInst, Tile_1, DATA_MEM_INPUT_ADDR,
			(void *)data, sizeof(uint32_t) * NUM_ELEMS);
	if(RC != XAIE_OK) {
		printf("Writing data to aie data memory failed.\n");
		return -1;
	}

	/* Configure stream switch ports to move data from Tile_2 to Tile_3
	 * Route the data through AIE Tiles Mem Tiles cannot connect to
	 * EAST/WEST
	 */
	RC |= XAie_StrmConnCctEnable(DevInst, Tile_2, DMA, 0, NORTH, 0);
	RC |= XAie_StrmConnCctEnable(DevInst, AieTile_2, SOUTH, 0, EAST, 0);
	RC |= XAie_StrmConnCctEnable(DevInst, AieTile_3, WEST, 0, SOUTH, 0);
	RC |= XAie_StrmConnCctEnable(DevInst, Tile_3, NORTH, 0, DMA, 0);

	/* Configure DMAs */
	RC |= XAie_DmaDescInit(DevInst, &Tile_2_MM2S, Tile_2);
	RC |= XAie_DmaDescInit(DevInst, &Tile_3_S2MM, Tile_3);

	RC |= XAie_DmaEnableCompression(&Tile_2_MM2S);
	RC |= XAie_DmaEnableCompression(&Tile_3_S2MM);

	/* Acessing West tile memory does not need any offset */
	RC |= XAie_DmaSetAddrLen(&Tile_2_MM2S, DATA_MEM_INPUT_ADDR,
			NUM_ELEMS * sizeof(uint32_t));
	/* Accessing East tile memory requires an offset of 0x100000 */
	RC |= XAie_DmaSetAddrLen(&Tile_3_S2MM, 0x100000 + DATA_MEM_INPUT_ADDR,
			NUM_ELEMS * sizeof(uint32_t));
	RC |= XAie_DmaEnableBd(&Tile_2_MM2S);
	RC |= XAie_DmaEnableBd(&Tile_3_S2MM);
	RC |= XAie_DmaWriteBd(DevInst, &Tile_2_MM2S, Tile_2, 0U);
	RC |= XAie_DmaWriteBd(DevInst, &Tile_3_S2MM, Tile_3, 0U);


	XAie_DmaChannelDesc Tile_2_ChDesc;
	XAie_DmaChannelDescInit(DevInst, &Tile_2_ChDesc, Tile_2);
	XAie_DmaChannelEnCompression(&Tile_2_ChDesc, XAIE_ENABLE);
	XAie_DmaWriteChannel(DevInst, &Tile_2_ChDesc, Tile_2, 0, DMA_MM2S);


	XAie_DmaChannelDesc Tile_3_ChDesc;
	XAie_DmaChannelDescInit(DevInst, &Tile_3_ChDesc, Tile_3);
	XAie_DmaChannelEnCompression(&Tile_3_ChDesc, XAIE_ENABLE);
	XAie_DmaWriteChannel(DevInst, &Tile_3_ChDesc, Tile_3, 0, DMA_S2MM);

	RC |= XAie_DmaChannelPushBdToQueue(DevInst, Tile_2, 0U, DMA_MM2S, 0U);
	RC |= XAie_DmaChannelPushBdToQueue(DevInst, Tile_3, 0U, DMA_S2MM, 0U);

	while(PendingBd != 0)
		RC = XAie_DmaGetPendingBdCount(DevInst, Tile_2, 0, DMA_MM2S,
				&PendingBd);
#if DEVICE == 0 // channel status register have bug in systemc, do workground here
	sleep(1);
#endif
	/*
	 * Read data from aie data memory at DATA_MEM_INPUT`_ADDR to compare
	 * with input data.
	 */
	RC |= XAie_DataMemBlockRead(DevInst, Tile_4, DATA_MEM_INPUT_ADDR,
			(void*)buffer, NUM_ELEMS * sizeof(uint32_t));
	if(RC != XAIE_OK) {
		printf("Failed to read from aie data memory.\n");
		return -1;
	}

	/* Check for correctness */
	for(uint8_t i = 0; i < NUM_ELEMS; i++) {
		if(data[i] != buffer[i]) {
			printf("Data mismatch at index %d.\n", i);
			printf("AIE Mem Tile compression/decompression test "
					"failed.\n");
			return -1;
		}
	}

	printf("AIE Mem Tile compression/decompression test success.\n");

	return 0;
}

int test_aie_dma_compression(XAie_DevInst *DevInst)
{
	AieRC RC = XAIE_OK;
	uint32_t Count;
	uint8_t PendingBd = 5;
	uint32_t data[NUM_ELEMS];
	uint32_t buffer[NUM_ELEMS];
	XAie_LocType Tile_1, Tile_2;
	XAie_DmaDesc Tile_1_MM2S, Tile_2_S2MM;


	Tile_1 = XAie_TileLoc(1, XAIE_AIE_TILE_ROW_START);
	Tile_2 = XAie_TileLoc(1, XAIE_AIE_TILE_ROW_START + 1);

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
	if(RC != XAIE_OK) {
		printf("Failed to configure stream switches.\n");
		return -1;
	}

	/* Initialize software descriptors for aie dma */
	RC |= XAie_DmaDescInit(DevInst, &Tile_1_MM2S, Tile_1);
	RC |= XAie_DmaDescInit(DevInst, &Tile_2_S2MM, Tile_2);

	RC |= XAie_DmaEnableCompression(&Tile_1_MM2S);
	RC |= XAie_DmaEnableCompression(&Tile_2_S2MM);

	/* Configure address and length in dma software descriptors */
	RC |= XAie_DmaSetAddrLen(&Tile_1_MM2S, DATA_MEM_INPUT_ADDR,
			NUM_ELEMS * sizeof(uint32_t));
	RC |= XAie_DmaSetAddrLen(&Tile_2_S2MM, DATA_MEM_INPUT_ADDR,
			NUM_ELEMS * sizeof(uint32_t));

	/* Enable the buffer descriptors in software dma descriptors */
	RC |= XAie_DmaEnableBd(&Tile_1_MM2S);
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
	RC |= XAie_DmaWriteBd(DevInst, &Tile_2_S2MM, Tile_2, 9U);

	XAie_DmaChannelDesc Tile_1_ChDesc;
	XAie_DmaChannelDescInit(DevInst, &Tile_1_ChDesc, Tile_1);
	XAie_DmaChannelEnCompression(&Tile_1_ChDesc, XAIE_ENABLE);
	XAie_DmaWriteChannel(DevInst, &Tile_1_ChDesc, Tile_1, 0, DMA_MM2S);

	XAie_DmaChannelDesc Tile_2_ChDesc;
	XAie_DmaChannelDescInit(DevInst, &Tile_2_ChDesc, Tile_2);
	XAie_DmaChannelEnCompression(&Tile_2_ChDesc, XAIE_ENABLE);
	XAie_DmaWriteChannel(DevInst, &Tile_2_ChDesc, Tile_2, 0, DMA_S2MM);

	/* Push Bd numbers to aie dma channel queues and enable the channels */
	RC |= XAie_DmaChannelPushBdToQueue(DevInst, Tile_2, 0U, DMA_S2MM, 9U);
	RC |= XAie_DmaChannelPushBdToQueue(DevInst, Tile_1, 0U, DMA_MM2S, 1U);
	RC |= XAie_DmaChannelEnable(DevInst, Tile_2, 0U, DMA_S2MM);
	RC |= XAie_DmaChannelEnable(DevInst, Tile_1, 0U, DMA_MM2S);
	if(RC != XAIE_OK) {
		printf("Failed to configure aie dma hardware and start dma "
				"tranactions.\n");
		return -1;
	}

	while(PendingBd != 0)
		RC = XAie_DmaGetPendingBdCount(DevInst, Tile_2, 0, DMA_S2MM,
				&PendingBd);
	/*
	 * Read data from aie data memory at DATA_MEM_OUTPUT_ADDR to compare
	 * with input data.
	 */
	RC |= XAie_DataMemBlockRead(DevInst, Tile_2, DATA_MEM_INPUT_ADDR,
			(void*)buffer, NUM_ELEMS * sizeof(uint32_t));
	if(RC != XAIE_OK) {
		printf("Failed to read from aie data memory.\n");
		return -1;
	}

	/* Check for correctness */
	for(uint8_t i = 0; i < NUM_ELEMS; i++) {
		if(data[i] != buffer[i]) {
			printf("Data mismatch at index %d.\n", i);
			printf("AIE DMA compression/decompression failed.\n");
			return -1;
		}
	}

	printf("AIE DMA compression/decompression success.\n");

	return 0;
}
