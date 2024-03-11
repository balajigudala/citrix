/******************************************************************************
* Copyright (C) 2023 Advanced Micro Devices.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

/*****************************************************************************/
/**
* @file xaie_col_status.c
* @{
*
* This file contains the test application for the column status api.
*
* This application simply reads the status registers of the AIE array
* and prints the values.
*
******************************************************************************/

/***************************** Include Files *********************************/
#include "hw_config.h"

#include <stdlib.h>
#include <string.h>
#include <xaiengine.h>
#include <malloc.h>
#include <stdbool.h>
#include "xaiengine/xaie_helper.h"


/************************** Constant Definitions *****************************/
#define NUM_COLS XAIE_NUM_COLS
/************************** Function Definitions *****************************/
/* Lock values used for synchronization */
#define LOCK_FOR_WRITE	0
#define LOCK_FOR_READ	1

/* Input and output address in aie data memory */
#define DATA_MEM_INPUT_ADDR	0x4000
#define DATA_MEM_OUTPUT_ADDR	0x3000

#define NUM_ELEMS 32

static uint32_t data[NUM_ELEMS];
static uint32_t buffer[NUM_ELEMS];

#define MPOOL_POOL_SIZ   (64 * 1024)
#define MPOOL_ALIGN_SIZE (8)
#define MPOOL_FREE(p)                                   \
    do {                                                \
        if (p != NULL) {                                \
            free(p);                                    \
            (p) = NULL;                                 \
        }                                               \
    } while(false)

/************************** Memory Pool Structure *****************************/
typedef struct XAie_MpoolPool {
    void                *pool;     // memory pool field
    struct XAie_MpoolPool *next;     // next memory pool's pointer
} XAie_MpoolPool;

typedef struct XAie_Mpool {
    XAie_MpoolPool *head;       // memory pool's head
    void         *begin;      // data for internal conduct
    size_t        usiz;       // used pool size of current pool
    size_t        msiz;       // max pool size of current pool
    XAie_MpoolPool *mpool;      // memory pool
} XAie_Mpool;

XAie_Mpool *XAie_MpoolCreate (size_t siz);
void *XAie_MpoolAlloc(size_t siz, XAie_Mpool *pool);
void XAie_MpoolDestroy (XAie_Mpool *pool);

/************************** Private Function *****************************/
static inline bool XAie_MpoolExtend(XAie_MpoolPool *p, size_t siz, XAie_Mpool *pool);
static inline size_t XAie_MpoolAlign(size_t siz);
static inline size_t XAie_MpoolDecideCreateSiz(size_t siz);


/*****************************************************************************/
/**
*
* This API Extends the memory pool size
*
* @param        size - Required extra number of memory pool bytes.
*
* @return       Return aligned memory
* @note         None.
*
*******************************************************************************/
static inline bool XAie_MpoolExtend(XAie_MpoolPool *p, size_t siz, XAie_Mpool *pool) {
	siz = XAie_MpoolDecideCreateSiz(siz);
	XAie_MpoolPool *pp;

	pp = malloc(sizeof(*pp));
	if (pp == NULL) {
		return false;
	}

	pp->pool = malloc(siz);
	if (pp->pool == NULL) {
		return false;
	}

	memset(pp->pool, 0, siz);
	pp->next = NULL;
	p->next = pp;
	pool->begin = pp->pool;

	return true;
}

/*****************************************************************************/
/**
*
* This API Returns Allign memory
*
* @param        size - Required number of bytes
*
* @return       Return aligned memory
* @note         None.
*
*******************************************************************************/
static inline size_t XAie_MpoolAlign(size_t siz) {
	return (siz + (MPOOL_ALIGN_SIZE - 1)) & ~(MPOOL_ALIGN_SIZE - 1);
}

/*****************************************************************************/
/**
*
* This API Decides memory pool size
*
* @param        size - Required number of bytes
*
* @return       Return size of memory pool
* @note         None.
*
*******************************************************************************/
static inline size_t XAie_MpoolDecideCreateSiz(size_t siz) {
	return siz <= 0 ? MPOOL_POOL_SIZ : XAie_MpoolAlign(siz);
}

/*****************************************************************************/
/**
*
* This API Creates Memory Pool.
*
* @param        size - Required number of bytes
*
* @return       Return pointer to Memory Pool
* @note         None.
*
*******************************************************************************/
XAie_Mpool *XAie_MpoolCreate (size_t siz) {
	XAie_Mpool *pool;
	siz = XAie_MpoolDecideCreateSiz(siz);

	pool = malloc(sizeof(*pool));
	if (pool == NULL) {
		return NULL;
	}

	pool->mpool = malloc(sizeof(*pool->mpool));
	if (pool->mpool == NULL) {
		return NULL;
	}

	pool->mpool->pool = malloc(siz);
	if (pool->mpool->pool == NULL) {
		return NULL;
	}

	pool->mpool->next = NULL;

	pool->begin = pool->mpool->pool;
	pool->head  = pool->mpool;
	pool->usiz  = 0;
	pool->msiz  = siz;

	return pool;
}

/*****************************************************************************/
/**
*
* This API Allocate Memory from Memory pool
*
* @param        pool - Memory pool Instance.
* 		size - Required number of bytes
*
* @return       Return pointer to allocated memory.
* @note         None.
*
*******************************************************************************/
void *XAie_MpoolAlloc(size_t siz, XAie_Mpool *pool) {
	XAie_MpoolPool **p = &pool->mpool;
	XAie_MpoolPool *pp = *p;
	size_t usiz = XAie_MpoolAlign(pool->usiz + siz);
	size_t msiz = pool->msiz;
	void     *d = pool->begin;
	if (usiz > msiz) {
		if (!XAie_MpoolExtend(pp, usiz * 2 + 1, pool)) {
			return NULL;
		}
		pool->usiz = 0;
		pool->msiz = usiz * 2;
		d = pool->begin;
		pool->begin += XAie_MpoolAlign(siz);
		*p = pp->next;

		printf("pp = %p\n",pp);
	} else {
		pool->usiz = usiz;
		pool->begin += XAie_MpoolAlign(siz);
	}
	return d;
}

/*****************************************************************************/
/**
*
* This API Release all memory pool
*
* @param        pool - Memory pool Instance.
*
* @note         None.
*
*******************************************************************************/
void XAie_MpoolDestroy (XAie_Mpool *pool) {
	XAie_MpoolPool *cur, *next;

	cur = pool->head;
	for (cur = pool->head; cur; cur = next){
		next = cur->next;

		MPOOL_FREE(cur->pool);
		MPOOL_FREE(cur);
		cur = next;
	}

	MPOOL_FREE(pool);
}

/*****************************************************************************/
/**
*
* This API manipulates AIE driver Memory tile Dma and lock to check the
* difference in the respective register status.
*
* @param	DevInst - Device Instance.
* 		status - Column status structure.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_aie_mem_tile_col_status(XAie_DevInst *DevInst,XAie_ColStatus* status)
{
	AieRC RC = XAIE_OK;
	uint32_t data[NUM_ELEMS];
	uint32_t S2MMStatus_1, S2MMStatus_2, MM2SStatus_1, MM2SStatus_2;
	uint8_t LockValue_1, LockValue_2;
	XAie_Lock Lock;

	XAie_LocType Tile_1, Tile_2, Tile_3, Tile_4, AieTile_2, AieTile_3;
	XAie_DmaDesc Tile_3_S2MM, Tile_2_MM2S;
	// make the location work for different aie gen and layout
	const uint8_t aie_tile_first_row = XAIE_AIE_TILE_ROW_START;
	const uint8_t mtile_last_row = aie_tile_first_row - 1;
	const uint8_t mtile_last_row_idx = mtile_last_row - XAIE_MEM_TILE_ROW_START;

	Tile_1 = XAie_TileLoc(0, mtile_last_row);
	Tile_2 = XAie_TileLoc(1, mtile_last_row);
	Tile_3 = XAie_TileLoc(2, mtile_last_row);
	Tile_4 = XAie_TileLoc(3, mtile_last_row);
	AieTile_2 = XAie_TileLoc(1, aie_tile_first_row);
	AieTile_3 = XAie_TileLoc(2, aie_tile_first_row);

	Lock.LockId = 5;
	Lock.LockVal = 0;
	RC |= XAie_LockSetValue(DevInst, Tile_4, Lock);

	/* Initialize array with random integers */
	for(uint8_t i = 0U; i < NUM_ELEMS; i++) {
		data[i] = rand() % 127;
	}

	XAie_StatusDump(DevInst, status);
	S2MMStatus_1 =  status[2].MemTile[mtile_last_row_idx].Dma[0].S2MMStatus;
	MM2SStatus_1 =  status[1].MemTile[mtile_last_row_idx].Dma[0].MM2SStatus;
	LockValue_1 = status[3].MemTile[mtile_last_row_idx].LockValue[5];

	printf("Mem Tile: Before Processing\n");
	printf("Dma Status Register (S2MM): 0x%x\n", S2MMStatus_1);
	printf("Dma Status Register(MM2S): 0x%x\n", MM2SStatus_1);
	printf("Lock Value: 0x%x\n", LockValue_1);

	Lock.LockId = 5;
	Lock.LockVal = 1;
	RC |= XAie_LockSetValue(DevInst, Tile_4, Lock);

	/* Write data to aie tile data memory */
	RC = XAie_DataMemBlockWrite(DevInst, Tile_2, DATA_MEM_INPUT_ADDR,
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

	/* Acessing West tile memory does not need any offset */
	RC |= XAie_DmaSetAddrLen(&Tile_2_MM2S, DATA_MEM_INPUT_ADDR,
			NUM_ELEMS * sizeof(uint32_t));
	/* Accessing East tile memory requires an offset of 0x100000 */
	RC |= XAie_DmaSetAddrLen(&Tile_3_S2MM, DATA_MEM_OUTPUT_ADDR,
			NUM_ELEMS * sizeof(uint32_t));

	RC |= XAie_DmaEnableBd(&Tile_2_MM2S);
	RC |= XAie_DmaEnableBd(&Tile_3_S2MM);
	RC |= XAie_DmaWriteBd(DevInst, &Tile_2_MM2S, Tile_2, 0U);

	RC |= XAie_DmaWriteBd(DevInst, &Tile_3_S2MM, Tile_3, 0U);

	RC |= XAie_DmaChannelPushBdToQueue(DevInst, Tile_2, 0U, DMA_MM2S, 0U);
	RC |= XAie_DmaChannelPushBdToQueue(DevInst, Tile_2, 0U, DMA_MM2S, 2U);

	RC |= XAie_DmaChannelPushBdToQueue(DevInst, Tile_3, 0U, DMA_S2MM, 0U);
	RC |= XAie_DmaChannelPushBdToQueue(DevInst, Tile_3, 0U, DMA_S2MM, 1U);


	XAie_StatusDump(DevInst, status);
	S2MMStatus_2 =  status[2].MemTile[mtile_last_row_idx].Dma[0].S2MMStatus;
	MM2SStatus_2 =  status[1].MemTile[mtile_last_row_idx].Dma[0].MM2SStatus;
	LockValue_2 = status[3].MemTile[mtile_last_row_idx].LockValue[5];

	printf("Mem Tile: After Processing\n");
	printf("Dma Status Register(S2MM): 0x%x\n", S2MMStatus_2);
	printf("Dma Status Register(MM2S): 0x%x\n", MM2SStatus_2);
	printf("Lock Value: 0x%x\n", LockValue_2);

	if(S2MMStatus_1 != S2MMStatus_2 && MM2SStatus_1 != MM2SStatus_2 &&
		       	LockValue_1 != LockValue_2) {
		return XAIE_OK;
	}
	else {
		return XAIE_ERR;
	}
}


/*****************************************************************************/
/**
*
* This API manipulates AIE driver core tile core module,  Dma and lock to check
* the difference in the respective register status.
*
* @param	DevInst - Device Instance.
* 		status - Column status structure.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_aie_core_tile_col_status(XAie_DevInst *DevInst, XAie_ColStatus* status)
{
        AieRC RC = XAIE_OK;
        uint32_t data[NUM_ELEMS];
        XAie_LocType Tile_1, Tile_2;
        XAie_DmaDesc Tile_1_MM2S, Tile_1_S2MM, Tile_2_MM2S, Tile_2_S2MM;
        uint32_t S2MMStatus_1, S2MMStatus_2, MM2SStatus_1, MM2SStatus_2,
                 CoreStatus_1, CoreStatus_2;
        uint8_t LockValue_1, LockValue_2;

        Tile_1 = XAie_TileLoc(1, XAIE_AIE_TILE_ROW_START);
        Tile_2 = XAie_TileLoc(1, XAIE_AIE_TILE_ROW_START+1);

        XAie_Lock Lock;
        Lock.LockId = 5;
        Lock.LockVal = 0;
        RC |= XAie_LockSetValue(DevInst, Tile_1, Lock);


        XAie_StatusDump(DevInst, status);
        S2MMStatus_1 = status[1].CoreTile[1].Dma[0].S2MMStatus;
        MM2SStatus_1 = status[1].CoreTile[1].Dma[0].MM2SStatus;
        CoreStatus_1 = status[1].CoreTile[0].CoreStatus;
        LockValue_1 = status[1].CoreTile[0].LockValue[5];
        printf("Core Tile: Before Processing\n");
        printf("Dma Status Register(S2MM): 0x%x\n", S2MMStatus_1);
        printf("Dma Status Register(MM2S): 0x%x\n", MM2SStatus_1);
        printf("Core Status Register: 0x%x\n", CoreStatus_1);
        printf("Lock Value: 0x%x\n", LockValue_1);


        XAie_CoreEnable(DevInst, Tile_1);

        //XAie_Lock Lock;
        Lock.LockId = 5;
        Lock.LockVal = 1;
        RC |= XAie_LockSetValue(DevInst, Tile_1, Lock);

        /* Initialize array with random integers */
        for(uint8_t i = 0U; i < NUM_ELEMS; i++) {
                data[i] = rand() % 127;
        }

        /* Write data to aie tile data memory */
        RC = XAie_DataMemBlockWrite(DevInst, Tile_1, DATA_MEM_INPUT_ADDR,
                        (void *)data, sizeof(uint32_t) * NUM_ELEMS);
        if(RC != XAIE_OK) {
                printf("Writing data to aie data memory failed.\n");
                return -1;
        }

        /* Initialize software descriptors for aie Dma */
        RC |= XAie_DmaDescInit(DevInst, &Tile_1_MM2S, Tile_1);
        RC |= XAie_DmaDescInit(DevInst, &Tile_1_S2MM, Tile_1);
        RC |= XAie_DmaDescInit(DevInst, &Tile_2_MM2S, Tile_2);
        RC |= XAie_DmaDescInit(DevInst, &Tile_2_S2MM, Tile_2);

        /* Configure address and length in Dma software descriptors */
        RC |= XAie_DmaSetAddrLen(&Tile_1_MM2S, DATA_MEM_INPUT_ADDR,
                        NUM_ELEMS * sizeof(uint32_t));
        RC |= XAie_DmaSetAddrLen(&Tile_1_S2MM, DATA_MEM_OUTPUT_ADDR,
                        NUM_ELEMS * sizeof(uint32_t));
        RC |= XAie_DmaSetAddrLen(&Tile_2_MM2S, DATA_MEM_INPUT_ADDR,
                        NUM_ELEMS * sizeof(uint32_t));
        RC |= XAie_DmaSetAddrLen(&Tile_2_S2MM, DATA_MEM_INPUT_ADDR,
                        NUM_ELEMS * sizeof(uint32_t));

        /*
         * Configure locks for Dma channel synchronization. S2MM should run
         * before MM2S on Tile_2. Lock Id 5 is used for synchronization.
         */
        RC |= XAie_DmaSetLock(&Tile_2_S2MM, XAie_LockInit(7U, LOCK_FOR_WRITE),
                        XAie_LockInit(7, LOCK_FOR_READ));
        RC |= XAie_DmaSetLock(&Tile_2_MM2S, XAie_LockInit(7U, LOCK_FOR_READ),
                        XAie_LockInit(7, LOCK_FOR_WRITE));
        /*
         * Configure locks for Dma channel synchronization. MM2S should run
         * before S2MM on Tile_1. Lock Id 6 is used for synchronization.
         */
        RC |= XAie_DmaSetLock(&Tile_1_MM2S, XAie_LockInit(8U, LOCK_FOR_WRITE),
                        XAie_LockInit(8U, LOCK_FOR_READ));
        RC |= XAie_DmaSetLock(&Tile_1_S2MM, XAie_LockInit(8U, LOCK_FOR_READ),
                        XAie_LockInit(8U, LOCK_FOR_WRITE));

        /* Enable the buffer descriptors in software Dma descriptors */
        RC |= XAie_DmaEnableBd(&Tile_1_MM2S);
        RC |= XAie_DmaEnableBd(&Tile_1_S2MM);
        RC |= XAie_DmaEnableBd(&Tile_2_MM2S);
        RC |= XAie_DmaEnableBd(&Tile_2_S2MM);
        if(RC != XAIE_OK) {
                printf("Failed to setup software Dma descriptors.\n");
                return -1;
        }

        /*
         * Configure aie Dma hardware using software descriptors. Use buffer
         * descriptor 1 for MM2S and 9 for S2MM on both tiles.
         */
        RC |= XAie_DmaWriteBd(DevInst, &Tile_1_MM2S, Tile_1, 1U);
        RC |= XAie_DmaWriteBd(DevInst, &Tile_1_S2MM, Tile_1, 9U);
        RC |= XAie_DmaWriteBd(DevInst, &Tile_2_MM2S, Tile_2, 1U);
        RC |= XAie_DmaWriteBd(DevInst, &Tile_2_S2MM, Tile_2, 9U);

        /* Push Bd numbers to aie Dma channel queues and enable the channels */
        RC |= XAie_DmaChannelPushBdToQueue(DevInst, Tile_1, 0U, DMA_MM2S, 1U);
        RC |= XAie_DmaChannelPushBdToQueue(DevInst, Tile_1, 0U, DMA_S2MM, 9U);
        RC |= XAie_DmaChannelPushBdToQueue(DevInst, Tile_2, 0U, DMA_MM2S, 1U);
        RC |= XAie_DmaChannelPushBdToQueue(DevInst, Tile_2, 0U, DMA_S2MM, 9U);

        XAie_StatusDump(DevInst, status);
        S2MMStatus_2 = status[1].CoreTile[1].Dma[0].S2MMStatus;
        MM2SStatus_2 = status[1].CoreTile[1].Dma[0].MM2SStatus;
        CoreStatus_2 = status[1].CoreTile[0].CoreStatus;
        LockValue_2 = status[1].CoreTile[0].LockValue[5];

        printf("Core Tile: After Processing\n");
        printf("Dma Status Register(S2MM): 0x%x\n", S2MMStatus_2);
        printf("Dma Status Register(MM2S): 0x%x\n", MM2SStatus_2);
        printf("Core Status Register: 0x%x\n", CoreStatus_2);
        printf("Lock Value: 0x%x\n", LockValue_2);

        if(S2MMStatus_1 != S2MMStatus_2 && MM2SStatus_1 != MM2SStatus_2 &&
                        LockValue_1 != LockValue_2 && CoreStatus_1 != CoreStatus_2) {
                return XAIE_OK;
        }
        else {
                return XAIE_ERR;
        }
}

/*****************************************************************************/
/**
*
* This API manipulates AIE driver shim tile Dma and lock to check the
* difference in the respective register status.
*
* @param	DevInst - Device Instance.
* 		status - Column status structure.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_aie_shim_tile_col_status(XAie_DevInst *DevInst, XAie_ColStatus *status)
{
	AieRC RC = XAIE_OK;
	XAie_LocType ShimDma;
	XAie_DmaDesc DmaDesc1, DmaDesc2;
	XAie_Lock Lock;
	uint32_t S2MMStatus_1, S2MMStatus_2, MM2SStatus_1, MM2SStatus_2;
	uint8_t LockValue_1, LockValue_2;
	ShimDma = XAie_TileLoc(2, 0);

	//clear the dma channel status and lock value
	Lock.LockId = 6;
	Lock.LockVal = 0;
	RC |= XAie_LockSetValue(DevInst, ShimDma, Lock);
	RC = XAie_DmaDescInit(DevInst, &DmaDesc1, ShimDma);
	RC = XAie_DmaDescInit(DevInst, &DmaDesc2, ShimDma);
	XAie_DmaChannelDisable(DevInst, ShimDma, 1, DMA_S2MM);
	XAie_DmaChannelDisable(DevInst, ShimDma, 0, DMA_MM2S);


	XAie_StatusDump(DevInst, status);
	S2MMStatus_1 =  status[2].ShimTile[0].Dma[1].S2MMStatus;
	MM2SStatus_1 = status[2].ShimTile[0].Dma[0].MM2SStatus;
	LockValue_1 =  status[2].ShimTile[0].LockValue[6];
	printf("Shim Tile: Before Processing\n");
	printf("Dma Status Register(S2MM): 0x%x\n", S2MMStatus_1);
	printf("Dma Status Register(MM2S): 0x%x\n", MM2SStatus_1);
	printf("Lock Value: 0x%x\n", LockValue_1);



	RC = XAie_DmaWriteBd(DevInst, &DmaDesc1, ShimDma, 5);
	RC = XAie_DmaWriteBd(DevInst, &DmaDesc1, ShimDma, 2);

	RC |= XAie_DmaEnableBd(&DmaDesc1);
	RC |= XAie_DmaEnableBd(&DmaDesc2);
	RC = XAie_DmaChannelPushBdToQueue(DevInst, ShimDma, 1, DMA_S2MM, 5);
	RC = XAie_DmaChannelPushBdToQueue(DevInst, ShimDma, 0, DMA_MM2S, 2);

	Lock.LockId = 6;
	Lock.LockVal = 1;
	RC |= XAie_LockSetValue(DevInst, ShimDma, Lock);


	XAie_StatusDump(DevInst, status);
	S2MMStatus_2 =  status[2].ShimTile[0].Dma[1].S2MMStatus;
	MM2SStatus_2 = status[2].ShimTile[0].Dma[0].MM2SStatus;
	LockValue_2 =  status[2].ShimTile[0].LockValue[6];

	printf("Shim Tile: After Processing\n");
	printf("Dma Status Register(S2MM): 0x%x\n", S2MMStatus_2);
	printf("Dma Status Register(MM2S): 0x%x\n", MM2SStatus_2);
	printf("Lock Value: 0x%x\n", LockValue_2);


	if(S2MMStatus_1 != S2MMStatus_2 && MM2SStatus_1 != MM2SStatus_2 &&
			LockValue_1 != LockValue_2) {
		return XAIE_OK;
	}
	else {
		return XAIE_ERR;
	}
}

XAie_ColStatus * XAie_StatusDumpMemAlloc(XAie_DevInst *DevInst, XAie_Mpool *pool)
{
	XAie_ColStatus *Status;
	int i , j;

	const XAie_DmaMod *DmaModCoreTile, *DmaModMemTile, *DmaModShimTile;
	const XAie_EvntMod *EvntCoreMod, *EvntMemMod, *EvntModMemTile, *EvntModShimTile;
	const XAie_LockMod *LockModCoreTile, *LockModMemTile, *LockModShimTile;

	LockModCoreTile = DevInst->DevProp.DevMod[XAIEGBL_TILE_TYPE_AIETILE].LockMod;
	LockModMemTile = DevInst->DevProp.DevMod[XAIEGBL_TILE_TYPE_MEMTILE].LockMod;
	LockModShimTile = DevInst->DevProp.DevMod[XAIEGBL_TILE_TYPE_SHIMPL].LockMod;
	LockModShimTile = DevInst->DevProp.DevMod[XAIEGBL_TILE_TYPE_SHIMNOC].LockMod;

	DmaModCoreTile = DevInst->DevProp.DevMod[XAIEGBL_TILE_TYPE_AIETILE].DmaMod;
	DmaModMemTile = DevInst->DevProp.DevMod[XAIEGBL_TILE_TYPE_MEMTILE].DmaMod;
	DmaModShimTile = DevInst->DevProp.DevMod[XAIEGBL_TILE_TYPE_SHIMPL].DmaMod;
	DmaModShimTile = DevInst->DevProp.DevMod[XAIEGBL_TILE_TYPE_SHIMNOC].DmaMod;


	EvntCoreMod = &DevInst->DevProp.DevMod[XAIEGBL_TILE_TYPE_AIETILE].EvntMod[XAIE_CORE_MOD];
	EvntMemMod = &DevInst->DevProp.DevMod[XAIEGBL_TILE_TYPE_AIETILE].EvntMod[XAIE_MEM_MOD];

	EvntModMemTile = &DevInst->DevProp.DevMod[XAIEGBL_TILE_TYPE_MEMTILE].EvntMod[XAIE_MEM_MOD];
	EvntModShimTile = &DevInst->DevProp.DevMod[XAIEGBL_TILE_TYPE_SHIMPL].EvntMod[0U];
	EvntModShimTile = &DevInst->DevProp.DevMod[XAIEGBL_TILE_TYPE_SHIMNOC].EvntMod[0U];

	Status = malloc(sizeof(XAie_ColStatus)* (DevInst->NumCols));
	if(Status == NULL)
		return NULL;

	printf("Core Tile locks %d\n",LockModCoreTile->NumLocks);
	printf("Mem Tile locks %d\n",LockModMemTile->NumLocks);
	printf("Shim Tile locks %d\n",LockModShimTile->NumLocks);

	for(i = 0; i < DevInst->NumCols; i++)
	{
                Status[i].CoreTile = XAie_MpoolAlloc(sizeof(XAie_CoreTileStatus)
                                * (DevInst->AieTileNumRows), pool);
                if(Status[i].CoreTile == NULL)
                {
                        XAIE_ERROR("Malloc failed\n");
                        return NULL;
                }
                Status[i].MemTile = XAie_MpoolAlloc(sizeof(XAie_MemTileStatus)
                                * (DevInst->MemTileNumRows), pool);
                if(Status[i].MemTile == NULL)
                {
                        XAIE_ERROR("Malloc failed\n");
                        return NULL;
                }
                Status[i].ShimTile = XAie_MpoolAlloc(sizeof(XAie_ShimTileStatus)
                                * (DevInst->AieTileNumRows), pool);
                if(Status[i].ShimTile == NULL)
                {
                        XAIE_ERROR("Malloc failed\n");
                        return NULL;
                }

	}

	for(j =0; j < DevInst->NumCols; j++) {
		for(i =0; i < DevInst->AieTileNumRows; i++) {
                        Status[j].CoreTile[i].Dma = XAie_MpoolAlloc(sizeof(XAie_DmaStatus)
					* (DmaModCoreTile->NumChannels), pool);
                        if(Status[j].CoreTile[i].Dma == NULL)
                        {
                                XAIE_ERROR("Malloc failed\n");
                                return NULL;
                        }
                        Status[j].CoreTile[i].EventCoreModStatus = XAie_MpoolAlloc(sizeof(u32)
					* (EvntCoreMod->NumEventReg), pool);
                        if(Status[j].CoreTile[i].EventCoreModStatus == NULL)
                        {
                                XAIE_ERROR("Malloc failed\n");
                                return NULL;
                        }
                        Status[j].CoreTile[i].EventMemModStatus = XAie_MpoolAlloc(sizeof(u32)
					* (EvntMemMod->NumEventReg), pool);
                        if(Status[j].CoreTile[i].EventMemModStatus == NULL)
                        {
                                XAIE_ERROR("Malloc failed\n");
                                return NULL;
                        }
                        Status[j].CoreTile[i].LockValue = XAie_MpoolAlloc(sizeof(u32)
					* (LockModCoreTile->NumLocks), pool);
                        if(Status[j].CoreTile[i].LockValue == NULL)
                        {
                                XAIE_ERROR("Malloc failed\n");
                                return NULL;
                        }
		}
	}

	if(DevInst->DevProp.DevGen >= XAIE_DEV_GEN_AIEML) {
		for(j =0; j < DevInst->NumCols; j++) {
			for(i =0; i < DevInst->MemTileNumRows; i++) {
                                Status[j].MemTile[i].Dma = XAie_MpoolAlloc(sizeof(XAie_DmaStatus)
                                                * (DmaModMemTile->NumChannels), pool);
                                if(Status[j].MemTile[i].Dma == NULL)
                                {
                                        XAIE_ERROR("Malloc failed\n");
                                        return NULL;
                                }
                                Status[j].MemTile[i].EventStatus = XAie_MpoolAlloc(sizeof(u32)
                                                * (EvntModMemTile->NumEventReg), pool);
                                if(Status[j].MemTile[i].EventStatus == NULL)
                                {
                                        XAIE_ERROR("Malloc failed\n");
                                        return NULL;
                                }
                                Status[j].MemTile[i].LockValue = XAie_MpoolAlloc(sizeof(u32)
                                                * (LockModMemTile->NumLocks), pool);
                                if(Status[j].MemTile[i].LockValue == NULL)
                                {
                                        XAIE_ERROR("Malloc failed\n");
                                        return NULL;
                                }
			}
		}
	}
	for(j =0; j < DevInst->NumCols; j++) {
		for(i =0; i <= DevInst->ShimRow; i++) {
                        Status[j].ShimTile[i].Dma = XAie_MpoolAlloc(sizeof(XAie_DmaStatus)
                                        * (DmaModShimTile->NumChannels), pool);
                        if(Status[j].ShimTile[i].Dma == NULL)
                        {
                                XAIE_ERROR("Malloc failed\n");
                                return NULL;
                        }
                        Status[j].ShimTile[i].EventStatus = XAie_MpoolAlloc(sizeof(u32)
                                        * (EvntModShimTile->NumEventReg), pool);
                        if(Status[j].ShimTile[i].EventStatus == NULL)
                        {
                                XAIE_ERROR("Malloc failed\n");
                                return NULL;
                        }
                        Status[j].ShimTile[i].LockValue = XAie_MpoolAlloc(sizeof(u32)
                                        * (LockModShimTile->NumLocks), pool);
                        if(Status[j].ShimTile[i].LockValue == NULL)
                        {
                                XAIE_ERROR("Malloc failed\n");
                                return NULL;
                        }
		}
	}
	printf("Memory allocation successful  \n");
	return Status;
}

/*****************************************************************************/
/**
*
* This is the main entry point for capturing the column register statuses.
*
* @param	DevInst - Device Instance.
*
* @return	0 on success and error code on failure.
*
* @note		None.
*
*******************************************************************************/
int test_aie_col_status(XAie_DevInst *DevInst)
{
	AieRC RC = XAIE_OK;
	XAie_Mpool *pool;

        XAie_ColStatus *Status;

	pool = XAie_MpoolCreate(10000);
        Status =  XAie_StatusDumpMemAlloc(DevInst, pool);

	/*
	* This test prints captures the column status register values before
        * and after manipulating the core tile, mem tile and shim tile register.
	* The before and after values are compared to see if they are different.
	* If either one of the register values are not different the test would
	* fail.
	*/

	RC = test_aie_core_tile_col_status(DevInst, Status);
	RC |= test_aie_mem_tile_col_status(DevInst, Status);
	RC |= test_aie_shim_tile_col_status(DevInst, Status);

	XAie_MpoolDestroy (pool);
	return RC;
}


