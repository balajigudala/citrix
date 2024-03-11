/******************************************************************************
* Copyright (C) 2021 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

/*****************************************************************************/
/**
* @file xaie_task_complete_tocken.c
* @{
*
* This file contains the test application to show case the task complete token
* feature(TCT) of the AIE DMA.
*
* This application write random integers to memory buffer on Tile_1. Tile_2
* is configured to receive NUM_ELEMS_MAX integers on S2MM channel 0 and the TCT
* feature is enabled on the same channel. TCT is sent to TCompLoc and stored in
* the data memory. Data recieved on Tile_2 and the TCT on TCompLoc  are checked
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
#define DATA_MEM_INOUT_ADDR	0x4000

#define NUM_ELEMS_MAX		128

uint32_t data[NUM_ELEMS_MAX];
uint32_t buffer[NUM_ELEMS_MAX];

/************************** Function Definitions *****************************/
void configure_dma_for_token(XAie_DevInst *DevInst, XAie_LocType DmaLoc,
		XAie_LocType TCompLoc)
{
	AieRC RC;
	XAie_DmaDesc TCompDesc;

	RC |= XAie_StrmConnCctEnable(DevInst, DmaLoc, CTRL, 0, EAST, 0);
	RC |= XAie_StrmConnCctEnable(DevInst, TCompLoc, WEST, 0, DMA, 0);

	RC |= XAie_DmaDescInit(DevInst, &TCompDesc, TCompLoc);
	RC |= XAie_DmaSetAddrLen(&TCompDesc, DATA_MEM_INOUT_ADDR,
			1 * sizeof(uint32_t));
	RC |= XAie_DmaEnableBd(&TCompDesc);
	RC |= XAie_DmaWriteBd(DevInst, &TCompDesc, TCompLoc, 5U);

	XAie_DmaChannelPushBdToQueue(DevInst, TCompLoc, 0, DMA_S2MM, 5);
}

int test_aie_dma_task_complete_token(XAie_DevInst *DevInst)
{
	AieRC RC = XAIE_OK;
	uint32_t Count;
	uint8_t PendingBd = 5;
	XAie_LocType Tile_1, Tile_2, TCompLoc;
	XAie_DmaDesc Tile_1_MM2S, Tile_2_S2MM;

	Tile_1 = XAie_TileLoc(1, XAIE_AIE_TILE_ROW_START);
	Tile_2 = XAie_TileLoc(1, XAIE_AIE_TILE_ROW_START+1);
	TCompLoc = XAie_TileLoc(2, XAIE_AIE_TILE_ROW_START+1);

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
			NUM_ELEMS_MAX * sizeof(uint32_t));
	RC |= XAie_DmaSetAddrLen(&Tile_2_S2MM, DATA_MEM_INOUT_ADDR,
			NUM_ELEMS_MAX * sizeof(uint32_t));

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

	/* Setup Task complete token controller ID */
	XAie_DmaChannelDesc Tile_2_ChDesc;
	XAie_DmaChannelDescInit(DevInst, &Tile_2_ChDesc, Tile_2);
	XAie_DmaChannelSetControllerId(&Tile_2_ChDesc, 0x9);
	XAie_DmaWriteChannel(DevInst, &Tile_2_ChDesc, Tile_2, 0, DMA_S2MM);

	configure_dma_for_token(DevInst, Tile_2, TCompLoc);

	/* Push Bd numbers to aie dma channel queues and enable the channels */
	RC |= XAie_DmaChannelPushBdToQueue(DevInst, Tile_1, 0U, DMA_MM2S, 1U);
	RC |= XAie_DmaChannelEnable(DevInst, Tile_1, 0U, DMA_MM2S);

	RC |= XAie_DmaChannelSetStartQueue(DevInst, Tile_2, 0, DMA_S2MM, 9, 1,
			XAIE_ENABLE);
	if(RC != XAIE_OK) {
		printf("Failed to configure aie dma hardware and start dma "
				"tranactions.\n");
		return -1;
	}

	do {
		RC = XAie_DmaGetPendingBdCount(DevInst, Tile_2, 0, DMA_S2MM,
				&PendingBd);
	} while(PendingBd != 0);
	do {
		RC = XAie_DmaGetPendingBdCount(DevInst, TCompLoc, 0, DMA_S2MM,
				&PendingBd);
	} while(PendingBd != 0);

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
	for(uint8_t i = 0; i < NUM_ELEMS_MAX; i++) {
		if(data[i] != buffer[i]) {
			printf("Data mismatch at index %d.\n", i);
			printf("AIE DMA Task Complete token test failed.\n");
			return -1;
		}
	}

	/* Read task complete token and validate */
	u32 TaskCompleteToken;
	RC |= XAie_DataMemBlockRead(DevInst, TCompLoc, DATA_MEM_INOUT_ADDR,
			(void*)&TaskCompleteToken, 1 * sizeof(uint32_t));

	u8 SourceCol, SourceRow, ControllerId, PacketType;
	SourceCol = (TaskCompleteToken >> 21 ) & 0x7F;
	SourceRow = (TaskCompleteToken >> 16) & 0x1F;
	ControllerId = TaskCompleteToken & 0x1F;
	PacketType = (TaskCompleteToken >> 12) & 0x7;

	if((SourceCol != Tile_2.Col) || (SourceRow != Tile_2.Row) ||
			(PacketType != 6) || (ControllerId != 9)) {
		printf("AIE DMA Task Complete token test failed.\n");
		printf("SourceCol: %d, SourceRow: %d\nPacketType: %d, "
				"ControllerId:%d\n", SourceCol, SourceRow,
				PacketType, ControllerId);
		return -1;
	}

	printf("AIE DMA Task complete token test success.\n");

	return 0;
}

int test_aie_tct_pl_fifo(XAie_DevInst *DevInst)
{
	AieRC RC = XAIE_OK;
	uint32_t Count;
	uint8_t PendingBd = 5;
	XAie_LocType Tile_1, Tile_2, TCompLoc;
	XAie_DmaDesc Tile_1_MM2S, Tile_2_S2MM;
	uint8_t col = 11;

	/* AXI FIFO is connected to stream port 0 on col 11 */
	Tile_1 = XAie_TileLoc(col, 3);
	Tile_2 = XAie_TileLoc(col, 4);

	/* Initialize array with random integers */
	for(uint32_t i = 0U; i < NUM_ELEMS_MAX; i++) {
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
			NUM_ELEMS_MAX * sizeof(uint32_t));
	RC |= XAie_DmaSetAddrLen(&Tile_2_S2MM, DATA_MEM_INOUT_ADDR,
			NUM_ELEMS_MAX * sizeof(uint32_t));

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

	/* Setup Task complete token controller ID */
	XAie_DmaChannelDesc Tile_2_ChDesc;
	XAie_DmaChannelDescInit(DevInst, &Tile_2_ChDesc, Tile_2);
	XAie_DmaChannelSetControllerId(&Tile_2_ChDesc, 0x9);
	XAie_DmaWriteChannel(DevInst, &Tile_2_ChDesc, Tile_2, 0, DMA_S2MM);

	RC |= XAie_StrmConnCctEnable(DevInst, Tile_2, CTRL, 0, SOUTH, 2);
	RC |= XAie_StrmConnCctEnable(DevInst, XAie_TileLoc(col, 3), NORTH, 2, SOUTH, 2);
	RC |= XAie_StrmConnCctEnable(DevInst, XAie_TileLoc(col, 2), NORTH, 2, SOUTH, 2);
	RC |= XAie_StrmConnCctEnable(DevInst, XAie_TileLoc(col, 1), NORTH, 2, SOUTH, 2);
	RC |= XAie_StrmConnCctEnable(DevInst, XAie_TileLoc(col, 0), NORTH, 2, SOUTH, 0);

	XAie_AieToPlIntfEnable(DevInst, XAie_TileLoc(col, 0), 0, PLIF_WIDTH_32);

	/* Push Bd numbers to aie dma channel queues and enable the channels */
	RC |= XAie_DmaChannelPushBdToQueue(DevInst, Tile_1, 0U, DMA_MM2S, 1U);
	RC |= XAie_DmaChannelEnable(DevInst, Tile_1, 0U, DMA_MM2S);

	RC |= XAie_DmaChannelSetStartQueue(DevInst, Tile_2, 0, DMA_S2MM, 9, 1,
			XAIE_ENABLE);
	if(RC != XAIE_OK) {
		printf("Failed to configure aie dma hardware and start dma "
				"tranactions.\n");
		return -1;
	}

	do {
		RC = XAie_DmaGetPendingBdCount(DevInst, Tile_2, 0, DMA_S2MM,
				&PendingBd);
	} while(PendingBd != 0);

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
	for(uint32_t i = 0; i < NUM_ELEMS_MAX; i++) {
		if(data[i] != buffer[i]) {
			printf("Data mismatch at index %d.\n", i);
			printf("AIE DMA Task Complete token-PL FIFO test failed.\n");
			return -1;
		}
	}

	volatile void *FifoBase = 0xA4010000;

	/* Check if TCT is available in fifo */
	uint32_t num_tokens = *((volatile uint32_t *) (FifoBase + 0x1C));
	printf("Number of tokens available in FIFO: %d\n", num_tokens);

	uint32_t token = *((volatile uint32_t *) (FifoBase + 0x20));
	printf("Token: 0x%x\n", token);

	u8 SourceCol, SourceRow, ControllerId, PacketType;
	SourceCol = (token >> 21 ) & 0x7F;
	SourceRow = (token >> 16) & 0x1F;
	ControllerId = token & 0x1F;
	PacketType = (token >> 12) & 0x7;

	if((SourceCol != Tile_2.Col) || (SourceRow != Tile_2.Row) ||
			(PacketType != 6) || (ControllerId != 9)) {
		printf("AIE DMA Task Complete token test failed.\n");
		printf("SourceCol: %d, SourceRow: %d\nPacketType: %d, "
				"ControllerId:%d\n", SourceCol, SourceRow,
				PacketType, ControllerId);
		return -1;
	}

	printf("AIE Tile DMA TCT-AXI_FIFO test success\n");

	return 0;
}
