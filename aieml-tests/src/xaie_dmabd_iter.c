/******************************************************************************
* Copyright (C) 2021 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

/*****************************************************************************/
/**
* @file xaie_dmabd_iter.c
* @{
*
* This file contains the test application to show case dma bd iteration feature
* of AIE DMA
*
* This application write integers to memory buffer on Tile_1. Tile_2
* is configured to receive small chunks of input integers on S2MM channel 0.
* Tile_1 DMA MM2S channel is configured to run 4 times with the BD configured
* to test the BD iteration feature. The output data on Tile_2 is checked for
* correctness.
*
******************************************************************************/
/***************************** Include Files *********************************/
#include <stdio.h>
#include <stdlib.h>
#include <xaiengine.h>

#include "hw_config.h"

/************************** Constant Definitions *****************************/
/* Input and output address in aie data memory */
#define DATA_MEM_INOUT_ADDR		0x2000

#define INPUT_ELEMENTS_NUM 		128
#define DMA_BD_ITERATION_STEP_SIZE 	32
#define DMA_BD_ITERATION_WRAP		4

#define DMA_CHANNEL_REPEAT_COUNT	4


/************************** Function Definitions *****************************/
int test_aie_dmabd_iter(XAie_DevInst *DevInst)
{
	AieRC RC = XAIE_OK;
	uint32_t Count;
	uint32_t data[INPUT_ELEMENTS_NUM];
	uint32_t buffer[INPUT_ELEMENTS_NUM];
	uint8_t PendingBd = 5;
	XAie_LocType Tile_1, Tile_2;
	XAie_DmaDesc Tile_1_MM2S, Tile_2_S2MM;

	Tile_1 = XAie_TileLoc(3, XAIE_AIE_TILE_ROW_START);
	Tile_2 = XAie_TileLoc(3, XAIE_AIE_TILE_ROW_START + 1);

	/* Initialize array with random integers */
	for(uint8_t i = 0U; i < INPUT_ELEMENTS_NUM; i++) {
		data[i] = i;
	}

	/* Write data to aie tile data memory */
	RC = XAie_DataMemBlockWrite(DevInst, Tile_1, DATA_MEM_INOUT_ADDR,
			(void *)data, sizeof(uint32_t) * INPUT_ELEMENTS_NUM);
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

	RC |= XAie_DmaSetBdIteration(&Tile_1_MM2S, DMA_BD_ITERATION_STEP_SIZE,
			DMA_BD_ITERATION_WRAP, 0);

	/* Configure address and length in dma software descriptors */
	RC |= XAie_DmaSetAddrLen(&Tile_1_MM2S, DATA_MEM_INOUT_ADDR,
			INPUT_ELEMENTS_NUM * sizeof(uint32_t) / 8);
	RC |= XAie_DmaSetAddrLen(&Tile_2_S2MM, DATA_MEM_INOUT_ADDR,
			INPUT_ELEMENTS_NUM * sizeof(uint32_t) / 2);

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

	/* Push Bd numbers to aie dma channel queues and enable the channels */
	RC |= XAie_DmaChannelPushBdToQueue(DevInst, Tile_2, 0U, DMA_S2MM, 9U);
	RC |= XAie_DmaChannelEnable(DevInst, Tile_2, 0U, DMA_S2MM);

	RC |= XAie_DmaChannelSetStartQueue(DevInst, Tile_1, 0, DMA_MM2S, 1,
			DMA_CHANNEL_REPEAT_COUNT, 0);
	if(RC != XAIE_OK) {
		printf("Failed to configure aie dma hardware and start dma "
				"tranactions.\n");
		return -1;
	}

	while(PendingBd != 0) {
		RC = XAie_DmaGetPendingBdCount(DevInst, Tile_2, 0, DMA_S2MM,
				&PendingBd);
	}

	while(PendingBd != 0) {
		RC = XAie_DmaGetPendingBdCount(DevInst, Tile_1, 0, DMA_MM2S,
				&PendingBd);
	}

	/*
	 * Read data from aie data memory at DATA_MEM_OUTPUT_ADDR to compare
	 * with input data.
	 */
	RC |= XAie_DataMemBlockRead(DevInst, Tile_2, DATA_MEM_INOUT_ADDR,
			(void*)buffer,
			INPUT_ELEMENTS_NUM * sizeof(uint32_t));
	if(RC != XAIE_OK) {
		printf("Failed to read from aie data memory.\n");
		return -1;
	}

	/* Check for correctness */
	uint8_t comp_val = 0;
	for(uint8_t i = 0; i < INPUT_ELEMENTS_NUM / 2; i++) {
		if((i % 16 == 0) && (i != 0)) {
			comp_val =  comp_val + 16 * 2;
		}
		if(buffer[i] != (i % 16) + comp_val) {
			printf("Data mismatch at index %d.\n", i);
			printf("AIE DMA BD Iteration test failed\n");
			return -1;
		}
	}

	printf("AIE DMA BD Iteration test success.\n");

	return 0;
}

int test_mem_tile_bditer(XAie_DevInst *DevInst)
{
	AieRC RC = XAIE_OK;
	uint32_t data[INPUT_ELEMENTS_NUM];
	uint32_t buffer[INPUT_ELEMENTS_NUM];
	uint8_t PendingBd = 5;
	XAie_LocType Tile_1, Tile_2, Tile_3, Tile_4, AieTile_2, AieTile_3;
	XAie_DmaDesc Tile_3_S2MM, Tile_2_MM2S;

	Tile_2 = XAie_TileLoc(1, XAIE_AIE_TILE_ROW_START - 1);
	Tile_3 = XAie_TileLoc(2, XAIE_AIE_TILE_ROW_START - 1);
	AieTile_2 = XAie_TileLoc(1, XAIE_AIE_TILE_ROW_START);
	AieTile_3 = XAie_TileLoc(2, XAIE_AIE_TILE_ROW_START);

	/* Initialize array with random integers */
	for(uint8_t i = 0U; i < INPUT_ELEMENTS_NUM; i++) {
		data[i] = i;
	}

	/* Write data to aie tile data memory */
	RC = XAie_DataMemBlockWrite(DevInst, Tile_2, DATA_MEM_INOUT_ADDR,
			(void *)data, sizeof(uint32_t) * INPUT_ELEMENTS_NUM);
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

	RC |= XAie_DmaSetBdIteration(&Tile_2_MM2S, DMA_BD_ITERATION_STEP_SIZE,
			DMA_BD_ITERATION_WRAP, 0);
	RC |= XAie_DmaSetAddrLen(&Tile_2_MM2S, 0x80000 + DATA_MEM_INOUT_ADDR,
			INPUT_ELEMENTS_NUM * sizeof(uint32_t) / 8);
	RC |= XAie_DmaSetAddrLen(&Tile_3_S2MM, 0x80000 + DATA_MEM_INOUT_ADDR,
			INPUT_ELEMENTS_NUM * sizeof(uint32_t) / 2);
	RC |= XAie_DmaEnableBd(&Tile_2_MM2S);
	RC |= XAie_DmaEnableBd(&Tile_3_S2MM);
	RC |= XAie_DmaWriteBd(DevInst, &Tile_2_MM2S, Tile_2, 0U);
	RC |= XAie_DmaWriteBd(DevInst, &Tile_3_S2MM, Tile_3, 0U);

	RC |= XAie_DmaChannelPushBdToQueue(DevInst, Tile_3, 0U, DMA_S2MM, 0U);
	RC |= XAie_DmaChannelSetStartQueue(DevInst, Tile_2, 0, DMA_MM2S, 0,
			DMA_CHANNEL_REPEAT_COUNT, 0);
	if(RC != XAIE_OK) {
		printf("Failed to configure aie dma hardware and start dma "
				"tranactions.\n");
		return -1;
	}

	while(PendingBd != 0)
		RC = XAie_DmaGetPendingBdCount(DevInst, Tile_2, 0, DMA_MM2S,
				&PendingBd);

	PendingBd = 5;
	while(PendingBd != 0)
		RC = XAie_DmaGetPendingBdCount(DevInst, Tile_3, 0, DMA_S2MM,
				&PendingBd);
	/*
	 * Read data from aie data memory at DATA_MEM_INPUT`_ADDR to compare
	 * with input data.
	 */
	RC |= XAie_DataMemBlockRead(DevInst, Tile_3, DATA_MEM_INOUT_ADDR,
			(void*)buffer, INPUT_ELEMENTS_NUM * sizeof(uint32_t));
	if(RC != XAIE_OK) {
		printf("Failed to read from aie data memory.\n");
		return -1;
	}

	/* Check for correctness */
	uint8_t comp_val = 0;
	for(uint8_t i = 0; i < INPUT_ELEMENTS_NUM / 2; i++) {
		if((i % 16 == 0) && (i != 0)) {
			comp_val =  comp_val + 16 * 2;
		}
		if(buffer[i] != (i % 16) + comp_val) {
			printf("Data mismatch at index %d.\n", i);
			printf("AIE Mem Tile DMA BD Iteration test failed\n");
			return -1;
		}
	}

	printf("AIE Mem Tile BD Iteration test success.\n");

	return 0;
}
