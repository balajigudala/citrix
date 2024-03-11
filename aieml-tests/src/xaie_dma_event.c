/******************************************************************************
* Copyright (C) 2022 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/


/*****************************************************************************/
/**
* @file xaie_dma_event.c
* @{
*
* This file contains the test application of dma memtile events for aie.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who     Date     Changes
* ----- ------  -------- -----------------------------------------------------
* 1.0   Gregory 12/29/2022  Initial creation
* </pre>
*
******************************************************************************/

/***************************** Include Files *********************************/
#include <stdlib.h>
#include <xaiengine.h>

/************************** Constant Definitions *****************************/
/* Lock values used for synchronization */
#define LOCK_FOR_WRITE	0
#define LOCK_FOR_READ	1

/* Input and output address in aie data memory */
#define DATA_MEM_INPUT_ADDR	0x4000
#define DATA_MEM_OUTPUT_ADDR	0x3000

#define NUM_ELEMS 32

#define DMA_GROUP_BITMAP	0xFFFFFFFF
#define DMA_CHANNEL_NUM		1U
#define DMA_SELECT_ID		0U
#define DMA_MM2S_BD		31U
#define DMA_S2MM_BD		39U

static uint32_t data[NUM_ELEMS];
static uint32_t buffer[NUM_ELEMS];

/*****************************************************************************/
/**
*
* This is the main entry point for the AIE driver memtile DMA event example.
*
* @param	None.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_aie_dma_event(XAie_DevInst *DevInst)
{
	AieRC RC = XAIE_OK;
	uint8_t Status = 0, PendingBd = 5;
	XAie_LocType MemTile, EastTile;
	XAie_DmaDesc MemTile_MM2S, MemTile_S2MM;
	XAie_DmaDesc EastTile_MM2S, EastTile_S2MM;

	MemTile = XAie_TileLoc(1, 1);
	EastTile = XAie_TileLoc(2, 1);

	/* Configure ss ports to move data from memtile to aietile */
	RC |= XAie_StrmConnCctEnable(DevInst, MemTile, DMA, 0, NORTH, 0);
	RC |= XAie_StrmConnCctEnable(DevInst, EastTile, SOUTH, 0, DMA, 0);

	/* Configure ss ports to move data from aietile to memtile */
	RC |= XAie_StrmConnCctEnable(DevInst, EastTile, DMA, 0, SOUTH, 0);
	RC |= XAie_StrmConnCctEnable(DevInst, MemTile, NORTH, 0, DMA, 0);

	if(RC != XAIE_OK) {
		printf("Failed to configure stream switches.\n");
		return -1;
	}

	/* Initialize software descriptors for aie dma */
	RC |= XAie_DmaDescInit(DevInst, &MemTile_MM2S, MemTile);
	RC |= XAie_DmaDescInit(DevInst, &MemTile_S2MM, MemTile);
	RC |= XAie_DmaDescInit(DevInst, &EastTile_MM2S, EastTile);
	RC |= XAie_DmaDescInit(DevInst, &EastTile_S2MM, EastTile);

	/* Configure address and length in dma software descriptors */
	RC |= XAie_DmaSetAddrLen(&MemTile_MM2S, DATA_MEM_INPUT_ADDR,
			NUM_ELEMS * sizeof(uint32_t));
	RC |= XAie_DmaSetAddrLen(&MemTile_S2MM, DATA_MEM_OUTPUT_ADDR,
			NUM_ELEMS * sizeof(uint32_t));
	RC |= XAie_DmaSetAddrLen(&EastTile_MM2S, DATA_MEM_INPUT_ADDR,
			NUM_ELEMS * sizeof(uint32_t));
	RC |= XAie_DmaSetAddrLen(&EastTile_S2MM, DATA_MEM_INPUT_ADDR,
			NUM_ELEMS * sizeof(uint32_t));

	/*
	 * Configure locks for dma channel synchronization. S2MM should run
	 * before MM2S on EastTile. Lock Id 5 is used for synchronization.
	 */
	RC |= XAie_DmaSetLock(&EastTile_S2MM, XAie_LockInit(5U, LOCK_FOR_WRITE),
			XAie_LockInit(5, LOCK_FOR_READ));
	RC |= XAie_DmaSetLock(&EastTile_MM2S, XAie_LockInit(5U, LOCK_FOR_READ),
			XAie_LockInit(5, LOCK_FOR_WRITE));
	/*
	 * Configure locks for dma channel synchronization. MM2S should run
	 * before S2MM on MemTile. Lock Id 6 is used for synchronization.
	 */
	RC |= XAie_DmaSetLock(&MemTile_MM2S, XAie_LockInit(6U, LOCK_FOR_WRITE),
			XAie_LockInit(6U, LOCK_FOR_READ));
	RC |= XAie_DmaSetLock(&MemTile_S2MM, XAie_LockInit(6U, LOCK_FOR_READ),
			XAie_LockInit(6U, LOCK_FOR_WRITE));

	/* Enable the buffer descriptors in software dma descriptors */
	RC |= XAie_DmaEnableBd(&MemTile_MM2S);
	RC |= XAie_DmaEnableBd(&MemTile_S2MM);
	RC |= XAie_DmaEnableBd(&EastTile_MM2S);
	RC |= XAie_DmaEnableBd(&EastTile_S2MM);

	if(RC != XAIE_OK) {
		printf("Failed to setup software dma descriptors.\n");
		return -1;
	}

	/* Enable DMA group events */
	RC |= XAie_EventGroupControl(DevInst, MemTile, XAIE_MEM_MOD,
			XAIE_EVENT_GROUP_DMA_ACTIVITY_MEM_TILE,
			DMA_GROUP_BITMAP);

	/* Select Dma channels to generate events */
	RC |= XAie_EventSelectDmaChannel(DevInst, MemTile, DMA_SELECT_ID,
						DMA_MM2S, DMA_CHANNEL_NUM);
	RC |= XAie_EventSelectDmaChannel(DevInst, MemTile, DMA_SELECT_ID,
						DMA_S2MM, DMA_CHANNEL_NUM);

	if(RC != XAIE_OK) {
		printf("Failed to setup dma events.\n");
		return -1;
	}
	/*
	 * Configure aie dma hardware using software descriptors. Use buffer
	 * descriptor 1 for MM2S and 9 for S2MM on both tiles.
	 */
	RC |= XAie_DmaWriteBd(DevInst, &MemTile_MM2S, MemTile, DMA_MM2S_BD);
	RC |= XAie_DmaWriteBd(DevInst, &MemTile_S2MM, MemTile, DMA_S2MM_BD);
	RC |= XAie_DmaWriteBd(DevInst, &EastTile_MM2S, EastTile, DMA_MM2S_BD);
	RC |= XAie_DmaWriteBd(DevInst, &EastTile_S2MM, EastTile, DMA_S2MM_BD);

	/* Push Bd numbers to aie dma channel queues and enable the channels */
	RC |= XAie_DmaChannelPushBdToQueue(DevInst, MemTile, DMA_CHANNEL_NUM,
						DMA_MM2S, DMA_MM2S_BD);
	RC |= XAie_DmaChannelPushBdToQueue(DevInst, MemTile, DMA_CHANNEL_NUM,
						DMA_S2MM, DMA_S2MM_BD);
	RC |= XAie_DmaChannelPushBdToQueue(DevInst, EastTile, DMA_CHANNEL_NUM,
						DMA_MM2S, DMA_MM2S_BD);
	RC |= XAie_DmaChannelPushBdToQueue(DevInst, EastTile, DMA_CHANNEL_NUM,
						DMA_S2MM, DMA_S2MM_BD);
	RC |= XAie_DmaChannelEnable(DevInst, MemTile, DMA_CHANNEL_NUM, DMA_MM2S);
	RC |= XAie_DmaChannelEnable(DevInst, MemTile, DMA_CHANNEL_NUM, DMA_S2MM);
	RC |= XAie_DmaChannelEnable(DevInst, EastTile, DMA_CHANNEL_NUM, DMA_MM2S);
	RC |= XAie_DmaChannelEnable(DevInst, EastTile, DMA_CHANNEL_NUM, DMA_S2MM);
	if(RC != XAIE_OK) {
		printf("Failed to configure aie dma hardware and start dma "
				"tranactions.\n");
		return -1;
	}

	/* Wait for transfers */
	while(PendingBd != 0) {
		RC = XAie_DmaGetPendingBdCount(DevInst, MemTile, 0, DMA_S2MM,
				&PendingBd);
		if(RC != XAIE_OK) {
			printf("Failed to get PendingBd count.\n");
			return -1;
		}
	}
#if DEVICE == 0
	sleep(1);
#endif

	/* Check status of DMA events */
	RC = XAie_EventReadStatus(DevInst, MemTile, XAIE_MEM_MOD,
			XAIE_EVENT_GROUP_DMA_ACTIVITY_MEM_TILE, &Status);
	if(RC != XAIE_OK) {
		printf("Failed to get status of dma group event.\n");
		return -1;
	}
	if(!Status) {
		printf("DMA group event not triggered.\n");
		return -1;
	}

	RC = XAie_EventReadStatus(DevInst, MemTile, XAIE_MEM_MOD,
			XAIE_EVENT_DMA_S2MM_SEL0_START_TASK_MEM_TILE, &Status);
	if(RC != XAIE_OK) {
		printf("Failed to get status of s2mm dma start task event.\n");
		return -1;
	}
	if(!Status) {
		printf("S2MM DMA start task event not triggered.\n");
		return -1;
	}

	RC = XAie_EventReadStatus(DevInst, MemTile, XAIE_MEM_MOD,
			XAIE_EVENT_DMA_MM2S_SEL0_START_TASK_MEM_TILE, &Status);
	if(RC != XAIE_OK) {
		printf("Failed to get status of mm2s dma start task event.\n");
		return -1;
	}
	if(!Status) {
		printf("MM2S DMA start task event not triggered.\n");
		return -1;
	}

	printf("AIE Memtile DMA Event success.\n");
	return 0;
}
/** @} */
