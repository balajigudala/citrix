/******************************************************************************
* Copyright (C) 2021 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/


/*****************************************************************************/
/**
* @file xaie_dma_loopback.c
* @{
*
* This file contains the test application of shim dma loopback for aie.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who     Date     Changes
* ----- ------  -------- -----------------------------------------------------
* 1.0   Tejus   10/18/2021  Initial creation
* </pre>
*
******************************************************************************/

/***************************** Include Files *********************************/
#include <stdlib.h>
#include <xaiengine.h>

/************************** Constant Definitions *****************************/
#define SIZE 512
/* SHIM DMA - NOC - DDR path on the below columns are enabled */
int cols[12] = {2, 3, 6, 7, 14, 15, 22, 23, 30, 31, 34, 35};

/*****************************************************************************/
/**
*
* This is the main entry point for the AIE driver Shim DMA loopback example.
*
* @param	src: source ddr buffer address
* @param	dest: destnation ddr buffer address
* @param	size: size of the buffer in bytes
* @param	col: column of the shim dma to be used.
*
* @return	0
*
* @note		The input/output data buffer allocation and initialization
*		should be taken care by the user of this function.
*		This implementation has been tested on baremetal with aie1.
*
*******************************************************************************/
int xaie_shimdma_loopback(uint64_t src, uint64_t dest, uint32_t size,
		uint8_t col, XAie_DevInst *DevInst)
{
	AieRC RC;
	XAie_LocType ShimDma;
	XAie_DmaDesc DmaDesc1, DmaDesc2;
	u8 PendingBd = 5;

	ShimDma = XAie_TileLoc(col, 0);

	printf("Configuring AIE...\n");
	RC = XAie_StrmConnCctEnable(DevInst, ShimDma, SOUTH, 3, SOUTH, 3);
	RC = XAie_EnableShimDmaToAieStrmPort(DevInst, ShimDma, 3);
	RC = XAie_EnableAieToShimDmaStrmPort(DevInst, ShimDma, 3);

	RC = XAie_DmaDescInit(DevInst, &DmaDesc1, ShimDma);
	RC = XAie_DmaSetAddrLen(&DmaDesc1, src, size);
	RC = XAie_DmaEnableBd(&DmaDesc1);
	RC = XAie_DmaSetAxi(&DmaDesc1, 0U, 16U, 0U, 0U, 0U);
	RC = XAie_DmaWriteBd(DevInst, &DmaDesc1, ShimDma, 2);

	RC = XAie_DmaDescInit(DevInst, &DmaDesc2, ShimDma);
	RC = XAie_DmaSetAddrLen(&DmaDesc2, dest, size);
	RC = XAie_DmaEnableBd(&DmaDesc2);
	RC = XAie_DmaSetAxi(&DmaDesc2, 0U, 16U, 0U, 0U, 0U);
	RC = XAie_DmaWriteBd(DevInst, &DmaDesc2, ShimDma, 5);

	printf("Enabling channels....\n");
	RC = XAie_DmaChannelPushBdToQueue(DevInst, ShimDma, 1, DMA_S2MM, 5);
	RC = XAie_DmaChannelPushBdToQueue(DevInst, ShimDma, 0, DMA_MM2S, 2);

	RC = XAie_DmaChannelEnable(DevInst, ShimDma, 1, DMA_S2MM);
	RC = XAie_DmaChannelEnable(DevInst, ShimDma, 0, DMA_MM2S);

	do {
	        XAie_DmaGetPendingBdCount(DevInst, ShimDma, 1, DMA_S2MM,
				&PendingBd);
	        printf("PendingBd: %d\n", PendingBd);
	} while(PendingBd != 0);

	return 0;
}

int test_aie_shimdma_loopback(XAie_DevInst *DevInst)
{
	for(int i = 0; i < sizeof(cols)/sizeof(cols[0]); i++) {
		int col = cols[i];
		printf("Running shim dma loopback for column %d\n", col);

		XAie_MemInst *in, *out;
		uint32_t *src, *dest;
		in = XAie_MemAllocate(DevInst, SIZE * sizeof(int), XAIE_MEM_CACHEABLE);
		out = XAie_MemAllocate(DevInst, SIZE * sizeof(int), XAIE_MEM_CACHEABLE);

		src = (uint32_t *)XAie_MemGetDevAddr(in);
		dest = (uint32_t *)XAie_MemGetDevAddr(out);

		XAie_MemSyncForCPU(in);
		XAie_MemSyncForCPU(out);
		for(int i = 0; i < SIZE; i++) {
			src[i] = i * col;
			dest[i] = 0x33;
		}

		XAie_MemSyncForDev(in);
		XAie_MemSyncForDev(out);

		xaie_shimdma_loopback((uint64_t )src, (uint64_t)dest,
				SIZE * sizeof(int), col, DevInst);

		XAie_MemSyncForCPU(in);
		XAie_MemSyncForCPU(out);
		for(int i = 0; i < SIZE; i++) {
			if(src[i] != dest[i]) {
				printf("Test failed at index %d for column %d\n", i, col);
				return -1;
			}
		}

		XAie_MemFree(in);
		XAie_MemFree(out);

		printf("SHIM DMA loopback on col %d passed\n", col);
	}

	return 0;
}
