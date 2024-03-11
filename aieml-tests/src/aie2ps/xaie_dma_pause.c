/******************************************************************************
 * Copyright (C) 2021 Xilinx, Inc.  All rights reserved.
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

/*****************************************************************************/
/**
 * @file xaie_dma_pause.c
 * @{
 *
 * This file contains the test application to check UC and NOC dma pause.
 *
 * This application checks the NOC & UC dma pausing and Unpausing by running
 * on the Dmaloop back.
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
void *app_elf_start_dmapause = _binary___app_elf_start;
#else
#define ELF_LOAD_START 0x2000000
void *app_elf_start_dmapause = ELF_LOAD_START;
#endif

#define DMA_PAUSE                 1
#define DMA_UNPAUSE               0
#define DMA_CHANNEL_MAX           2
#define UC_MODULE__DMA_DIR__DM2MM 0
#define UC_MODULE__DMA_DIR__MM2DM 1
#define NOC_MODULE__DMA_DIR__S2MM 0
#define NOC_MODULE__DMA_DIR__MM2S 1
#define NOC_MODULE__DMA_DIR__MAX  2
#define SIZE 512

/************************* UcDma_pause ***************************************/
#if AIE_GEN == 5
int test_aie_ucdma_pause(XAie_DevInst *DevInst)
{
	AieRC RC = XAIE_OK;
	XAie_LocType Loc =  {0, 0};

	/******** pausing ucdma *********/
	printf("/****** Pausing Ucdma ******/\n");
	RC = XAie_UcDmaPause(DevInst, &Loc, UC_MODULE__DMA_DIR__DM2MM,
			DMA_PAUSE);
	if (RC != XAIE_OK) {
		printf("\n/****** Pause UcDma failed ****** /\n\n");
		return XAIE_ERR;
	}

	u8 TileType = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
	uint32_t shared_mem_base_addr = DevInst->DevProp.DevMod[TileType].UcMod->DataMemAddr;
	uint32_t smvar1 = 0, smvar2 = 0;
	uint32_t addr = _XAie_GetTileAddr(DevInst, Loc.Row, Loc.Col) + shared_mem_base_addr;

	XAie_Write32(DevInst, addr, smvar1);
	XAie_Read32(DevInst, addr, &smvar1);

	void *ElfMem = app_elf_start_dmapause;
	if(ElfMem == NULL) {
		printf("Error load the UC firmware data section.\n");
		return -1;
	}

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
		printf("uc write value %x in %x\n", smvar2, shared_mem_base_addr);
		sleep(1);
	}

	if (smvar2 != MAGIC_DATA){
		printf("timeout the UC load failed\n");
		return -1;
	}

	sleep(5);
	printf("/****** Unpausing Ucdma ******/\n");
	/*********** unpause ucdma **********/
        RC = XAie_UcDmaPause(DevInst, &Loc, UC_MODULE__DMA_DIR__DM2MM,
			DMA_UNPAUSE);
	if (RC != XAIE_OK) {
		printf("\n/****** Unpause UcDma failed ******/\n\n");
		return XAIE_ERR;
	}

	printf("test UcDmaPause success\n");
	return 0;
}

int test_aie_nocdma_pause(XAie_DevInst *DevInst)
{
	AieRC RC;
	uint32_t *src, *dest;
	XAie_MemInst *in, *out;
	int Channel, Dir, col = 0;
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

	/******* calling loopback ***********/
	printf("/****** Dma loopback started ******/\n");
	xaie_shimdma_loopback((uint64_t )src, (uint64_t)dest,
			SIZE * sizeof(int), col, DevInst);

	/******* check loop back success *******/
	for(int i = 0; i < SIZE; i++) {
		if(src[i] != dest[i]) {
			printf("Test failed at index %d for column %d\n", i, col);
			return -1;
		}
	}

	printf("Dma loopback passed without dmapausing\n");
	/******* change src data and clear dest data ******/
	for(int i = 0; i < SIZE; i++) {
		src[i] = i * (col + 1);
		dest[i] = 0x0;
	}
	/************* pausing Nocdma ******************/
	printf("\n/******* Pausing NocDma ********/\n");
	for(int Channel = 0; Channel < DMA_CHANNEL_MAX; Channel++) {
		for(int Dir = DMA_S2MM; Dir < DMA_MAX; Dir++) {
			RC = XAie_NocDmaPause(DevInst, &tile_loc, Channel, Dir, DMA_PAUSE);
			if (RC != XAIE_OK) {
				printf("\nNocDmapause failed \n\n");

				return -1;
			}
		}
	}

	printf("/****** After pausing, Dma loopback started ******/\n");
	sleep(2);
	/******* calling loopback ***********/
	xaie_shimdma_loopback((uint64_t )src, (uint64_t)dest,
			SIZE * sizeof(int), col, DevInst);

	/******* check loop back success *******/
	for(int i = 0; i < SIZE; i++) {
		if(src[i] != dest[i]) {
			printf("Test failed at index %d for column %d\n", i, col);
			return -1;
		}
	}
	printf("Dma loopback is failed after dmapausing\n");

	/*********** unpausing the Nocdma ************/
	printf("/****** Unpausing NocDma ******/\n");
	for(int Channel = 0; Channel < DMA_CHANNEL_MAX; Channel++) {
		for(int Dir = DMA_S2MM; Dir < DMA_MAX; Dir++) {
			RC = XAie_NocDmaPause(DevInst, &tile_loc, Channel, Dir, DMA_UNPAUSE);
			if (RC != XAIE_OK) {
				printf("\n/****** NocDmapause failed ******/\n\n");
				return XAIE_ERR;
			}
		}
	}

	XAie_MemSyncForCPU(in);
	XAie_MemSyncForCPU(out);
	XAie_MemFree(in);
	XAie_MemFree(out);

	printf("test NocDmaPause success\n");
	return 0;
}
#else
int test_aie_ucdma_pause(XAie_DevInst *DevInst)
{
	return -1;
}

int test_aie_nocdma_pause(XAie_DevInst *DevInst)
{
	return -1;
}
#endif
