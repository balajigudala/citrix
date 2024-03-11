/******************************************************************************
 * Copyright (C) 2021 Xilinx, Inc.  All rights reserved.
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

/*****************************************************************************/
/**
 * @file xaie_axi_mm_outstanding_txn.c
 * @{
 *
 * This file contains the test application to checks the Uc AxiMm Outstanding
 * transaction status and Noc AxiMm Outstanding transaction status.
 *
 * This application checks the Uc AxiMm Outstanding and Noc AxiMm Outstanding
 * transaction register data in various scenarios while the loopback is
 * running.
 *
 ******************************************************************************/

/***************************** Include Files *********************************/
#include <stdlib.h>
#include <xaiengine.h>
#include "ucfirmware/ucheader.h"

#ifdef _UCFW_
#include "../hw_config.h"
/************************** Constant Definitions *****************************/
extern const char _binary___app_elf_start[];
extern const char _binary___app_elf_end[];
void *app_elf_start_AxiMmtxn = _binary___app_elf_start;
#else
#define ELF_LOAD_START 0x2000000
void *app_elf_start_AxiMmtxn = ELF_LOAD_START;
#endif

#define SIZE               102400
#define UC_DMA_TO_NMU      1
#define UC_MODULE_TO_ARRAY 2

#if AIE_GEN == 5
int test_aie_uc_aximmoutstanding_txn(XAie_DevInst *DevInst)
{
	AieRC RC;
	u32 uc_sts;
	XAie_LocType Loc = {0, 0};

	u8 TileType = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
	uint32_t shared_mem_base_addr =
		DevInst->DevProp.DevMod[TileType].UcMod->DataMemAddr;
	uint32_t smvar1 = 0, smvar2 = 0;
	uint32_t addr = _XAie_GetTileAddr(DevInst, Loc.Row, Loc.Col)
		+ shared_mem_base_addr;

	XAie_Write32(DevInst, addr, smvar1);
	XAie_Read32(DevInst, addr, &smvar1);

	void *ElfMem = app_elf_start_AxiMmtxn;
	if(ElfMem == NULL) {
		printf("Error load the UC firmware data section.\n");
		return -1;
	}

	/********* checking register before starting txn ********/
	uc_sts = -1;
	XAie_GetUcDmaAxiMmOutstandingTxn(DevInst, Loc, &uc_sts);
	printf("before starting UcDma transaction "
			"the aximm outstanding_txn status: %u\n", uc_sts);

	/************* starting the ucDma transaction ************/
	RC = XAie_LoadUcMem(DevInst, Loc, ElfMem);
	if (XAIE_OK != RC) {
		printf("failed to Load UC elf.\n");
		return -1;
	}

	printf("Load UC elf.\n");
	const XAie_UcMod *UcMod = DevInst->DevProp.DevMod[TileType].UcMod;
	u32 status1, status2;

	UcMod->GetCoreStatus(DevInst, Loc, &status1, UcMod);
	printf("Get core status.%x before Wakeup\n", status1);
	UcMod->Wakeup(DevInst, Loc, UcMod);
	UcMod->GetCoreStatus(DevInst, Loc, &status2, UcMod);
	printf("Get core status.%x after Wakeup\n", status2);

	if (status1 == status2) {
		return -1;
	}

	uint32_t count = 0;
	while(smvar2 != MAGIC_DATA && count++ < 60) {
		XAie_Read32(DevInst, addr, &smvar2);
		sleep(1);
	}

	if (smvar2 != MAGIC_DATA){
		printf("timeout the UC load failed\n");
			return -1;
	}

	/******* checking register after Ucdma txn started *******/
	int isUcDmaToNmuTxn = 0, isUcModuleToArrayTxn = 0;
	for (int i = 0; i <= 15000; i++) {
		uc_sts = -1;
		XAie_GetUcDmaAxiMmOutstandingTxn(DevInst, Loc,	&uc_sts);
		if ((uc_sts == UC_DMA_TO_NMU) && (uc_sts != isUcDmaToNmuTxn)) {
			printf("/****** txn from UcDma to NMU ******/\n");
			isUcDmaToNmuTxn = UC_DMA_TO_NMU;
		} else if ((uc_sts == UC_MODULE_TO_ARRAY) &&
				(uc_sts != isUcModuleToArrayTxn)) {
			printf("/****** txn from UcModule to Array ******/\n");
			isUcModuleToArrayTxn = UC_MODULE_TO_ARRAY;
		}

		if (isUcDmaToNmuTxn && isUcModuleToArrayTxn) {
			printf("AIE uc_aximm_outstanding_txn success\n");
			return XAIE_OK;
		}
	}

	printf("AIE uc_aximm_outstanding_txn failed\n");
	return XAIE_OK;
}

int xaie_aximm_outstanding_loopback(uint64_t src, uint64_t dest,
		uint32_t size, uint8_t col, XAie_DevInst *DevInst)
{
	AieRC RC;
	u32 noc_sts;
	u8 PendingBd = 0;
	XAie_DmaDesc DmaDesc1, DmaDesc2;
	XAie_LocType tile_loc = XAie_TileLoc(col, 0);

	printf("Configuring AIE...\n");
	RC = XAie_StrmConnCctEnable(DevInst, tile_loc, SOUTH, 3, SOUTH, 3);
	RC = XAie_EnableShimDmaToAieStrmPort(DevInst, tile_loc, 3);
	RC = XAie_EnableAieToShimDmaStrmPort(DevInst, tile_loc, 3);

	RC = XAie_DmaDescInit(DevInst, &DmaDesc1, tile_loc);
	RC = XAie_DmaSetAddrLen(&DmaDesc1, src, size);

	RC = XAie_DmaDescInit(DevInst, &DmaDesc2, tile_loc);
	RC = XAie_DmaSetAddrLen(&DmaDesc2, dest, size);

	RC = XAie_DmaEnableBd(&DmaDesc1);
	RC = XAie_DmaSetAxi(&DmaDesc1, 0U, 16U, 0U, 0U, 0U);
	RC = XAie_DmaEnableBd(&DmaDesc2);
	RC = XAie_DmaSetAxi(&DmaDesc2, 0U, 16U, 0U, 0U, 0U);

	RC = XAie_DmaWriteBd(DevInst, &DmaDesc1, tile_loc, 2);
	RC = XAie_DmaWriteBd(DevInst, &DmaDesc2, tile_loc, 5);

	printf("Enabling channels....\n");
	RC = XAie_DmaChannelPushBdToQueue(DevInst, tile_loc, 1, DMA_S2MM, 5);
	RC = XAie_DmaChannelPushBdToQueue(DevInst, tile_loc, 0, DMA_MM2S, 2);

	RC = XAie_DmaChannelEnable(DevInst, tile_loc, 1, DMA_S2MM);
	RC = XAie_DmaChannelEnable(DevInst, tile_loc, 0, DMA_MM2S);

	noc_sts = -1;
	printf("/****** while Running aximm oustanding loopback txn *****/\n");
	XAie_GetNocDmaAxiMmOutstandingTxn(DevInst, tile_loc, &noc_sts);
	printf("status of aximm outstanding txn :%u\n\n", noc_sts);

	do {
		XAie_DmaGetPendingBdCount(DevInst, tile_loc, 1, DMA_S2MM,
				&PendingBd);
	} while(PendingBd != 0);

	return 0;
}

int test_aie_noc_aximmoutstanding_txn(XAie_DevInst *DevInst)
{
	int col = 0;
	u32 noc_sts;
	uint32_t *src, *dest;
	XAie_MemInst *in, *out;
	XAie_LocType tile_loc = XAie_TileLoc(col, 0);

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

	noc_sts = -1;
	printf("/****** Before starting AxiMm Oustanding "
			"loopback transaction ******/\n");
	XAie_GetNocDmaAxiMmOutstandingTxn(DevInst, tile_loc, &noc_sts);
	printf("status of aximm outstanding txn :%u\n\n", noc_sts);

	/****** calling AxiMmOutstanding loopback transaction ******/
	printf("aximm outstanding loopback for column %d\n", col);
	xaie_aximm_outstanding_loopback((uint64_t )src, (uint64_t)dest,
			SIZE * sizeof(int), col, DevInst);

	noc_sts = -1;
	printf("/****** After completion aximm outstanding "
			"loopback transaction ******/\n");
	XAie_GetNocDmaAxiMmOutstandingTxn(DevInst, tile_loc, &noc_sts);
	printf("status of aximm outstanding txn :%u\n\n", noc_sts);

	XAie_MemSyncForCPU(in);
	XAie_MemSyncForCPU(out);

	XAie_MemFree(in);
	XAie_MemFree(out);

	printf("AIE noc_aximm_outstanding_txn success \n");
	return 0;
}

#else
int test_aie_uc_aximmoutstanding_txn(XAie_DevInst *DevInst)
{
	return -1;
}

int test_aie_noc_aximmoutstanding_txn(XAie_DevInst *DevInst)
{
	return -1;
}
#endif
