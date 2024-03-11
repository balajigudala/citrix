/******************************************************************************
* Copyright (C) 2019 - 2020 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

/*****************************************************************************/
/**
* @file xaie_load_ucelf.c
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

#ifndef _UCFW_
int test_aie_load_uc_elf(XAie_DevInst *DevInst){
	return 0;
}
#else
#include "../hw_config.h"
#include "ucfirmware/ucheader.h"
/************************** Constant Definitions *****************************/
extern const char _binary___app_elf_start[];
extern const char _binary___app_elf_end[];

/************************** Constant Definitions *****************************/
int test_aie_load_uc_elf(XAie_DevInst *DevInst)
{
	AieRC RC = XAIE_OK;
	XAie_LocType Loc = {0,0};
	u8 TileType = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
	uint32_t shared_mem_base_addr = DevInst->DevProp.DevMod[TileType].UcMod->DataMemAddr;
	uint32_t smvar1 = 0,smvar2 = 0;
	uint32_t addr = _XAie_GetTileAddr(DevInst, Loc.Row, Loc.Col) + shared_mem_base_addr;
	XAie_Write32(DevInst, addr, smvar1);
	XAie_Read32(DevInst, addr, &smvar1);
#if DEVICE == 0
	sleep(1);
#endif
	printf("before uc write the value is %x in %x\n", smvar1, shared_mem_base_addr);
	void *ElfMem = _binary___app_elf_start;
	if(ElfMem == NULL) {
		printf("Error load the UC firmware data section.\n");
		return -1;
	} else {
		printf("Elf file read from _binary___app_elf_start %p.\n", _binary___app_elf_start);
	}
	RC = XAie_LoadUcMem(DevInst, Loc,ElfMem);
	printf("Load UC elf.\n");
	
	const XAie_UcMod *UcMod = DevInst->DevProp.DevMod[TileType].UcMod;
	u32 status1, status2;
	UcMod->GetCoreStatus(DevInst, Loc, &status1,UcMod );
	printf("Get core status.%x before Wakeup\n", status1);
	UcMod->Wakeup(DevInst, Loc, UcMod);
	UcMod->GetCoreStatus(DevInst, Loc, &status2,UcMod );
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
	printf("uc load success!\n");
	return XAIE_OK;
}
#endif
