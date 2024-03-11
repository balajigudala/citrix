/******************************************************************************
* Copyright (C) 2021 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

/*****************************************************************************/
/**
* @file xaie_dma_tlast_suppress.c
* @{
*
* This file contains the test application to show case TLAST suppress feature
* of AIE DMA
*
******************************************************************************/
/***************************** Include Files *********************************/
#include <stdio.h>
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

/* Input and output address in aie data memory */
#define DATA_MEM_INOUT_ADDR	0x4000

#define NUM_ELEMS_MAX 128
#define NUM_ELEMS NUM_ELEMS_MAX

/************************** Function Definitions *****************************/
int test_aie_dma_tlast(XAie_DevInst *DevInst)
{
	AieRC RC = XAIE_OK;
	uint32_t Count;
	uint32_t data[NUM_ELEMS_MAX];
	uint32_t buffer[NUM_ELEMS_MAX];
	uint8_t PendingBd = 5;
	XAie_LocType Tile_1, Tile_2;
	XAie_DmaDesc Tile_1_MM2S, Tile_2_S2MM;

	Tile_1 = XAie_TileLoc(1, XAIE_AIE_TILE_ROW_START);
	Tile_2 = XAie_TileLoc(1, XAIE_AIE_TILE_ROW_START + 1);

	/* Initialize array with random integers */
	for(uint8_t i = 0U; i < NUM_ELEMS_MAX; i++) {
		data[i] = rand() % 127;
	}

	/* Write data to aie tile data memory */
	RC = XAie_DataMemBlockWrite(DevInst, Tile_1, DATA_MEM_INOUT_ADDR,
			(void *)data, sizeof(uint32_t) * NUM_ELEMS_MAX);
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

	/* Configure address and length in dma software descriptors */
	RC |= XAie_DmaSetAddrLen(&Tile_1_MM2S, DATA_MEM_INOUT_ADDR,
			NUM_ELEMS_MAX * sizeof(uint32_t) / 2);
	RC |= XAie_DmaSetAddrLen(&Tile_2_S2MM, DATA_MEM_INOUT_ADDR,
			NUM_ELEMS_MAX * sizeof(uint32_t));

	/* Enable the buffer descriptors in software dma descriptors */
	RC |= XAie_DmaEnableBd(&Tile_1_MM2S);
	RC |= XAie_DmaEnableBd(&Tile_2_S2MM);
	if(RC != XAIE_OK) {
		printf("Failed to setup software dma descriptors.\n");
		return -1;
	}

	/* Disable TLAST on Tile_1MM2S */
	RC |= XAie_DmaTlastDisable(&Tile_1_MM2S);

	/*
	 * Configure aie dma hardware using software descriptors. Use buffer
	 * descriptor 1 for MM2S and 9 for S2MM on both tiles.
	 */
	RC |= XAie_DmaWriteBd(DevInst, &Tile_1_MM2S, Tile_1, 1U);
	RC |= XAie_DmaWriteBd(DevInst, &Tile_2_S2MM, Tile_2, 9U);


	XAie_DmaChannelDesc Tile_2_ChDesc;
	XAie_DmaChannelDescInit(DevInst, &Tile_2_ChDesc, Tile_2);
	XAie_DmaChannelSetFoTMode(&Tile_2_ChDesc, DMA_FoT_COUNTS_FROM_MM_REG);
	XAie_DmaWriteChannel(DevInst, &Tile_2_ChDesc, Tile_2, 0, DMA_S2MM);

	/* Push Bd numbers to aie dma channel queues and enable the channels */
	RC |= XAie_DmaChannelPushBdToQueue(DevInst, Tile_2, 0U, DMA_S2MM, 9U);
	RC |= XAie_DmaChannelEnable(DevInst, Tile_2, 0U, DMA_S2MM);
	RC |= XAie_DmaChannelPushBdToQueue(DevInst, Tile_1, 0U, DMA_MM2S, 1U);
	RC |= XAie_DmaChannelEnable(DevInst, Tile_1, 0U, DMA_MM2S);
	if(RC != XAIE_OK) {
		printf("Failed to configure aie dma hardware and start dma "
				"tranactions.\n");
		return -1;
	}

	int PollCount = 0;
	while(PendingBd != 0) {
		RC = XAie_DmaGetPendingBdCount(DevInst, Tile_2, 0, DMA_S2MM,
				&PendingBd);
		if(PollCount == 25) {
			printf("AIE Tile DMA TLAST Suppress successful. "
					"BD is stalling\n");
			break;
		}
		PollCount++;
	}

	/*
	 * Read data from aie data memory at DATA_MEM_OUTPUT_ADDR to compare
	 * with input data.
	 */
	RC |= XAie_DataMemBlockRead(DevInst, Tile_2, DATA_MEM_INOUT_ADDR,
			(void*)buffer, NUM_ELEMS_MAX * sizeof(uint32_t));
	if(RC != XAIE_OK) {
		printf("Failed to read from aie data memory.\n");
		return -1;
	}

	/* Check for correctness */
	for(uint8_t i = 0; i < NUM_ELEMS_MAX / 2; i++) {
		if(data[i] != buffer[i]) {
			printf("Data mismatch at index %d.\n", i);
			printf("AIE DMA TLAST suppress test failed.\n");
			return -1;
		}
	}

	printf("AIE DMA TLAST suppress test success.\n");

	return 0;
}

int test_mem_tile_dma_tlast(XAie_DevInst *DevInst)
{
	AieRC RC = XAIE_OK;
	uint32_t Count;
	uint32_t data[NUM_ELEMS_MAX];
	uint32_t buffer[NUM_ELEMS_MAX];
	uint8_t PendingBd = 5;
	XAie_LocType MemTile;
	XAie_DmaDesc DmaMM2S, DmaS2MM;

	MemTile = XAie_TileLoc(0, 1);

	/* Initialize array with random integers */
	for(uint8_t i = 0U; i < NUM_ELEMS; i++) {
		data[i] = rand() % 127;
	}

	/* Write data to aie tile data memory */
	RC = XAie_DataMemBlockWrite(DevInst, MemTile, DATA_MEM_INPUT_ADDR,
			(void *)data, sizeof(uint32_t) * NUM_ELEMS);
	if(RC != XAIE_OK) {
		printf("Writing data to aie data memory failed.\n");
		return -1;
	}

	RC = XAie_StrmConnCctEnable(DevInst, MemTile, DMA, 0, DMA, 0);
	if(RC != XAIE_OK) {
		printf("Failed to configure stream switches.\n");
		return -1;
	}

	/* Initialize software descriptors for aie dma */
	RC |= XAie_DmaDescInit(DevInst, &DmaMM2S, MemTile);
	RC |= XAie_DmaDescInit(DevInst, &DmaS2MM, MemTile);

	/* Configure address and length in dma software descriptors */
	RC |= XAie_DmaSetAddrLen(&DmaMM2S, 0x80000 + DATA_MEM_INPUT_ADDR,
			NUM_ELEMS * sizeof(uint32_t) / 2);
	RC |= XAie_DmaSetAddrLen(&DmaS2MM, 0x80000 + DATA_MEM_OUTPUT_ADDR,
			NUM_ELEMS * sizeof(uint32_t));
	/* Enable the buffer descriptors in software dma descriptors */
	RC |= XAie_DmaEnableBd(&DmaMM2S);
	RC |= XAie_DmaEnableBd(&DmaS2MM);
	if(RC != XAIE_OK) {
		printf("Failed to setup software dma descriptors.\n");
		return -1;
	}

	/* Disable TLAST on DmaMM2S */
	RC |= XAie_DmaTlastDisable(&DmaMM2S);

	RC |= XAie_DmaWriteBd(DevInst, &DmaMM2S, MemTile, 1U);
	RC |= XAie_DmaWriteBd(DevInst, &DmaS2MM, MemTile, 9U);

	XAie_DmaChannelDesc MemTile_S2MM_ChDesc;
	XAie_DmaChannelDescInit(DevInst, &MemTile_S2MM_ChDesc, MemTile);
	XAie_DmaChannelSetFoTMode(&MemTile_S2MM_ChDesc,
			DMA_FoT_COUNTS_FROM_MM_REG);
	XAie_DmaWriteChannel(DevInst, &MemTile_S2MM_ChDesc, MemTile, 0,
			DMA_S2MM);

	/* Push Bd numbers to aie dma channel queues and enable the channels */
	RC |= XAie_DmaChannelPushBdToQueue(DevInst, MemTile, 0U, DMA_MM2S, 1U);
	RC |= XAie_DmaChannelEnable(DevInst, MemTile, 0U, DMA_MM2S);
	RC |= XAie_DmaChannelPushBdToQueue(DevInst, MemTile, 0U, DMA_S2MM, 9U);
	RC |= XAie_DmaChannelEnable(DevInst, MemTile, 0U, DMA_S2MM);

	if(RC != XAIE_OK) {
		printf("Failed to configure aie dma hardware and start dma "
				"tranactions.\n");
		return -1;
	}

	int PollCount = 0;
	while(PendingBd != 0) {
		RC = XAie_DmaGetPendingBdCount(DevInst, MemTile, 0, DMA_S2MM,
				&PendingBd);
		if(PollCount == 25) {
			printf("AIE Mem Tile DMA TLAST Suppress successful. "
					"BD is stalling\n");
			break;
		}
		PollCount++;
	}

	/*
	 * Read data from aie data memory at DATA_MEM_OUTPUT_ADDR to compare
	 * with input data.
	 */
	RC |= XAie_DataMemBlockRead(DevInst, MemTile, DATA_MEM_OUTPUT_ADDR,
			(void*)buffer, NUM_ELEMS * sizeof(uint32_t));
	if(RC != XAIE_OK) {
		printf("Failed to read from aie data memory.\n");
		return -1;
	}

	/* Check for correctness */
	for(uint8_t i = 0; i < NUM_ELEMS / 2; i++) {
		if(data[i] != buffer[i]) {
			printf("Data mismatch at index %d.\n", i);
			printf("AIE Mem Tile DMA TLAST suppress failed.\n");
			return -1;
		}
	}

	printf("AIE Mem Tile DMA TLAST suppress success.\n");

	return 0;
}
