/******************************************************************************
* Copyright (C) 2023 AMD, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/


/*****************************************************************************/
/**
* @file xaie_lite_shim_aie2ps.h
* @{
*
* This header file defines a lite shim interface for AIE2PS type devices.
*
******************************************************************************/
#ifndef XAIE_LITE_SHIM_AIE2PS_H
#define XAIE_LITE_SHIM_AIE2PS_H

/***************************** Include Files *********************************/
#include "xaie_lite_hwcfg.h"
#include "xaiegbl_defs.h"
#include "xaiegbl.h"

/************************** Constant Definitions *****************************/
#define UPDT_NEXT_NOC_TILE_LOC(Loc)	(Loc).Col++
#define IS_TILE_NOC_TILE(Loc)		1
#define XAIE_MAX_NUM_NOC_INTR		4U

/************************** Function Prototypes  *****************************/
/*****************************************************************************/
/**
*
* This is API returns the shim tile type for a given device instance and tile
* location.
*
* @param	DevInst: Device Instance
* @param	Loc: Location of the AIE tile.
*
* @return	TileType SHIMPL/SHIMNOC.
*
* @note		Internal only.
*
******************************************************************************/
static inline u8 _XAie_LGetShimTTypefromLoc(XAie_DevInst *DevInst,
			XAie_LocType Loc)
{
	(void) DevInst;
	(void) Loc;

	return XAIEGBL_TILE_TYPE_SHIMNOC;
}

/*****************************************************************************/
/**
*
* This API maps L2 status bit to its L1 switch.
*
* @param	DevInst: Device Instance.
* @param	Index: Set bit position in L2 status.
* @param	L2Col: Location of L2 column.
* @param	L1Col: Mapped value of L1 column.
* @param	Switch: Broadcast switch.
*
* @return	None.
*
* @note		Internal only.
*
******************************************************************************/
static inline void _XAie_MapL2MaskToL1(XAie_DevInst *DevInst, u32 Index,
			u8 L2Col, u8 *L1Col, XAie_BroadcastSw *Switch)
{
	(void) DevInst;
	(void) L2Col;

	*L1Col = L2Col;
	*Switch= Index;
}

/*****************************************************************************/
/**
*
* This is API returns the range of columns programmed to generate interrupt on
* the given IRQ channel.
*
* @param	IrqId: L2 IRQ ID.
*
* @return	Range of columns.
*
* @note		Internal only.
*
******************************************************************************/
static inline XAie_Range _XAie_MapIrqIdToCols(u8 IrqId)
{
	XAie_Range _MapIrqIdToCols[] = {
		{.Start = 0, .Num = 5},
		{.Start = 1, .Num = 5},
		{.Start = 2, .Num = 5},
		{.Start = 3, .Num = 5},
	};

	return _MapIrqIdToCols[IrqId];
}

/*****************************************************************************/
/**
*
* This is API returns the L2 IRQ ID for a given column.
*
* @param	DevInst: Device Instance
* @param	Loc: Location of the AIE tile.
*
* @return	L2 IRQ ID.
*
* @note		Internal only.
*
******************************************************************************/
static inline u8 _XAie_MapColToIrqId(XAie_DevInst *DevInst, XAie_LocType Loc)
{
	u8 AbsCol = DevInst->StartCol + Loc.Col;

	return AbsCol / (XAIE_NUM_COLS / XAIE_MAX_NUM_NOC_INTR);
}

/*****************************************************************************/
/**
* This API modifies(enable or disable) the clock control register for given shim.
*
* @param        DevInst: Device Instance
* @param        Loc: Location of AIE SHIM tile
* @param        Enable: XAIE_ENABLE to enable shim clock buffer,
*                       XAIE_DISABLE to disable.

* @note         It is internal function to this file
*
******************************************************************************/
static inline void _XAie_PrivilegeSetShimClk(XAie_DevInst *DevInst,
					     XAie_LocType Loc, u8 Enable)
{
	u64 RegAddr;
	u32 FldVal;

	RegAddr = _XAie_LGetTileAddr(Loc.Row, Loc.Col) +
		XAIE_SHIM_TILE_MOD_CLOCK_CONTROL_0_REGOFF;

	FldVal = XAie_SetField(Enable,
			XAIE_SHIM_TILE_MOD_CLOCK_CONTROL_0_CTE_CLOCK_ENABLE_LSB,
			XAIE_SHIM_TILE_MOD_CLOCK_CONTROL_0_CTE_CLOCK_ENABLE_MASK);
	/* TODO: UCONTROLLER here?
	FldVal |= XAie_SetField(Enable,
			XAIE2PSGBL_PL_MODULE_MODULE_CLOCK_CONTROL_0_UCONTROLLER_CLOCK_ENABLE_LSB,
			XAIE2PSGBL_PL_MODULE_MODULE_CLOCK_CONTROL_0_UCONTROLLER_CLOCK_ENABLE_MASK);
	*/
	FldVal |= XAie_SetField(Enable,
			XAIE_SHIM_TILE_MOD_CLOCK_CONTROL_0_PL_INTERFACE_CLOCK_ENABLE_LSB,
			XAIE_SHIM_TILE_MOD_CLOCK_CONTROL_0_PL_INTERFACE_CLOCK_ENABLE_MASK);
	FldVal |= XAie_SetField(Enable,
			XAIE_SHIM_TILE_MOD_CLOCK_CONTROL_0_STREAM_SWITCH_CLOCK_ENABLE_LSB,
			XAIE_SHIM_TILE_MOD_CLOCK_CONTROL_0_STREAM_SWITCH_CLOCK_ENABLE_MASK);

	_XAie_LPartMaskWrite32(DevInst, RegAddr,
		XAIE_SHIM_TILE_MOD_CLOCK_CONTROL_0_MASK, FldVal);

	RegAddr = _XAie_LGetTileAddr(Loc.Row, Loc.Col) +
		XAIE_SHIM_TILE_MOD_CLOCK_CONTROL_1_REGOFF;

	FldVal = XAie_SetField(Enable,
			XAIE2PSGBL_PL_MODULE_MODULE_CLOCK_CONTROL_1_NOC_MODULE_1_CLOCK_ENABLE_LSB,
			XAIE2PSGBL_PL_MODULE_MODULE_CLOCK_CONTROL_1_NOC_MODULE_1_CLOCK_ENABLE_MASK);

	FldVal |= XAie_SetField(Enable,
			XAIE2PSGBL_PL_MODULE_MODULE_CLOCK_CONTROL_1_NOC_MODULE_0_CLOCK_ENABLE_LSB,
			XAIE2PSGBL_PL_MODULE_MODULE_CLOCK_CONTROL_1_NOC_MODULE_0_CLOCK_ENABLE_MASK);

	_XAie_LPartMaskWrite32(DevInst, RegAddr,
		XAIE_SHIM_TILE_MOD_CLOCK_CONTROL_1_MASK, FldVal);

}

/*****************************************************************************/
/**
* This API puts the uc-core to sleep in the given column.
*
* @param        DevInst: Device Instance
* @param        Loc: Location of AIE SHIM tile
*
* @note         Internal API only.
*
******************************************************************************/
static inline void _XAie_UCCoreSleep(XAie_DevInst *DevInst,
					  XAie_LocType Loc)
{
	u64 RegAddr;
	u32 FldVal;

	RegAddr = _XAie_LGetTileAddr(Loc.Row, Loc.Col) +
		XAIE_SHIM_TILE_UC_MODULE_CORE_CONTROL;

	_XAie_LPartMaskWrite32(DevInst, RegAddr,
			XAIE_SHIM_TILE_UC_MODULE_CORE_CONTROL_GO_TO_SLEEP_MASK,
			XAIE_SHIM_TILE_UC_MODULE_CORE_CONTROL_GO_TO_SLEEP_MASK);

}

/*****************************************************************************/
/**
* This API wakes up the uc-core in the given column.
*
* @param        DevInst: Device Instance
* @param        Loc: Location of AIE SHIM tile
*
* @note         Internal API only.
*
******************************************************************************/
static inline void _XAie_UCCoreWakeUp(XAie_DevInst *DevInst,
					  XAie_LocType Loc)
{
	u64 RegAddr;
	u32 FldVal;

	RegAddr = _XAie_LGetTileAddr(Loc.Row, Loc.Col) +
		XAIE_SHIM_TILE_UC_MODULE_CORE_CONTROL;

	_XAie_LPartMaskWrite32(DevInst, RegAddr,
			XAIE_SHIM_TILE_UC_MODULE_CORE_CONTROL_WAKEUP_MASK,
			XAIE_SHIM_TILE_UC_MODULE_CORE_CONTROL_WAKEUP_MASK);

}

/*****************************************************************************/
/**
* This API puts all the uc-cores to sleep in the partition.
*
* @param        DevInst: Device Instance.
*
* @return	None.
*
* @note         None.
*
******************************************************************************/
static inline void XAie_LPartUCCoreSleep(XAie_DevInst *DevInst)
{
	XAie_LocType Loc;

	Loc.Row = 0U;
	for(Loc.Col = 0U; Loc.Col < DevInst->NumCols; Loc.Col++) {
		_XAie_UCCoreSleep(DevInst, Loc);
	}
}

/*****************************************************************************/
/**
* This API wakes all the uc-cores up in the partition.
*
* @param        DevInst: Device Instance.
*
* @return	None.
*
* @note         None.
*
******************************************************************************/
static inline void XAie_LPartUCCoreWakeUp(XAie_DevInst *DevInst)
{
	XAie_LocType Loc;

	Loc.Row = 0U;
	for(Loc.Col = 0U; Loc.Col < DevInst->NumCols; Loc.Col++) {
		_XAie_UCCoreWakeUp(DevInst, Loc);
	}
}

/*****************************************************************************/
/**
* This API modifies(enable or disable) the uc memory_privileged register for all
* cols in the partition.
*
* @param        DevInst: Device Instance
* @param        Enable: XAIE_ENABLE to enable the uc memory_privileged,
*                       XAIE_DISABLE to disable.

* @note         Modifying the uc.memory_privileged register while the uc core is
* 		enabled results in undefined behaviour.
* 		It is job of the caller to make sure the UC core is in sleep.
*
******************************************************************************/
static inline void _XAie_PrivilegeSetUCMemoryPrivileged(XAie_DevInst *DevInst,
							u8 Enable)
{
	u64 RegAddr;
	u32 Val;
	int i;

	for (i = 0; i < DevInst->NumCols; i++) {
		RegAddr = _XAie_LGetTileAddr(0, i) +
			  XAIE2PSGBL_UC_MODULE_MEMORY_PRIVILEGED;
		Val = XAie_SetField(Enable,
				    XAIE2PSGBL_UC_MODULE_MEMORY_PRIVILEGED_MEMORY_PRIVILEGED_LSB,
				    XAIE2PSGBL_UC_MODULE_MEMORY_PRIVILEGED_MEMORY_PRIVILEGED_MASK);
		_XAie_LPartMaskWrite32(DevInst, RegAddr,
				       XAIE2PSGBL_UC_MODULE_MEMORY_PRIVILEGED_MEMORY_PRIVILEGED_MASK,
				       Val);
	}
}

/*****************************************************************************/
/**
* This API initializes the hw_error config for all cols in the partition.
*
* @param        DevInst: Device Instance
*
******************************************************************************/
static void _XAie_PrivilegeSetHWErrIrq(XAie_DevInst *DevInst)
{
	int i;
	u64 TileAddr;
	u64 RegAddr;
	u32 Val;
	XAie_LocType Loc = {0, 0};

	for (i = 0; i < DevInst->NumCols; i++) {
		Loc.Col = i;
		TileAddr = _XAie_LGetTileAddr(0, i);
		RegAddr = TileAddr + XAIE2PSGBL_PL_MODULE_INTERRUPT_CONTROLLER_HW_ERROR_MASK;
		_XAie_LPartWrite32(DevInst, RegAddr, 0);

		RegAddr = TileAddr + XAIE2PSGBL_PL_MODULE_INTERRUPT_CONTROLLER_HW_ERROR_INTERRUPT;
		Val = _XAie_MapColToIrqId(DevInst, Loc);
		_XAie_LPartWrite32(DevInst, RegAddr, Val);
	}
}

#endif		/* end of protection macro */
/** @} */
